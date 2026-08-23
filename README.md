# FoloToy AI Passport — Matte-Black Digital Badge

[简体中文](README.zh_CN.md) | English

Firmware for the **FoloToy AI Passport** that turns the 240×320 display into a **matte-black digital name badge**. On power-up it shows a clean personal badge — name, title, status, and battery — in a restrained, high-contrast layout, and it can be re-customized over Bluetooth without re-flashing. To save power it deep-sleeps automatically after 3 minutes of inactivity and wakes on any button press.

This repo is a running, hardware-validated baseline. Board-level drivers live in `components/bsp` behind stable APIs; the badge application lives in `main`. See [`docs/AI_HARDWARE_DEVELOPMENT_GUIDE.md`](docs/AI_HARDWARE_DEVELOPMENT_GUIDE.md) for the full hardware context and troubleshooting knowledge.

## Features

- **Matte-black badge UI** with a clear information hierarchy: a large name (24 px, white) as the primary element, smaller title/status (14 px) as secondary, and a single accent color (`0x5B8DEF` soft blue) used sparingly for the status dot and dock selection.
- **Three-button interaction** — `UP`/`DOWN` navigate the bottom dock; `OK` is reserved.
- **BLE customization (NimBLE)** — a phone app can change the name / top bar / title / status live via GATT, no re-flash needed.
- **NVS persistence** — all four customizable fields survive power loss.
- **Deep-sleep power saving** — auto deep-sleep after 3 minutes of no activity, wake on any button press (GPIO0 low-level wake).
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
- Bottom dock: two placeholder tabs; the selected one gets a 3 px accent indicator on top.

## Three-button interaction

| Button | Action |
| --- | --- |
| `UP` | Switch to the previous dock tab |
| `DOWN` | Switch to the next dock tab |
| `OK` | Reserved (short / long press are placeholders; power-off is via the physical switch) |

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
- The bundled font covers the **GB2312 level-1 common characters** to keep Flash small (whole firmware ≈ 2.1 MB). Fields written via BLE that contain characters outside this set will render as missing glyphs. A backup of the original full-coverage fonts is kept in `_font_backup/` (not committed) if you need broader coverage.

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
- `UP`/`DOWN` switch the dock selection (accent indicator follows).
- BLE: phone discovers `FoloToy-Badge`; reading/writing all four characteristics updates UI and survives reboot.
- Battery shows a plausible SOC and degrades to `--%` when CW2017 is absent.
- After 3 minutes idle the badge deep-sleeps; any button wakes it.
- Repeated page transitions / BLE writes do not leak tasks, objects, or heap.

## Project structure

```text
components/bsp/include/  Public BSP APIs + bsp_pins.h hardware facts
components/bsp/src/      Display, button, audio, battery, shared-I2C implementations
main/                    Badge app (main.c, badge.c, ble.c, fonts, avatar)
docs/                    Agent hardware development guide
sdkconfig.defaults       ESP32-C3, USB console, Flash, LVGL defaults
partitions.csv           Custom 4 MB app partition
AGENTS.md                Coding, validation, and contribution rules
```
