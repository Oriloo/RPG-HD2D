from __future__ import annotations
import json, torch, os

# ------------------------- Adapter for your payload --------------------------
def adapt_payload_to_encoder_schema(d: dict) -> dict:
    """
    Map your JSON schema (ply_recent_actions, actives, combat_state, nested ply_stats)
    into the internal schema used by the encoder/normalizer.
    """
    # Copy AI stats and peel out nested player/enemy stats
    ai_stats_src = dict(d["combat_stats"]["ai_stats"])  # shallow copy
    enemy_stats_src = ai_stats_src.pop("ply_stats", None)

    # Build the structure expected internally
    out = {
        "enemy_recent_actions": d["ply_recent_actions"],
        "actifs": {
            "ai_actifs":    d["actives"]["ai_active_effects"],
            "enemy_actifs": d["actives"]["ply_active_effects"],
        },
        "combat_stats": {
            "ai_stats": ai_stats_src,
            "enemy_stats": enemy_stats_src or {
                # Fallbacks (keeps normalizer safe if missing)
                "level": 1,
                "hpMax": ai_stats_src.get("hpMax", 100),
                "hp":    ai_stats_src.get("hpMax", 100),
            },
        },
        # pluralize to match internal naming
        "combat_states": d["combat_state"],
        "ai_available_actions": d["ai_available_actions"],
        "ai_actions_history":   d["ai_actions_history"],
    }
    return out

# ------------------------------ Encoder -------------------------------------
class state_encoder():
    def __init__(self):
        pass

    def data_to_tensor(self, data, dtype=torch.float32, device="cpu"):
        """Transform a list of numbers into a PyTorch tensor (default CPU)."""
        return torch.tensor(data, dtype=dtype, device=device)

    def json_to_list(self, path):
        with open(path, 'r', encoding='utf-8') as file:
            raw_user = json.load(file)
        # adapt then encode
        return self.dict_to_list(adapt_payload_to_encoder_schema(raw_user))

    def dict_to_list(self, data_dict):
        """Convert a raw observation dict to a flat feature list (length=92)."""
        normalized_data = normalizer(data_dict)

        def extract_effect_values(effect):
            return [effect["code"], effect["multiplier"], effect["duration"] + 1]

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

    # === [5]-action availability mask =======================================
    def build_availability_mask(self, normalized_data) -> torch.Tensor:
        """
        Returns a tensor of shape (5,) for [P1,P2,P3,P4,P5],
        where P1..P4 are 1 if (slot non-empty AND cost <= ap), else 0.
        P5 (Skip) can be 0; the decoder will force it to 1.
        """
        ap = normalized_data["combat_stats"]["ai_stats"]["ap"]  # normalized [0,1]
        mask = [0, 0, 0, 0, 0]
        actions = normalized_data["ai_available_actions"]

        for i in range(4):
            slot = actions[i]
            slot_non_empty = (slot["cost"] > 0.0) or any(
                e["code"] > 0.0 or e["multiplier"] > 0.0 or e["duration"] > 0.0
                for e in slot["effects"]
            )
            available = slot_non_empty and (slot["cost"] <= ap)
            mask[i] = 1 if available else 0

        mask[4] = 0
        return torch.tensor(mask, dtype=torch.int32)

# ------------------------------ Bounds & Normalizer --------------------------
class FighterGenBounds:
    COST_MIN, COST_MAX = 0, 4
    CODE_MIN, CODE_MAX = 0, 10
    DURATION_MIN, DURATION_MAX = 0, 4
    MULT_BASE_MIN, MULT_BASE_MAX = 20, 25
    MULT_COST_MIN, MULT_COST_MAX = 1, 4
    MULT_FIXED_MIN, MULT_FIXED_MAX = 15, 20

    @classmethod
    def eff_multiplier_minmax(cls) -> tuple[float, float]:
        # randint(20,25) * (cost+1) with cost in [1..4] -> [40..125]
        rand_max = cls.MULT_BASE_MAX * (cls.MULT_COST_MAX + 1)
        return (0.0, max(rand_max, cls.MULT_FIXED_MAX))  # [0 .. 125]

    @staticmethod
    def hpmax_at_level(level: int) -> tuple[float, float]:
        r_min, r_max = 80.0, 120.0
        mult = 1.0 + max(0, level - 1) * 0.75
        return (r_min * mult, r_max * mult)

    @staticmethod
    def speed_at_level(level: int) -> tuple[float, float]:
        return (3 + level, 5 + level)

    @staticmethod
    def apmax_at_level(level: int) -> tuple[float, float]:
        return (4.0, 4.0 + (1.0 if level >= 5 else 0.0))

    @staticmethod
    def ap_range(level: int) -> tuple[float, float]:
        lo, hi = FighterGenBounds.apmax_at_level(level)
        return (0.0, hi)

    @staticmethod
    def stat_max_at_level(level: int) -> float:
        return 10.5 * level

    @classmethod
    def at_level(cls, level: int) -> dict:
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
            "stat":  (0.0, stat_hi),
            "cost":       (cls.COST_MIN, cls.COST_MAX),
            "code":       (cls.CODE_MIN, cls.Code_MAX) if False else (cls.CODE_MIN, cls.CODE_MAX),  # keep calm, linters :)
            "multiplier": (mult_lo, mult_hi),
            "duration":   (cls.DURATION_MIN, cls.DURATION_MAX),
            "round_count": (0.0, 50.0),
            "action_left": (0.0, 3.0),
        }

def normalizer(data: dict) -> dict:
    """
    Accepts *already-adapted* schema (use adapt_payload_to_encoder_schema first).
    Normalizes values to [0,1] using level-aware bounds.
    """
    def clamp01(x: float) -> float:
        return 0.0 if x <= 0.0 else (1.0 if x >= 1.0 else x)

    def norm_val(v, lo, hi) -> float:
        lo = float(lo); hi = float(hi)
        if hi <= lo:
            return 0.0
        return clamp01((float(v) - lo) / (hi - lo))

    ai_src = data["combat_stats"]["ai_stats"]
    level = int(ai_src.get("level", 1))
    level = 1 if level < 1 else (5 if level > 5 else level)
    B = FighterGenBounds.at_level(level)

    def n_cost(x):  lo,hi = B["cost"];       return norm_val(x, lo, hi)
    def n_code(x):  lo,hi = B["code"];       return norm_val(x, lo, hi)
    def n_mult(x):  lo,hi = B["multiplier"]; return norm_val(x, lo, hi)
    def n_dur(x):   lo,hi = B["duration"];   return norm_val(x, lo, hi)

    def norm_effect(e: dict) -> dict:
        return {"code": n_code(e["code"]), "multiplier": n_mult(e["multiplier"]), "duration": n_dur(e["duration"])}

    def norm_action(a: dict) -> dict:
        return {"cost": n_cost(a["cost"]), "effects": [norm_effect(x) for x in a["effects"]]}

    out = {}
    out["enemy_recent_actions"] = [norm_action(a) for a in data["enemy_recent_actions"]]
    out["actifs"] = {
        "ai_actifs":    [norm_effect(x) for x in data["actifs"]["ai_actifs"]],
        "enemy_actifs": [norm_effect(x) for x in data["actifs"]["enemy_actifs"]],
    }

    hpMax_lo, hpMax_hi = B["hpMax"]; hp_lo, hp_hi = B["hp"]
    spd_lo, spd_hi = B["speed"]; apM_lo, apM_hi = B["apMax"]; ap_lo, ap_hi = B["ap"]
    stat_lo, stat_hi = B["stat"]

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
        "enemy_stats": {
            "level": norm_val(data["combat_stats"]["enemy_stats"]["level"], *B["level"]),
            "hpMax": norm_val(data["combat_stats"]["enemy_stats"]["hpMax"], hpMax_lo, hpMax_hi),
            "hp":    norm_val(data["combat_stats"]["enemy_stats"]["hp"],    hp_lo,    hp_hi),
        },
    }

    out["combat_states"] = {
        "round_count": norm_val(data["combat_states"]["round_count"], *B["round_count"]),
        "action_left": norm_val(data["combat_states"]["action_left"], *B["action_left"]),
    }

    out["ai_available_actions"] = [norm_action(a) for a in data["ai_available_actions"]]
    out["ai_actions_history"]   = [norm_action(a) for a in data["ai_actions_history"]]
    return out

# ------------------------------ Decoder -------------------------------------
def decode_action(nn_output, mask):
    nn_output = nn_output.to(torch.float32)
    mask = mask.to(torch.int)

    mask = mask.clone()
    mask[-1] = 1  # force Skip available

    masked_nn_output = nn_output.clone()
    masked_nn_output[mask == 0] = -1e9
    probs = torch.softmax(masked_nn_output, dim=0)
    choice = int(torch.argmax(probs).item())
    output_json = {
        "P1": float(probs[0]), "P2": float(probs[1]), "P3": float(probs[2]),
        "P4": float(probs[3]), "P5": float(probs[4]), "chosen_index": choice
    }
    return probs, choice, output_json

# ------------------------------ Model ---------------------------------------
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

# ------------------------------- Main ---------------------------------------
def main():
    script_dir = os.path.dirname(__file__)
    # 1) Read user-format JSON
    features_path = os.path.join(script_dir, "features.json")
    # Use utf-8-sig to gracefully handle files saved with a UTF-8 BOM
    with open(features_path, "r", encoding="utf-8-sig") as f:
        raw_user = json.load(f)

    # 2) Adapt to internal schema used by encoder/normalizer
    raw = adapt_payload_to_encoder_schema(raw_user)

    # 3) 92-feature encoding
    encoder = state_encoder()
    feature_list = encoder.dict_to_list(raw)
    x = encoder.data_to_tensor(feature_list, dtype=torch.float32, device="cpu").unsqueeze(0)  # [1,92]

    # 4) Mask from normalized data
    normalized = normalizer(raw)
    mask = encoder.build_availability_mask(normalized)

    # 5) Load model
    model = PolicyMLP()
    model_path = os.path.join(script_dir, "best_final - pop192 gen1024.pt")
    # Load checkpoint robustly: file may contain a dict with wrapper keys
    checkpoint = torch.load(model_path, map_location="cpu")
    # Extract common nested keys if present
    if isinstance(checkpoint, dict):
        if "best_state_dict" in checkpoint:
            state_dict = checkpoint["best_state_dict"]
        elif "state_dict" in checkpoint:
            state_dict = checkpoint["state_dict"]
        else:
            state_dict = checkpoint
    else:
        state_dict = checkpoint
    # Try strict load first, then fallback to non-strict to handle minor mismatches
    try:
        model.load_state_dict(state_dict)
    except RuntimeError as e:
        print("Warning: strict load failed, retrying with strict=False:", e)
        model.load_state_dict(state_dict, strict=False)
    model.eval()

    # 6) Inference
    with torch.no_grad():
        nn_output = model(x)[0]  # [5]

    # 7) Decode + save
    _, choice, output_json = decode_action(nn_output, mask)
    print("Model output:", output_json)
    out_path = os.path.join(script_dir, "ai_result.json")
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(output_json, f, indent=2)

if __name__ == "__main__":
    import traceback, os
    script_dir = os.path.dirname(__file__)
    try:
        print("CWD:", os.getcwd())
        main()
    except Exception as e:
        print("Exception during run:", e)
        traceback.print_exc()
        # Try to write an error file so user can inspect what went wrong
        err = {"error": str(e), "traceback": traceback.format_exc()}
        try:
            out_err = os.path.join(script_dir, "ai_result.json")
            with open(out_err, "w", encoding="utf-8") as f:
                json.dump(err, f, indent=2)
            print(f"Wrote error details to {out_err}")
        except Exception as e2:
            print("Failed to write error file:", e2)