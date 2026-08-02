# embedded-pio pin policy

All SC2 board repositories must pin this submodule to a commit on **`main`**.

Do not track long-lived fork branches (`STM32_CAN`, `ESP32-CAN`, etc.) from consumer repos.
Merge hardware-specific needs into `main` instead.

When publishing a breaking change:

1. Land it on `embedded-pio` `main` with green self-tests and consumer matrix CI.
2. Bump the submodule SHA in all five consumers in the same change window:
   `sc2-powertrain`, `sc2-pdc`, `sc2-steering-wheel`, `sc2-mppt`, `sc2-lighting`.

`STM32_CAN` is vendored under `STM32_CAN/` for boards that do not pull it via `lib_deps`.
Boards may still declare `pazi88/STM32_CAN` in `lib_deps`; prefer one source per project to avoid duplicate symbols.
