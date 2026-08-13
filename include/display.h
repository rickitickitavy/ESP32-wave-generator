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
    static const char *dacModeName(DacMode mode);
    static int rowY(int screenIndex);
    static int rowH(int screenIndex);
    static void formatFieldName(FocusField field, char *buf, size_t buflen);
    static void formatFieldValue(const ParamSnapshot &s, FocusField field, char *buf, size_t buflen);
    static void formatSummary(const ParamSnapshot &s, char *buf, size_t buflen);
    static bool fieldChanged(const ParamSnapshot &a, const ParamSnapshot &b, FocusField field);

    void ensureFocusVisible(FocusField focus, int fieldCount, const FocusField *fields);
    void drawDottedSeparator(int y, int width);
    void drawUpLevelIcon(int x, int y, uint16_t color);
    void drawThickLine(int x0, int y0, int x1, int y1, uint16_t color, int thickness);
    void drawBirdCheck(int boxX, int boxY, uint16_t checkFg);
    void drawCheckbox(int rowY, int rowH, bool checked, bool focused, bool editing, uint16_t rowFg);
    void drawFieldRow(int screenIndex, const char *name, const char *value, bool focused,
                      bool editing, bool isBack, bool isCheckbox, bool checked);
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
    static constexpr int kBackIconGap = 15; // tft-ui: icon ↔ BACK label
    static constexpr int kSepMaxItemH = 22;  // tft-ui: separators only if item height < this
    static constexpr int kCheckSize = 14;
    static constexpr int kCheckRightPad = 8; // room for oversized ✓
    static constexpr int kCheckStrokePx = 4;
    // Dark green (RGB565) — edit chrome / Confirm bg role
    static constexpr uint16_t kEditBg = 0x0320;
    // Dark-dark gray (RGB565) for 1px dotted separators
    static constexpr uint16_t kSepColor = 0x2104;
    // Light red (RGB565) for BACK label / icon
    static constexpr uint16_t kBackFg = 0xFD14;
    // Checkbox bird ✓: green unfocused; black when focused or editing
    static constexpr uint16_t kCheckMarkGreen = 0x07E0;
    static constexpr uint16_t kCheckMarkOnFocus = 0x0000;

    SPIClass spiTft_;
    Adafruit_ST7789 tft_;
    ParamSnapshot last_{};
    bool hasLast_ = false;
    int scrollOffset_ = 0;
    int lastScrollOffset_ = -1;
};
