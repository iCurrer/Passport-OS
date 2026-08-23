# Repository Guidelines

## Project Structure & Module Organization

This repository is an ESP-IDF BSP and hardware demonstration for the ESP32-C3-based FoloToy AI Passport.

The firmware follows a **three-layer architecture**:

- **Hardware Abstraction Layer** — `components/bsp/`: public BSP APIs and the hardware pin/configuration source of truth (`bsp_pins.h`). Display, button, audio, battery, and shared-I2C drivers live in `src/`.
- **UI Component Library** — `components/ui/`: reusable LVGL drawing primitives (`ui_block`, `ui_label`) that are hardware-agnostic and shared across applications. Pure LVGL, no BSP dependency.
- **Application Layer** — `main/`:
  - `main.c`: entry point: init hardware then `badge_enter()`.
  - `badge/`: badge application, split by responsibility into `badge.c` (facade), `badge_data.c` (NVS field storage), `badge_power.c` (deep-sleep timer), and `badge_ui.c` (LVGL layout).
  - `transport/`: NimBLE GATT service (`ble.c`).
  - `assets/`: static resources (fonts, avatar image).

Dependency direction: `main → ui → bsp`. Keep reusable hardware logic in `components/bsp`; keep reusable UI primitives in `components/ui`; keep application-specific logic in `main`.

## Build, Test, and Development Commands

Use ESP-IDF 5.5.x:

```bash
# Enter ESP-IDF environment (source export.sh or equivalent)
idf.py set-target esp32c3     # Configure a fresh checkout
idf.py build                  # Compile firmware and validate dependencies
idf.py flash monitor          # Flash the connected board and open logs
idf.py fullclean              # Remove generated build state when configuration is stale
```

There is no host-side automated test suite currently. Treat a clean `idf.py build` as the minimum check, then run every applicable item in the README acceptance checklist on real hardware.

## Coding Style & Naming Conventions

Write C using four-space indentation and K&R-style braces, following nearby files. Use `snake_case` for functions and locals, `BSP_*` for public hardware constants, and `s_` for file-local state. Keep BSP APIs prefixed with `bsp_`; name badge submodule entry points `badge_<subsystem>_<action>` (e.g. `badge_data_init`, `badge_ui_set_field`). Prefer `static` for internal symbols. UI text stays English; explanatory comments may be Chinese. Preserve comments documenting hardware-specific register values and memory constraints.

## Testing Guidelines

Before submitting, build from the repository root and inspect warnings. On hardware, verify the badge UI renders correctly and BLE customization works. For pin, display-rotation, codec-clock, ADC, or DMA changes, explicitly record the observed hardware result in the PR. Do not increase LVGL buffers or audio allocations without checking ESP32-C3 internal RAM usage; the board has no PSRAM.

## Known Traps & Build Notes

- **LVGL font format must match the LVGL version.** This project uses LVGL 9.5 (`components/bsp/idf_component.yml` declares `lvgl/lvgl: ^9.5.0`). `lv_font_t.bitmap_format` semantics changed between LVGL 8 and 9.5 (`2` was A4 in LVGL 8 but is `COMPRESSED_NO_PREFILTER` in 9.5). A font generated for LVGL 8 renders **all text blank while layout/images/sound stay normal** — a hard-to-diagnose failure. Regenerate custom fonts with the `lv_font_conv` version matching the target LVGL and confirm `bitmap_format=1` (`COMPRESSED`); always include the ASCII range `0x20-0x7E` or English/digits are missing. Reproducible script: `scripts/gen_badge_fonts.py`.
- **`sdkconfig.defaults` must stay ASCII-only.** Non-UTF-8 bytes (e.g. GBK Chinese comments written by a mismatched editor on a Chinese-locale Windows) make `idf.py` fail reading `sdkconfig` with a `UnicodeDecodeError`. Keep comments ASCII (or English) and strip non-ASCII before committing.
- **Never delete or hand-edit `sdkconfig` as a "fix".** It is gitignored and regenerated from `sdkconfig.defaults`; changes belong in `sdkconfig.defaults`. Deleting it is only safe if `sdkconfig.defaults` fully reproduces the desired config (LVGL, BLE, C++ exceptions/RTTI, partition, console).
- **C++ exceptions/RTTI are required.** XiaoZhi and parts of the badge stack use `dynamic_cast` and `throw`; keep `CONFIG_COMPILER_CXX_EXCEPTIONS=y` and `CONFIG_COMPILER_CXX_RTTI=y`.
- **Reference folders stay out of git.** `xiaozhi-esp32-main/` (a large copy of the XiaoZhi firmware used only as a reference) and `_font_backup/` are ignored and must not be committed.

## Commit & Pull Request Guidelines

History follows Conventional Commit-style subjects such as `feat(bsp): ...`, `feat(badge): ...`, `fix(bsp): ...`, and `docs: ...`. Keep commits focused by subsystem. Pull requests should explain the hardware/revision tested, summarize behavior changes, list build and on-device results, and include photos or screenshots for display changes. Link related issues and call out wiring, pin-map, or compatibility impacts.
