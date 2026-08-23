# FoloToy AI Passport — Matte-Black Digital Badge

[简体中文](README.zh_CN.md) | English

Firmware for the **FoloToy AI Passport** that turns the 240×320 display into a **matte-black digital name badge**. On power-up it shows a clean personal badge — name, title, status, and battery — in a restrained, high-contrast layout, and it can be re-customized over Bluetooth without re-flashing. A bottom dock launches a built-in **pixel game** and a **settings page** (sleep timer, Bluetooth on/off, firmware version). To save power it auto deep-sleeps after a configurable idle timeout (default 7 minutes) and wakes on any button press.

This repo is a running, hardware-validated baseline. The firmware follows a clean **three-layer architecture**: hardware abstraction (`components/bsp`), reusable UI primitives (`components/ui`), and application logic (`main/` — `badge/`, `game/`, `settings/`, `transport/`, `assets/`). See [`docs/AI_HARDWARE_DEVELOPMENT_GUIDE.md`](docs/AI_HARDWARE_DEVELOPMENT_GUIDE.md) for the full hardware context and troubleshooting knowledge.

## Features

- **Matte-black badge UI** with a clear information hierarchy: a large name (24 px, white) as the primary element, smaller title/status (14 px) as secondary, and a single accent color (`0x4CD964` green) used sparingly for the status dot and dock selection.
- **Three-button interaction** — `UP`/`DOWN` navigate the bottom dock; `OK` (short) opens the selected page, `OK` (long) returns to the badge.
- **Pixel game** — a "catch the gems, dodge the bombs" minigame in the dock; `UP`/`DOWN` move the basket, difficulty ramps up as the score climbs.
- **Settings page** — sleep timeout (30 s / 1 / 2 / 5 min / never), Bluetooth on/off (persisted to NVS), and firmware version.
- **BLE customization (NimBLE)** — a phone app can change the name / top bar / title / status live via GATT, no re-flash needed.
- **NVS persistence** — all four customizable fields, the sleep timeout, and the Bluetooth state survive power loss.
- **Deep-sleep power saving** — auto deep-sleep after a configurable idle timeout (default 7 min), wake on any button press (GPIO0 low-level wake).
- **Battery display** — CW2017 state-of-charge, turns orange below 20% and red below 10%; degrades to `--%` when the fuel gauge is absent.

## On-screen layout

```text
┌─────────────────────────────┐
│ FoloToy            ▯ 96%    │  top bar: brand + battery
├─────────────────────────────┤
│             ┌──────────┐    │
│             │          │    │
│  [avatar]   │ Zhang San│    │  name  (24 px, white, primary)
│             │ ──────── │    │
│             │ 豆包大学  │    │  title (14 px, gray)
│             │ ● 自由    │    │  status (14 px, accent + dot)
│             └──────────┘    │
├─────────────────────────────┤
│      ▮        │              │  bottom dock (UP/DOWN to switch)
└─────────────────────────────┘
```

- Header: brand (left) + battery bar and percent (right), thin divider below.
- Body: pixel avatar on the left, vertically centered against an information column on the right — all left-aligned on one reference line.
- Bottom dock: two live entries (game icon + settings icon); the selected one gets a 3 px accent indicator on top.

## Three-button interaction

| Button | Badge (home) | Game | Settings |
| --- | --- | --- | --- |
| `UP` / `DOWN` | Switch dock selection | Move the basket left / right | Move selection up / down |
| `OK` (short press) | Open the selected dock page | Reserved (future skill) | Change the selected item |
| `OK` (long press) | — | Return to badge | Return to badge |

## Dock pages

### Pixel game

A "catch the gems, dodge the bombs" minigame. Move the basket at the bottom with `UP`/`DOWN` to catch falling gems (green +10, blue +20, gold +50) while avoiding red bombs (−1 life). You start with 3 lives; the fall speed ramps up every 30 points. When lives run out, `OK` (short) restarts the badge view. Long-press `OK` quits back to the badge at any time.

### Settings

A list-style page: **sleep timeout** (30 s / 1 min / 2 min / 5 min / never), **Bluetooth on/off**, and **firmware version**. `UP`/`DOWN` move the selection, `OK` (short) toggles or cycles the selected value. The sleep timeout and Bluetooth state are persisted to NVS and survive reboot. Long-press `OK` returns to the badge.

## BLE customization (NimBLE)

- **Advertising name:** `FoloToy-Badge`
- **GATT service:** `0xFFE0`
- **Characteristics** (read + write), 16-bit UUIDs:

| UUID | Field | NVS key |
| --- | --- | --- |
| `0xFFE1` | name | `name` |
| `0xFFE2` | top bar text | `top` |
| `0xFFE3` | title | `title` |
| `0xFFE4` | status | `status` |

Writing a characteristic updates the LVGL label and persists to NVS. The Android-side UUID is `0000FFEx-0000-1000-8000-00805F9B34FB`.

## Hardware capability contract (BSP)

| Capability | Confirmed implementation | Application interface | Boundaries |
| --- | --- | --- | --- |
| Display | ST7789P3, 240 × 320 portrait RGB565, SPI2 @ 40 MHz; LEDC backlight | `bsp_display_*`, `bsp_lvgl_*` | ESP32-C3 has no PSRAM; single small DMA buffer; no MISO/touch/TE |
| Input | `UP`/`DOWN`/`OK` share an ADC resistor ladder on GPIO0 | `bsp_button_init()`, `bsp_button_read_mv()` | Callbacks run in the button task; must not block |
| Audio | ES8311, full-duplex PCM over I2S0 (play + record) | `bsp_audio_*` | PCM I/O is blocking → worker task; format change must close/reopen |
| Battery | CW2017 SOC + voltage | `bsp_battery_*` | Optional at runtime; accuracy depends on cell/profile |
| Shared bus | ES8311 & CW2017 share I2C0 | `bsp_i2c_*` | Reuse the BSP-owned bus; never create a second bus on the port |
| Logging/flash | ESP32-C3 native USB Serial/JTAG | ESP-IDF console | GPIO18/19 for USB; UART0 TX on GPIO21 conflicts with backlight |

All pins, addresses, panel parameters, and button voltage windows live only in [`components/bsp/include/bsp_pins.h`](components/bsp/include/bsp_pins.h).

## Chinese font note

- Name uses the 24 px full Chinese font; title/status/top bar use the 14 px full Chinese font.
- The fonts (`main/assets/badge_font_gb2312.c`, `main/assets/badge_font_gb2312_small.c`) are generated for **LVGL 9.5** with `lv_font_conv` and cover **GB2312 (6763 common chars) + ASCII `0x20-0x7E`**. Fields written via BLE that use characters outside this set will render as missing glyphs.
- ⚠️ LVGL 8 vs 9.5 changed `lv_font_t.bitmap_format` meaning; a font generated for LVGL 8 will show **all text blank** (layout/images/sound still fine). Regenerate with the matching `lv_font_conv` (`scripts/gen_badge_fonts.py`) and confirm `bitmap_format=1` (`COMPRESSED`) and that the ASCII range `0x20-0x7E` is included.
- The original pre-regeneration font source lives in `_font_backup/` (not committed).

## Build & flash

ESP-IDF 5.5.x (known env 5.5.3):

```bash
idf.py set-target esp32c3     # fresh checkout only
idf.py build
idf.py -p COM5 flash          # replace COM5 with your port
idf.py -p COM5 monitor
```

Custom partition table (`partitions.csv`) enlarges the app partition to 4 MB to hold the Chinese fonts.

## Acceptance checklist (on device)

- Stable startup logs via USB Serial/JTAG; no reboot loop or watchdog reset.
- Display orientation, colors, edges, and backlight correct; accent color renders correctly.
- `UP`/`DOWN` switch the dock selection (accent indicator follows); `OK` opens the selected page and long-press `OK` returns.
- Pixel game: basket moves, gems/bombs score correctly, and Game Over / quit both return to the badge cleanly.
- Settings: sleep timeout and Bluetooth toggle take effect and survive reboot; version string renders.
- BLE: phone discovers `FoloToy-Badge`; reading/writing all four characteristics updates UI and survives reboot.
- Battery shows a plausible SOC and degrades to `--%` when CW2017 is absent.
- After the configured idle timeout (default 7 min) the badge deep-sleeps; any button wakes it.
- Repeated page transitions / BLE writes do not leak tasks, objects, or heap.

## Project structure

```text
components/bsp/                  Hardware abstraction layer (BSP)
├── include/bsp_pins.h            Single source of truth for pins, addresses, panel params
├── include/bsp_display.h         Display & LVGL port API
├── include/bsp_button.h          ADC three-button API
├── include/bsp_audio.h           ES8311 codec API
├── include/bsp_battery.h         CW2017 fuel gauge API
├── include/bsp_i2c.h             Shared I2C0 bus API
└── src/                          Driver implementations

components/ui/                    Reusable UI primitive library (shared across apps)
├── include/ui_pixel.h            ui_block(), ui_label(), ui_header_bar(), ui_battery_pct()
└── src/ui_pixel.c

main/                             Application layer
├── main.c                        Entry point: init hardware → badge_enter()
├── badge/                        Badge application (modular, layered)
│   ├── badge.h                   Public API + badge_sub_t (sub-page routing state)
│   ├── badge.c                   Facade: orchestrates submodules, routes keys to game/settings
│   ├── badge_data.h / .c          Data model: NVS-backed field storage (name/top/title/status)
│   ├── badge_power.h / .c         Power: configurable deep-sleep timer, wake-on-button
│   ├── badge_ui.h / .c            LVGL layout: header bar, dock, field refresh
│   └── badge_theme.h             Shared color / layout constants (theme)
├── game/                         Pixel game page
│   └── game.h / .c               "Catch the gems" minigame (enter / exit / key)
├── settings/                     Settings page
│   └── settings.h / .c           Sleep timeout, Bluetooth toggle, version (enter / exit / key)
├── transport/                    Communication layer
│   └── ble.h / .c                NimBLE GATT service (advertising + read/write characteristics)
├── assets/                       Static resources
│   ├── badge_fonts.h             Font declarations
│   ├── badge_font_gb2312.c        24 px Chinese font (GB2312)
│   ├── badge_font_gb2312_small.c  14 px Chinese font (GB2312)
│   └── badge_avatar.h / .c        Pixel avatar image data
└── CMakeLists.txt                Build: sources, include dirs, component dependencies

sdkconfig.defaults                 ESP32-C3, USB console, Flash, LVGL defaults
partitions.csv                     Custom 4 MB app partition
AGENTS.md                          Coding, validation, and contribution rules
docs/                              AI hardware development guide
```

### Architecture & dependency direction

```
┌──────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
│  main.c ──► badge/ (facade → data, power, ui, theme)        │
│              ├─► game/ (pixel game)   └─► settings/          │
│              transport/ble.c (NimBLE GATT)                   │
├──────────────────────────────────────────────────────────────┤
│                    UI Component Library                      │
│  components/ui/ (ui_block, ui_label, ui_header_bar,          │
│                  ui_battery_pct — pure LVGL)                 │
├──────────────────────────────────────────────────────────────┤
│                Hardware Abstraction Layer                    │
│  components/bsp/ (display, button, audio, battery, i2c)      │
└──────────────────────────────────────────────────────────────┘
```

Dependency direction: `main → ui → bsp`. The `ui` layer is pure LVGL and knows nothing about hardware. The `badge` submodule is split by responsibility: `data` (NVS storage), `power` (sleep timer), `ui` (LVGL layout), and `theme` (shared constants) — all coordinated by the `badge.c` facade. Dock pages (`game/`, `settings/`) implement the `enter` / `exit` / `key` interface, are launched via `badge_ui_dock_enter()`, and reuse `components/ui` for drawing. New applications can likewise reuse `components/ui` without duplicating pixel primitives.
