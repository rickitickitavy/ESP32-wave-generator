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

    void ensureFocusVisible(FocusField focus, int fieldCount, const FocusField *fields);
    void drawDottedSeparator(int y, int width);
    void drawUpLevelIcon(int x, int y, uint16_t color);
    void drawFieldRow(int screenIndex, const char *name, const char *value, bool focused,
                      bool editing, bool isBack);
    void drawSummaryRow(const char *text);

    static constexpr int kLogicalW = 240;
    static constexpr int kLogicalH = 320;
    // Target row height; pack max rows that still fit, then stretch bands to fill TFT.
    static constexpr int kRowH = 26;
    static constexpr int kVisibleRows = kLogicalH / kRowH;
    static constexpr int kVisibleFieldRows = kVisibleRows - 1; // last band = pinned summary
    static constexpr int kSummaryScreenRow = kVisibleFieldRows;
    static constexpr int kPadX = 6;
    static constexpr int kBackIconW = 12;
    static constexpr int kBackIconH = 12;
    static constexpr int kBackIconGap = 4;
    // Dark green (RGB565): R≈0, G≈96, B≈0
    static constexpr uint16_t kEditBg = 0x0320;
    // Medium gray (RGB565) for 1px dotted separators
    static constexpr uint16_t kSepColor = 0x8410;
    // Light red (RGB565) for Back label / icon
    static constexpr uint16_t kBackFg = 0xFD14;

    SPIClass spiTft_;
    Adafruit_ST7789 tft_;
    ParamSnapshot last_{};
    bool hasLast_ = false;
    int scrollOffset_ = 0;
    int lastScrollOffset_ = -1;
};
