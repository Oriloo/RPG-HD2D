import json, torch
from __future__ import annotations

class state_encoder():
    def __init__(self):
        pass

    def data_to_tensor(self, data, dtype=torch.float32, device="cuda"):
        """
        Transform a list of numbers into a PyTorch tensor.
        """
        return torch.tensor(data, dtype=dtype, device=device)

    def json_to_list(self, path):
        with open(path, 'r', encoding='utf-8') as file:
            data = json.load(file)
        return self.dict_to_list(data)

    def dict_to_list(self, data_dict):
        """Convert a raw observation dict to a flat feature list (length=92)."""
        normalized_data = normalizer(data_dict)

        def extract_effect_values(effect):
            return [effect["code"], effect["multiplier"], effect["duration"]]

        def extract_action_values(action):
            values = [action["cost"]]
            for effect in action["effects"]:
                values.extend(extract_effect_values(effect))
            return values

        values_list = []

        for action in normalized_data["enemy_recent_actions"]:
            values_list.extend(extract_action_values(action))

        for actif in normalized_data["actifs"]["ai_actifs"]:
            values_list.extend(extract_effect_values(actif))

        for actif in normalized_data["actifs"]["enemy_actifs"]:
            values_list.extend(extract_effect_values(actif))

        ai_stats = normalized_data["combat_stats"]["ai_stats"]
        values_list.extend([
            ai_stats["level"], ai_stats["hpMax"], ai_stats["hp"],
            ai_stats["speed"], ai_stats["apMax"], ai_stats["ap"]
        ])

        ai_detailed_stats = ai_stats["stats"]
        values_list.extend([
            ai_detailed_stats["phy_atk"], ai_detailed_stats["phy_def"],
            ai_detailed_stats["spi_atk"], ai_detailed_stats["spi_def"],
            ai_detailed_stats["ele_atk"], ai_detailed_stats["ele_def"]
        ])

        enemy_stats = normalized_data["combat_stats"]["enemy_stats"]
        values_list.extend([enemy_stats["level"], enemy_stats["hpMax"], enemy_stats["hp"]])

        combat_states = normalized_data["combat_states"]
        values_list.extend([combat_states["round_count"], combat_states["action_left"]])

        for action in normalized_data["ai_available_actions"]:
            values_list.extend(extract_action_values(action))

        for action in normalized_data["ai_actions_history"]:
            values_list.extend(extract_action_values(action))

        assert len(values_list) == 92, f"Expected 92 features, got {len(values_list)}"
        return values_list

    # === NEW: build the [5]-action availability mask ==========================
    def build_availability_mask(self, normalized_data) -> torch.Tensor:
        """
        Returns a tensor of shape (5,) for [P1,P2,P3,P4,P5],
        where P1..P4 are 1 if (slot non-empty AND cost <= ap), else 0.
        P5 (Skip) can be 0 here; the decoder will force it to 1.
        """
        ap = normalized_data["combat_stats"]["ai_stats"]["ap"]  # already normalized [0,1]
        # cost is also normalized [0,1] with max=4 → compare in normalized space
        mask = [0, 0, 0, 0, 0]
        actions = normalized_data["ai_available_actions"]  # CHANGED (same rename)

        for i in range(4):
            slot = actions[i]
            # slot is "non-empty" if it has at least one non-zero field
            slot_non_empty = (slot["cost"] > 0.0) or any(
                e["code"] > 0.0 or e["multiplier"] > 0.0 or e["duration"] > 0.0
                for e in slot["effects"]
            )
            # available if non-empty AND cost <= ap
            available = slot_non_empty and (slot["cost"] <= ap)
            mask[i] = 1 if available else 0

        # P5 (Skip) left as 0/1 here; decoder will force it to 1 anyway
        mask[4] = 0
        return torch.tensor(mask, dtype=torch.int32)


class FighterGenBounds:
    """
    Pure helper (no side effects) that mirrors fighter_gen formulas.
    Use FighterGenBounds.at_level(L) to get per-level dynamic bounds.
    """

    # static “global” ceilings implied by the generator
    COST_MIN, COST_MAX = 0, 4
    CODE_MIN, CODE_MAX = 0, 10
    DURATION_MIN, DURATION_MAX = 0, 4  # generator effectively clamps to >=1, keep headroom
    MULT_BASE_MIN, MULT_BASE_MAX = 20, 25  # used for random effects
    MULT_COST_MIN, MULT_COST_MAX = 1, 4    # random actions cost in [1..4]
    MULT_FIXED_MIN, MULT_FIXED_MAX = 15, 20  # for a few predefined effects at cost 0

    @classmethod
    def eff_multiplier_minmax(cls) -> tuple[float, float]:
        # Random effects: randint(20,25) * (cost+1) with cost in [1..4] -> [20*2 .. 25*5] = [40..125]
        rand_min = cls.MULT_BASE_MIN * (cls.MULT_COST_MIN + 1)
        rand_max = cls.MULT_BASE_MAX * (cls.MULT_COST_MAX + 1)
        # Fixed effects exist at cost 0: [15..20]
        return (0.0, max(rand_max, cls.MULT_FIXED_MAX))  # [0 .. 125]

    @staticmethod
    def hpmax_at_level(level: int) -> tuple[float, float]:
        # In generator: r in [80..120];  hpMax = r + r * (level-1) * 0.75
        r_min, r_max = 80.0, 120.0
        mult = 1.0 + max(0, level - 1) * 0.75
        return (r_min * mult, r_max * mult)

    @staticmethod
    def speed_at_level(level: int) -> tuple[float, float]:
        # speed = randint(3,5) + level
        return (3 + level, 5 + level)

    @staticmethod
    def apmax_at_level(level: int) -> tuple[float, float]:
        # apMax = 4 + int(level/5)  -> 4 (L<5) or 5 (L==5)
        return (4.0, 4.0 + (1.0 if level >= 5 else 0.0))

    @staticmethod
    def ap_range(level: int) -> tuple[float, float]:
        lo, hi = FighterGenBounds.apmax_at_level(level)
        return (0.0, hi)

    @staticmethod
    def stat_max_at_level(level: int) -> float:
        # init_stat = 30 * level
        # principale up to 35% → 0.35 * 30 * level = 10.5 * level
        return 10.5 * level

    @classmethod
    def at_level(cls, level: int) -> dict:
        """Convenience bundle of dynamic bounds for a given level."""
        hp_lo, hp_hi = cls.hpmax_at_level(level)
        spd_lo, spd_hi = cls.speed_at_level(level)
        apM_lo, apM_hi = cls.apmax_at_level(level)
        ap_lo, ap_hi = cls.ap_range(level)
        mult_lo, mult_hi = cls.eff_multiplier_minmax()
        stat_hi = cls.stat_max_at_level(level)
        return {
            "level": (1.0, 5.0),
            "hpMax": (hp_lo, hp_hi),
            "hp":    (0.0, hp_hi),
            "speed": (spd_lo, spd_hi),
            "apMax": (apM_lo, apM_hi),
            "ap":    (ap_lo, ap_hi),

            # detailed stats (we normalize vs principale ceiling for safety)
            "stat":  (0.0, stat_hi),

            # action/effect fields
            "cost":       (cls.COST_MIN, cls.COST_MAX),
            "code":       (cls.CODE_MIN, cls.CODE_MAX),
            "multiplier": (mult_lo, mult_hi),
            "duration":   (cls.DURATION_MIN, cls.DURATION_MAX),

            # combat state fields
            "round_count": (0.0, 50.0),  # keep env cap headroom
            "action_left": (0.0, 3.0),   # MAX_ACTIONS_PER_TURN
        }

def normalizer(data: dict) -> dict:
    """
    Normalize the exact 92-feature structure your state_encoder expects,
    but derive min/max from fighter_gen's formulas at the CURRENT AI level.
    Output structure is unchanged.
    """
    def clamp01(x: float) -> float:
        return 0.0 if x <= 0.0 else (1.0 if x >= 1.0 else x)

    def norm_val(v, lo, hi) -> float:
        lo = float(lo); hi = float(hi)
        if hi <= lo:  # safety
            return 0.0
        return clamp01((float(v) - lo) / (hi - lo))

    # --- 1) determine the AI level and retrieve dynamic bounds --------------
    ai_src = data["combat_stats"]["ai_stats"]
    level = int(ai_src.get("level", 1))
    level = 1 if level < 1 else (5 if level > 5 else level)
    B = FighterGenBounds.at_level(level)

    # Small helpers for common fields
    def n_cost(x):  lo,hi = B["cost"];       return norm_val(x, lo, hi)
    def n_code(x):  lo,hi = B["code"];       return norm_val(x, lo, hi)
    def n_mult(x):  lo,hi = B["multiplier"]; return norm_val(x, lo, hi)
    def n_dur(x):   lo,hi = B["duration"];   return norm_val(x, lo, hi)

    def norm_effect(e: dict) -> dict:
        return {
            "code":       n_code(e["code"]),
            "multiplier": n_mult(e["multiplier"]),
            "duration":   n_dur(e["duration"]),
        }

    def norm_action(a: dict) -> dict:
        return {
            "cost":    n_cost(a["cost"]),
            "effects": [norm_effect(x) for x in a["effects"]],
        }

    out = {}

    # --- 2) enemy recent actions (3) ----------------------------------------
    out["enemy_recent_actions"] = [norm_action(a) for a in data["enemy_recent_actions"]]

    # --- 3) actifs (ai + enemy) ---------------------------------------------
    out["actifs"] = {
        "ai_actifs":    [norm_effect(x) for x in data["actifs"]["ai_actifs"]],
        "enemy_actifs": [norm_effect(x) for x in data["actifs"]["enemy_actifs"]],
    }

    # --- 4) combat stats (ai + enemy) ---------------------------------------
    # ai
    hpMax_lo, hpMax_hi = B["hpMax"]
    hp_lo, hp_hi       = B["hp"]
    spd_lo, spd_hi     = B["speed"]
    apM_lo, apM_hi     = B["apMax"]
    ap_lo, ap_hi       = B["ap"]
    stat_lo, stat_hi   = B["stat"]

    out["combat_stats"] = {
        "ai_stats": {
            "level": norm_val(ai_src["level"], *B["level"]),
            "hpMax": norm_val(ai_src["hpMax"], hpMax_lo, hpMax_hi),
            "hp":    norm_val(ai_src["hp"],    hp_lo,    hp_hi),
            "speed": norm_val(ai_src["speed"], spd_lo,   spd_hi),
            "apMax": norm_val(ai_src["apMax"], apM_lo,   apM_hi),
            "ap":    norm_val(ai_src["ap"],    ap_lo,    ap_hi),
            "stats": {
                "phy_atk": norm_val(ai_src["stats"]["phy_atk"], stat_lo, stat_hi),
                "phy_def": norm_val(ai_src["stats"]["phy_def"], stat_lo, stat_hi),
                "spi_atk": norm_val(ai_src["stats"]["spi_atk"], stat_lo, stat_hi),
                "spi_def": norm_val(ai_src["stats"]["spi_def"], stat_lo, stat_hi),
                "ele_atk": norm_val(ai_src["stats"]["ele_atk"], stat_lo, stat_hi),
                "ele_def": norm_val(ai_src["stats"]["ele_def"], stat_lo, stat_hi),
            },
        },
        # enemy (we don’t know enemy level’s role allocation → use same per-AI level ranges;
        # this matches how you trained/encode and keeps magnitudes consistent)
        "enemy_stats": {
            "level": norm_val(data["combat_stats"]["enemy_stats"]["level"], *B["level"]),
            "hpMax": norm_val(data["combat_stats"]["enemy_stats"]["hpMax"], hpMax_lo, hpMax_hi),
            "hp":    norm_val(data["combat_stats"]["enemy_stats"]["hp"],    hp_lo,    hp_hi),
        },
    }

    # --- 5) combat states ----------------------------------------------------
    out["combat_states"] = {
        "round_count": norm_val(data["combat_states"]["round_count"], *B["round_count"]),
        "action_left": norm_val(data["combat_states"]["action_left"], *B["action_left"]),
    }

    # --- 6) available actions (4) + history (2) ------------------------------
    out["ai_available_actions"] = [norm_action(a) for a in data["ai_available_actions"]]
    out["ai_actions_history"]   = [norm_action(a) for a in data["ai_actions_history"]]

    return out


def decode_action(nn_output, mask):
    """
    nn_output: tensor of shape (5,) from the model  
    mask:   tensor of shape (5,), 1 = available, 0 = unavailable
            (Skip can be 0 here, we will force it to 1)

    Returns:
      probs: tensor of shape (5,), normalized probabilities
      choice: int index 0..4 of the chosen action
      output_json: dict with P1..P5 probabilities
    """
    # === NEW: ensure stable dtype ===============================
    nn_output = nn_output.to(torch.float32)
    mask = mask.to(torch.int)
    # ============================================================

    # 1. Force Skip (last index) to always be available
    mask = mask.clone()
    mask[-1] = 1

    # 2. Zero out nn_output for unavailable actions
    masked_nn_output = nn_output.clone()
    masked_nn_output[mask == 0] = -1e9  # very negative = prob ~ 0

    # 3. Softmax to turn nn_output into probabilities
    probs = torch.softmax(masked_nn_output, dim=0)

    # 4. Pick the action with the highest probability
    choice = int(torch.argmax(probs).item())

    # 5. Build JSON-style output
    output_json = {
        "P1": float(probs[0]),
        "P2": float(probs[1]),
        "P3": float(probs[2]),
        "P4": float(probs[3]),
        "P5": float(probs[4]),
        "chosen_index": choice
    }

    return probs, choice, output_json


# === Define your trained model shape (must match training) ===
class PolicyMLP(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.net = torch.nn.Sequential(
            torch.nn.Linear(92, 128), torch.nn.ReLU(),
            torch.nn.Linear(128, 64), torch.nn.ReLU(),
            torch.nn.Linear(64, 5),
        )
    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.net(x)


def main():
    # 1. Load raw JSON data
    with open("features.json", "r", encoding="utf-8") as f:
        raw_data = json.load(f)

    # 2. Normalize + encode to 92 features
    encoder = state_encoder()
    feature_list = encoder.dict_to_list(raw_data)
    x = encoder.data_to_tensor(feature_list, dtype=torch.float32).unsqueeze(0)  # [1,92]

    # 3. Build mask from normalized data
    normalized_data = normalizer(raw_data)
    mask = encoder.build_availability_mask(normalized_data)

    # 4. Load model
    model = PolicyMLP()
    model.load_state_dict(torch.load("Python/best_final - pop192 gen1024.pt", map_location="cpu"))
    model.eval()

    # 5. Run inference
    with torch.no_grad():
        nn_output = model(x)[0]  # [5]

    # 6. Decode result
    _, choice, output_json = decode_action(nn_output, mask)

    # 7. Print and save result
    print("Model output:", output_json)
    with open("Python/ai_result.json", "w", encoding="utf-8") as f:
        json.dump(output_json, f, indent=2)


if __name__ == "__main__":
    main()
