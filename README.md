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
| `test_smoke_car.py` | Drive / lights / BPS contracts that must not break before shop testing |

Board compile checks stay in each board repo (`pio run`).

## CI notes (private org repos)

Contract tests (`pytest`) only need this repo.

The consumer matrix builds each private `sc2-*` board against this commit. That requires
read access to those repos:

1. **Preferred:** org or repo secret `SC2_CI_TOKEN` — a fine-grained PAT with
   **Contents: Read** on `sc2-powertrain`, `sc2-pdc`, `sc2-steering-wheel`,
   `sc2-mppt`, and `sc2-lighting`.
2. **Or:** GitHub org → Settings → Actions → General → enable access to other
   private repositories in the org (so `GITHUB_TOKEN` can clone them).

Without one of those, the matrix clone step fails with an access error even for
org members running CI — the default job token only covers `embedded-pio`.
