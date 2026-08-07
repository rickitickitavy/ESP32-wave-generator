# AGENTS.md — esp32-Sound-array

ESP32-WROOM-32 dual-channel DAC signal generator (PlatformIO / Arduino). CLion-friendly PlatformIO project.

## Before changing firmware

1. Read and follow [`~/.cursor/skills/esp32/SKILL.md`](/home/dsporynkhin/.cursor/skills/esp32/SKILL.md).
2. For SPI or callbacks, also read [`~/.cursor/skills/esp32/patterns.md`](/home/dsporynkhin/.cursor/skills/esp32/patterns.md).

## Always-on project rules

Enforced via [`.cursor/rules/`](.cursor/rules/):

| Rule | Meaning |
|------|---------|
| `esp32.mdc` | Apply the esp32 skill before firmware edits |
| `no-font-upscale.mdc` | No `setTextSize(n≠1)`; use sized GFXfonts; draw bitmaps 1:1 |
| `no-self-instance-ref.mdc` | No ad-hoc `static Foo *instance` in class files; wire callbacks from `main.cpp` |
| `ask-before-pins.mdc` | **Always ask** before changing any GPIO / pin assignments |

## Stack

- Board: `esp32dev` (ESP32-WROOM-32) — see [`platformio.ini`](platformio.ini)
- Pins: [`include/pins.h`](include/pins.h)
- Display: ST7789 240×320, `setRotation(2)` (180°), Adafruit GFX
- Outputs: DAC ch1 GPIO25, DAC ch2 GPIO26 (`PIN_DAC_CH1` / `PIN_DAC_CH2`; same waveform / freq / amplitude; ch2 phase offset)

## Hardware notes

- **GPIO34** (encoder button) is input-only and has **no internal pull-up** — wire an external pull-up to 3.3 V.
- TFT uses VSPI defaults: MOSI=23, SCLK=18, MISO=19, CS=5; DC=13, RST=12. No backlight pin.
- Encoder: A=33, B=32, button=34 (polled in `loop`; button needs external pull-up on GPIO34).

## Build / flash

```bash
pio run
pio run -t upload
pio device monitor     # 115200
```

## Hard constraints (do not violate)

- **UI:** `setTextSize(1)` with GFXfonts; no bitmap upscaling at draw time; prefer partial TFT redraws.
- **Callbacks:** register handlers from owners (e.g. `main.cpp`), not fake class singletons.
- **Channels:** type, frequency, and amplitude are always equal on both DACs; only phase (ch2 vs ch1) differs.

## Key sources

| Area | Path |
|------|------|
| Setup / loop / ISR trampolines | `src/main.cpp` |
| Encoder class | `src/encoder.cpp`, `include/encoder.h` |
| ParamModel class | `src/param_model.cpp`, `include/param_model.h` |
| SignalGenerator class | `src/signal_generator.cpp`, `include/signal_generator.h` |
| Display class | `src/display.cpp`, `include/display.h` |
| Pins | `include/pins.h` |

## Working style

- Prefer small, focused diffs; match existing naming and patterns.
- Do not commit unless the user asks.
