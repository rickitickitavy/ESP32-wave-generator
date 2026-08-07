#pragma once

// Dual DAC outputs (ESP32 built-in 8-bit DAC).
// Avoid PIN_DAC1/PIN_DAC2 names — those are macros in esp32-hal-gpio.h.
constexpr int PIN_DAC_CH1 = 25;
constexpr int PIN_DAC_CH2 = 26;

// Rotary encoder (phaseA / phaseB / button)
constexpr int PIN_ENCODER_A = 33;
constexpr int PIN_ENCODER_B = 32;
// GPIO34 is input-only and has no internal pull-up — use an external pull-up.
constexpr int PIN_ENCODER_BTN = 34;

// ST7789 SPI — VSPI defaults (MOSI/SCLK/MISO/CS); no backlight pin
constexpr int PIN_TFT_MOSI = 23;
constexpr int PIN_TFT_SCLK = 18;
constexpr int PIN_TFT_MISO = 19;
constexpr int PIN_TFT_CS = 4;
constexpr int PIN_TFT_DC = 14;
constexpr int PIN_TFT_RST = 13;

// Physical panel (240x320; UI uses setRotation(2) = 180°)
constexpr int TFT_WIDTH = 240;
constexpr int TFT_HEIGHT = 320;
