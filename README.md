# embedded-pio

Shared PlatformIO libraries for Badgerloop SC2 firmware.

## What's here

| Path | Role |
|------|------|
| `can_ids.h` | Shared CAN IDs (single source of truth) |
| `canmanager/` / `esp32canmanager/` | Hardware CAN managers |
| `STM32_CAN/` | Vendored STM32 CAN driver (optional; boards may use `lib_deps`) |
| `adc/` `dac/` `PID/` `ina281/` `thermistor/` | Peripheral helpers |
| `tests/` | pytest + python-can contract tests (no g++ required) |

## Pin policy

See [PIN_POLICY.md](PIN_POLICY.md). Consumers track `main`.

## Tests (local, same as CI)

```bash
cd tests
pip install -r requirements.txt
pytest
```

- `test_protocol.py` — ID/packing contracts from `can_ids.h`
- `test_boards.py` — per-board bus behavior
- `test_vehicle.py` — multi-board scenario

Firmware compile checks stay in each board repo (`pio run`).
