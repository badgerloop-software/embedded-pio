# embedded-pio pin policy

SC2 board repos pin this submodule to a known commit. After restructuring merges, that tip should be `main`.

Do not point consumers at long-lived fork branches. Land shared changes here, then bump the submodule SHA in every board that uses them:

- `sc2-powertrain`
- `sc2-pdc`
- `sc2-steering-wheel`
- `sc2-mppt`
- `sc2-lighting`
- `sc2-template` (starter for new boards)

`STM32_CAN` is vendored under `STM32_CAN/`. Prefer that path (or Registry `pazi88/STM32_CAN`), not both in one project.
