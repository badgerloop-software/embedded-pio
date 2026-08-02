# embedded-pio

Shared PlatformIO libraries for Badger Solar Racing SC2 firmware.

## Contents

| Path | Role |
|------|------|
| `can_ids.h` | Shared CAN IDs (single source of truth) |
| `canmanager/` | STM32 CAN manager |
| `esp32canmanager/` | ESP32 TWAI CAN manager |
| `STM32_CAN/` | Vendored STM32 CAN driver |
| `adc/` `dac/` `PID/` `ina281/` `thermistor/` | Peripheral helpers |
| `tests/` | pytest + python-can contract tests |

## Pin policy

See [PIN_POLICY.md](PIN_POLICY.md). Board repos pin a commit of this submodule; prefer `main` once restructuring lands.

## Tests

Same suite CI runs:

```bash
cd tests
pip install -r requirements.txt
pytest
```

| File | Covers |
|------|--------|
| `test_protocol.py` | IDs and packing from `can_ids.h` |
| `test_boards.py` | Per-board bus behavior |
| `test_vehicle.py` | Multi-board scenarios |

Board compile checks stay in each board repo (`pio run`).
