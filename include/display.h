#pragma once

#include <Adafruit_ST7789.h>
#include <Arduino.h>
#include <SPI.h>

#include "param_model.h"

class Display {
public:
    Display();

    void begin();
    void render(const ParamSnapshot &state);

private:
    static const char *waveformName(Waveform w);
    static int rowY(int screenIndex);
    static int rowH(int screenIndex);
    static void formatFieldName(FocusField field, char *buf, size_t buflen);
    static void formatFieldValue(const ParamSnapshot &s, FocusField field, char *buf, size_t buflen);
    static void formatSummary(const ParamSnapshot &s, char *buf, size_t buflen);
    static bool fieldChanged(const ParamSnapshot &a, const ParamSnapshot &b, FocusField field);

    void ensureFocusVisible(FocusField focus);
    void drawFieldRow(int screenIndex, const char *name, const char *value, bool focused,
                      bool editing);
    void drawSummaryRow(const char *text);

    static constexpr int kLogicalW = 240;
    static constexpr int kLogicalH = 320;
    static constexpr int kFieldCount = static_cast<int>(FocusField::Count);
    // Target row height; pack max rows that still fit, then stretch bands to fill TFT.
    static constexpr int kRowH = 26;
    static constexpr int kVisibleRows = kLogicalH / kRowH;
    static constexpr int kVisibleFieldRows = kVisibleRows - 1; // last band = pinned summary
    static constexpr int kSummaryScreenRow = kVisibleFieldRows;
    static constexpr int kPadX = 6;
    // Dark green (RGB565): R≈0, G≈96, B≈0
    static constexpr uint16_t kEditBg = 0x0320;

    SPIClass spiTft_;
    Adafruit_ST7789 tft_;
    ParamSnapshot last_{};
    bool hasLast_ = false;
    int scrollOffset_ = 0;
    int lastScrollOffset_ = -1;
};
