# Combat Policy File Runner - Python Version

This Python script is a direct equivalent of the C++ `CombatPolicyRunner` class from your Unreal Engine project. Instead of reading combat data from the game state, it reads JSON data from a `features.json` file and processes it in exactly the same way.

## What it does

The Python version replicates the complete C++ functionality:

1. **JSON Loading**: Reads combat data from `features.json` file
2. **JSON Parsing**: Parses the JSON string into a dictionary structure
3. **Data Normalization**: Normalizes all values to 0-1 ranges using the same bounds as the C++ version
4. **Feature Encoding**: Flattens the normalized data into exactly 92 features
5. **Mock AI**: Runs the same simple heuristic-based AI logic

## Files

- `combat_policy_file_runner.py` - Main Python equivalent of CombatPolicyRunner
- `test_combat_policy.py` - Test script with sample data
- `features.json` - JSON data file (created by the Unreal Engine CombatJsonExporter)

## Usage

### Basic Usage

```bash
# Run with default features.json file
python combat_policy_file_runner.py

# Run with custom JSON file
python combat_policy_file_runner.py --features-file my_combat_data.json

# Enable verbose output
python combat_policy_file_runner.py --verbose
```

### Running Tests

```bash
# Run the test script (will create sample data if needed)
python test_combat_policy.py
```

## JSON File Format

The `features.json` file should contain combat data in this format:

```json
{
  "ply_recent_actions": [
    {
      "cost": 2,
      "effects": [
        {"code": 3, "multiplier": 25, "duration": 0},
        {"code": 0, "multiplier": 0, "duration": 0}
      ]
    }
  ],
  "actives": {
    "ai_active_effects": [...],
    "ply_active_effects": [...]
  },
  "combat_stats": {
    "ai_stats": {
      "level": 2,
      "hpMax": 150,
      "hp": 120,
      "stats": {
        "phy_atk": 18,
        "phy_def": 12,
        ...
      },
      "ply_stats": {
        "level": 1,
        "hpMax": 100,
        "hp": 85
      }
    }
  },
  "combat_state": {
    "round_count": 3,
    "action_left": 2
  },
  "ai_available_actions": [...],
  "ai_actions_history": [...]
}
```

## Feature Vector Structure

The script produces exactly 92 features in this order:

1. **Enemy Recent Actions** (21 features): 3 actions × 7 features each
2. **AI Active Effects** (6 features): 2 effects × 3 features each
3. **Enemy Active Effects** (6 features): 2 effects × 3 features each
4. **AI Stats** (12 features): level, hp_max, hp, speed, ap_max, ap, 6 combat stats
5. **Enemy Stats** (3 features): level, hp_max, hp
6. **Combat State** (2 features): round_count, action_left
7. **AI Available Actions** (28 features): 4 actions × 7 features each
8. **AI Action History** (14 features): 2 actions × 7 features each

**Total: 92 features** (all normalized to 0-1 range)

## Key Differences from C++

- **File-based**: Reads from JSON file instead of game state
- **Python syntax**: Uses Python idioms but maintains the same logic
- **Same normalization**: Uses identical bounds and normalization formulas
- **Same output**: Produces the exact same 92-feature vector
- **Same AI logic**: Runs the same mock AI decision-making

## Integration with Unreal Engine

1. **Data Generation**: The Unreal Engine `CombatJsonExporter::MakeCombatStateToJson()` writes `features.json`
2. **Python Processing**: This script reads and processes that file
3. **AI Decision**: The Python script can output decisions that could be fed back to the game

## Example Output

```
=== CombatPolicyFileRunner - Python Version ===
Successfully loaded JSON from features.json at: features.json
Successfully loaded and encoded JSON from features.json
JSON Data Length: 1250 characters
Using Mock AI - LibTorch not integrated (Python version).
AI Health Ratio: 0.80, Enemy Health Ratio: 0.85
Model logits [5]: [-0.2000, 0.1000, 0.8000, -0.3000, 0.4000]
Softmax probs [5]: [0.1500, 0.2000, 0.4500, 0.1000, 0.1000]
Picked action index: 2 (1-based: 3), prob=0.4500
Mock AI decision completed. Model path: MockAI_FromFile_Python
Features processed: 92/92
```

## Requirements

- Python 3.6+
- No external dependencies (uses only standard library)

## Future Enhancements

This Python version can easily be extended to:

- Load and run actual ML models (PyTorch, TensorFlow, etc.)
- Implement more sophisticated AI algorithms
- Connect to external AI services
- Log and analyze decision patterns
- Provide real-time AI responses back to the game