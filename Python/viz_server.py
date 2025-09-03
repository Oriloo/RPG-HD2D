from __future__ import annotations
import os, json, math
from dataclasses import dataclass
from typing import List, Dict, Any

from flask import Flask, jsonify, send_from_directory

# Reuse model/encoding logic (copied from main.py to keep this file standalone).
import torch


# ------------------------- Adapter for payload --------------------------
def adapt_payload_to_encoder_schema(d: dict) -> dict:
    ai_stats_src = dict(d["combat_stats"]["ai_stats"])  # shallow copy
    enemy_stats_src = ai_stats_src.pop("ply_stats", None)
    out = {
        "enemy_recent_actions": d["ply_recent_actions"],
        "actifs": {
            "ai_actifs":    d["actives"]["ai_active_effects"],
            "enemy_actifs": d["actives"]["ply_active_effects"],
        },
        "combat_stats": {
            "ai_stats": ai_stats_src,
            "enemy_stats": enemy_stats_src or {
                "level": 1,
                "hpMax": ai_stats_src.get("hpMax", 100),
                "hp":    ai_stats_src.get("hpMax", 100),
            },
        },
        "combat_states": d["combat_state"],
        "ai_available_actions": d["ai_available_actions"],
        "ai_actions_history":   d["ai_actions_history"],
    }
    return out


class FighterGenBounds:
    COST_MIN, COST_MAX = 0, 4
    CODE_MIN, CODE_MAX = 0, 10
    DURATION_MIN, DURATION_MAX = 0, 4
    MULT_BASE_MIN, MULT_BASE_MAX = 20, 25
    MULT_COST_MIN, MULT_COST_MAX = 1, 4
    MULT_FIXED_MIN, MULT_FIXED_MAX = 15, 20

    @classmethod
    def eff_multiplier_minmax(cls) -> tuple[float, float]:
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
            "code":       (cls.CODE_MIN, cls.CODE_MAX),
            "multiplier": (mult_lo, mult_hi),
            "duration":   (cls.DURATION_MIN, cls.DURATION_MAX),
            "round_count": (0.0, 50.0),
            "action_left": (0.0, 3.0),
        }


def normalizer(data: dict) -> dict:
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
        return {"code": n_code(e["code"]), "multiplier": n_mult(e["multiplier"]), "duration": n_dur(e["duration"]) }

    def norm_action(a: dict) -> dict:
        return {"cost": n_cost(a["cost"]), "effects": [norm_effect(x) for x in a["effects"]] }

    out = {}
    out["enemy_recent_actions"] = [norm_action(a) for a in data["enemy_recent_actions"]]
    out["actifs"] = {
        "ai_actifs":    [norm_effect(x) for x in data["actifs"]["ai_actifs"]],
        "enemy_actifs": [norm_effect(x) for x in data["actifs"]["enemy_actifs"]],
    }

    B_level = B["level"]
    hpMax_lo, hpMax_hi = B["hpMax"]; hp_lo, hp_hi = B["hp"]
    spd_lo, spd_hi = B["speed"]; apM_lo, apM_hi = B["apMax"]; ap_lo, ap_hi = B["ap"]
    stat_lo, stat_hi = B["stat"]

    out["combat_stats"] = {
        "ai_stats": {
            "level": norm_val(ai_src["level"], *B_level),
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
            "level": norm_val(data["combat_stats"]["enemy_stats"]["level"], *B_level),
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


class PolicyMLP(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.fc1 = torch.nn.Linear(92, 128)
        self.fc2 = torch.nn.Linear(128, 64)
        self.fc3 = torch.nn.Linear(64, 5)
        self.relu = torch.nn.ReLU()

    def forward(self, x: torch.Tensor):
        h1 = self.relu(self.fc1(x))
        h2 = self.relu(self.fc2(h1))
        out = self.fc3(h2)
        return h1, h2, out


# ------------------------- Feature flattening & names -----------------------
def extract_effect_values(effect: dict) -> List[float]:
    return [effect["code"], effect["multiplier"], effect["duration"]]


def extract_action_values(action: dict) -> List[float]:
    values = [action["cost"]]
    for effect in action["effects"]:
        values.extend(extract_effect_values(effect))
    return values


def feature_names_from_normalized(n: dict) -> List[str]:
    names: List[str] = []

    # enemy_recent_actions
    for i, action in enumerate(n["enemy_recent_actions"], start=1):
        names.append(f"enemy_recent_actions[{i}].cost")
        for j, _ in enumerate(action["effects"], start=1):
            names.extend([
                f"enemy_recent_actions[{i}].effects[{j}].code",
                f"enemy_recent_actions[{i}].effects[{j}].multiplier",
                f"enemy_recent_actions[{i}].effects[{j}].duration",
            ])

    # actifs
    for who in ("ai_actifs", "enemy_actifs"):
        for j, _ in enumerate(n["actifs"][who], start=1):
            names.extend([
                f"{who}[{j}].code",
                f"{who}[{j}].multiplier",
                f"{who}[{j}].duration",
            ])

    # ai_stats basics
    names.extend(["ai.level", "ai.hpMax", "ai.hp", "ai.speed", "ai.apMax", "ai.ap"])

    # ai.stats detailed
    names.extend([
        "ai.stats.phy_atk", "ai.stats.phy_def",
        "ai.stats.spi_atk", "ai.stats.spi_def",
        "ai.stats.ele_atk", "ai.stats.ele_def",
    ])

    # enemy_stats
    names.extend(["enemy.level", "enemy.hpMax", "enemy.hp"])

    # combat_states
    names.extend(["combat.round_count", "combat.action_left"])

    # ai_available_actions
    for i, action in enumerate(n["ai_available_actions"], start=1):
        names.append(f"ai_available_actions[{i}].cost")
        for j, _ in enumerate(action["effects"], start=1):
            names.extend([
                f"ai_available_actions[{i}].effects[{j}].code",
                f"ai_available_actions[{i}].effects[{j}].multiplier",
                f"ai_available_actions[{i}].effects[{j}].duration",
            ])

    # ai_actions_history
    for i, action in enumerate(n["ai_actions_history"], start=1):
        names.append(f"ai_actions_history[{i}].cost")
        for j, _ in enumerate(action["effects"], start=1):
            names.extend([
                f"ai_actions_history[{i}].effects[{j}].code",
                f"ai_actions_history[{i}].effects[{j}].multiplier",
                f"ai_actions_history[{i}].effects[{j}].duration",
            ])

    assert len(names) == 92, f"Expected 92 names, got {len(names)}"
    return names


def flatten_features_from_normalized(n: dict) -> List[float]:
    values_list: List[float] = []

    for action in n["enemy_recent_actions"]:
        values_list.extend(extract_action_values(action))

    for actif in n["actifs"]["ai_actifs"]:
        values_list.extend(extract_effect_values(actif))

    for actif in n["actifs"]["enemy_actifs"]:
        values_list.extend(extract_effect_values(actif))

    ai_stats = n["combat_stats"]["ai_stats"]
    values_list.extend([
        ai_stats["level"], ai_stats["hpMax"], ai_stats["hp"],
        ai_stats["speed"], ai_stats["apMax"], ai_stats["ap"],
    ])

    ai_detailed_stats = ai_stats["stats"]
    values_list.extend([
        ai_detailed_stats["phy_atk"], ai_detailed_stats["phy_def"],
        ai_detailed_stats["spi_atk"], ai_detailed_stats["spi_def"],
        ai_detailed_stats["ele_atk"], ai_detailed_stats["ele_def"],
    ])

    enemy_stats = n["combat_stats"]["enemy_stats"]
    values_list.extend([enemy_stats["level"], enemy_stats["hpMax"], enemy_stats["hp"]])

    combat_states = n["combat_states"]
    values_list.extend([combat_states["round_count"], combat_states["action_left"]])

    for action in n["ai_available_actions"]:
        values_list.extend(extract_action_values(action))

    for action in n["ai_actions_history"]:
        values_list.extend(extract_action_values(action))

    assert len(values_list) == 92, f"Expected 92 features, got {len(values_list)}"
    return values_list


def flatten_features_from_raw(raw: dict) -> List[float]:
    values_list: List[float] = []

    for action in raw["enemy_recent_actions"]:
        values_list.extend(extract_action_values(action))

    for actif in raw["actifs"]["ai_actifs"]:
        values_list.extend(extract_effect_values(actif))

    for actif in raw["actifs"]["enemy_actifs"]:
        values_list.extend(extract_effect_values(actif))

    ai_stats = raw["combat_stats"]["ai_stats"]
    values_list.extend([
        ai_stats["level"], ai_stats["hpMax"], ai_stats["hp"],
        ai_stats["speed"], ai_stats["apMax"], ai_stats["ap"],
    ])

    ai_detailed_stats = ai_stats["stats"]
    values_list.extend([
        ai_detailed_stats["phy_atk"], ai_detailed_stats["phy_def"],
        ai_detailed_stats["spi_atk"], ai_detailed_stats["spi_def"],
        ai_detailed_stats["ele_atk"], ai_detailed_stats["ele_def"],
    ])

    enemy_stats = raw["combat_stats"]["enemy_stats"]
    values_list.extend([enemy_stats["level"], enemy_stats["hpMax"], enemy_stats["hp"]])

    combat_states = raw["combat_states"]
    values_list.extend([combat_states["round_count"], combat_states["action_left"]])

    for action in raw["ai_available_actions"]:
        values_list.extend(extract_action_values(action))

    for action in raw["ai_actions_history"]:
        values_list.extend(extract_action_values(action))

    assert len(values_list) == 92, f"Expected 92 features, got {len(values_list)}"
    return values_list


# ------------------------- Server & Endpoints -------------------------------
app = Flask(__name__, static_folder=None)


def load_features(script_dir: str) -> dict:
    path = os.path.join(script_dir, "features.json")
    with open(path, "r", encoding="utf-8-sig") as f:
        d = json.load(f)
    return d


def load_result(script_dir: str) -> dict | None:
    path = os.path.join(script_dir, "ai_result.json")
    if not os.path.exists(path):
        return None
    with open(path, "r", encoding="utf-8") as f:
        try:
            return json.load(f)
        except Exception:
            return None


_MODEL: PolicyMLP | None = None


def get_model(script_dir: str) -> PolicyMLP:
    global _MODEL
    if _MODEL is not None:
        return _MODEL
    model = PolicyMLP()
    model_path = os.path.join(script_dir, "best_final - pop192 gen1024.pt")
    checkpoint = torch.load(model_path, map_location="cpu")
    if isinstance(checkpoint, dict):
        if "best_state_dict" in checkpoint:
            state_dict = checkpoint["best_state_dict"]
        elif "state_dict" in checkpoint:
            state_dict = checkpoint["state_dict"]
        else:
            state_dict = checkpoint
    else:
        state_dict = checkpoint
    try:
        model.load_state_dict(state_dict)
    except RuntimeError:
        model.load_state_dict(state_dict, strict=False)
    model.eval()
    _MODEL = model
    return _MODEL


@app.get("/")
def serve_visualizer():
    # Serve the HTML file from the same directory
    base = os.path.dirname(__file__)
    return send_from_directory(base, "nn_visualizer.html")


@app.get("/api/arch")
def api_arch():
    return jsonify({"layers": [92, 128, 64, 5]})


@app.get("/api/features")
def api_features():
    base = os.path.dirname(__file__)
    raw_user = load_features(base)
    raw = adapt_payload_to_encoder_schema(raw_user)
    n = normalizer(raw)
    names = feature_names_from_normalized(n)
    vals = flatten_features_from_normalized(n)
    raws = flatten_features_from_raw(raw)
    return jsonify({
        "feature_names": names,
        "features_normalized": vals,
        "features_raw": raws,
        "raw_obj": raw_user,
    })


@app.get("/api/outputs")
def api_outputs():
    base = os.path.dirname(__file__)
    res = load_result(base)
    return jsonify(res or {})


@app.get("/api/forward")
def api_forward():
    base = os.path.dirname(__file__)
    raw_user = load_features(base)
    raw = adapt_payload_to_encoder_schema(raw_user)
    n = normalizer(raw)
    x_list = flatten_features_from_normalized(n)
    raw_list = flatten_features_from_raw(raw)
    x = torch.tensor([x_list], dtype=torch.float32)

    model = get_model(base)
    with torch.no_grad():
        h1, h2, out = model(x)
        logits = out[0]
        probs = torch.softmax(logits, dim=0)

    names = feature_names_from_normalized(n)

    return jsonify({
        "feature_names": names,
        "features_normalized": x_list,
        "features_raw": raw_list,
        "h1": h1[0].tolist(),
        "h2": h2[0].tolist(),
        "logits": logits.tolist(),
        "probs": probs.tolist(),
    })


def main():
    port = int(os.environ.get("PORT", "8008"))
    app.run(host="127.0.0.1", port=port, debug=False)


if __name__ == "__main__":
    main()
