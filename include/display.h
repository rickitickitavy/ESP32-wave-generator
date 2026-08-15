#pragma once

#include <Adafruit_ST7789.h>
#include <Arduino.h>
#include <SPI.h>

#include "param_model.h"

struct WavePlotSamples {
    const uint8_t *ch1 = nullptr;
    const uint8_t *ch2 = nullptr;
    int count = 0;
};

class Display {
public:
    Display();

    void begin();
    // plot: CH1/CH2 one-period samples for Signal/PWM submenus; ignored on Top.
    void render(const ParamSnapshot &state, const WavePlotSamples &plot);

    // Digital PWM one-period high/low samples (0 or 255) from pulse µs / period.
    static void fillPwmPeriodPreview(const ParamSnapshot &params, uint8_t *ch1, uint8_t *ch2,
                                     int count);

    static constexpr int kPlotSampleCount = 228; // width − 2×pad − border

private:
    static const char *waveformName(Waveform w);
    static const char *dacModeName(DacMode mode);
    static bool showsPlot(MenuLevel menu);
    static int menuHeight(MenuLevel menu);
    static int visibleFieldRows(MenuLevel menu);
    static int rowY(int screenIndex, int menuH, int visibleRows);
    static int rowH(int screenIndex, int menuH, int visibleRows);
    static void formatFieldName(FocusField field, char *buf, size_t buflen);
    static void formatFieldValue(const ParamSnapshot &s, FocusField field, char *buf, size_t buflen);
    static bool fieldChanged(const ParamSnapshot &a, const ParamSnapshot &b, FocusField field);
    static bool plotParamsChanged(const ParamSnapshot &a, const ParamSnapshot &b);

    void ensureFocusVisible(FocusField focus, int fieldCount, const FocusField *fields,
                            int visibleRows);
    void drawDottedSeparator(int y, int width);
    void drawUpLevelIcon(int x, int y, uint16_t color);
    void drawThickLine(int x0, int y0, int x1, int y1, uint16_t color, int thickness);
    void drawBirdCheck(int boxX, int boxY, uint16_t checkFg);
    void drawCheckbox(int rowY, int rowH, bool checked, bool focused, bool editing, uint16_t rowFg);
    void drawFieldRow(int screenIndex, int menuH, int visibleRows, const char *name,
                      const char *value, bool focused, bool editing, bool isBack, bool isCheckbox,
                      bool checked);
    void drawWavePlot(const uint8_t *ch1, const uint8_t *ch2, int count, bool sampleHold);

    static constexpr int kLogicalW = 240;
    static constexpr int kLogicalH = 320;
    static constexpr int kPlotH = 80; // bottom 1/4
    static constexpr int kMenuHWithPlot = kLogicalH - kPlotH;
    // Target row height; pack max rows that still fit, then stretch bands to fill menu area.
    static constexpr int kRowH = 26;
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
    static constexpr uint16_t kPlotCh1Color = 0x07FF; // cyan
    static constexpr uint16_t kPlotCh2Color = 0xFFE0; // yellow
    static constexpr uint16_t kPlotMidColor = 0x4208; // dark gray midline
    static constexpr uint16_t kPlotBorderColor = 0x8410; // medium gray

    SPIClass spiTft_;
    Adafruit_ST7789 tft_;
    ParamSnapshot last_{};
    bool hasLast_ = false;
    int scrollOffset_ = 0;
    int lastScrollOffset_ = -1;
    bool lastShowedPlot_ = false;
};
