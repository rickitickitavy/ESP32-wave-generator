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
    static int rowY(int index);
    static int rowH(int index);
    static void formatField(const ParamSnapshot &s, FocusField field, char *buf, size_t buflen);
    static void formatSummary(const ParamSnapshot &s, char *buf, size_t buflen);
    static bool fieldChanged(const ParamSnapshot &a, const ParamSnapshot &b, FocusField field);

    void drawRow(int index, const char *text, bool focused);

    static constexpr int kLogicalW = 240;
    static constexpr int kLogicalH = 320;
    static constexpr int kRowCount = static_cast<int>(FocusField::Count) + 1; // + summary
    static constexpr int kSummaryRow = static_cast<int>(FocusField::Count);

    SPIClass spiTft_;
    Adafruit_ST7789 tft_;
    ParamSnapshot last_{};
    bool hasLast_ = false;
};
