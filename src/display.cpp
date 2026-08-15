#include "display.h"

#include <Adafruit_GFX.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <cstdio>

#include "pins.h"

namespace {
    // Native 12×12 "up level" glyph (MSB-first, 2 bytes/row). Drawn 1:1.
    const uint8_t kUpLevelIcon[] PROGMEM = {
            0x06, 0x00, // ....##......
            0x0F, 0x00, // ...####.....
            0x1F, 0x80, // ..######....
            0x36, 0xC0, // .##..##.##..
            0x66, 0x60, // ##...##..##.
            0xC6, 0x30, // ##...##...##
            0x06, 0x00, // ....##......
            0x06, 0x00, // ....##......
            0x06, 0x00, // ....##......
            0x06, 0x00, // ....##......
            0x06, 0x00, // ....##......
            0x06, 0x00, // ....##......
    };
} // namespace

Display::Display()
    : spiTft_(VSPI), tft_(&spiTft_, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST) {}

const char *Display::waveformName(Waveform w) {
    switch (w) {
        case Waveform::Rectangular:
            return "Rect";
        case Waveform::Triangle:
            return "Tri";
        case Waveform::Sine:
        default:
            return "Sine";
    }
}

const char *Display::dacModeName(DacMode mode) {
    switch (mode) {
        case DacMode::AnalogPwm:
            return "AnPWM";
        case DacMode::Oscillator:
        default:
            return "Osc";
    }
}

bool Display::showsPlot(MenuLevel menu) {
    return menu == MenuLevel::Signal || menu == MenuLevel::SigFreq || menu == MenuLevel::SigPhase ||
           menu == MenuLevel::SigShiftUs || menu == MenuLevel::SigPulse ||
           menu == MenuLevel::SigDuty || menu == MenuLevel::Pwm;
}

int Display::menuHeight(MenuLevel menu) {
    return showsPlot(menu) ? kMenuHWithPlot : kLogicalH;
}

int Display::visibleFieldRows(MenuLevel menu) {
    return menuHeight(menu) / kRowH;
}

int Display::rowY(int screenIndex, int menuH, int visibleRows) {
    if (visibleRows <= 0) {
        return 0;
    }
    return (screenIndex * menuH) / visibleRows;
}

int Display::rowH(int screenIndex, int menuH, int visibleRows) {
    return rowY(screenIndex + 1, menuH, visibleRows) - rowY(screenIndex, menuH, visibleRows);
}

void Display::formatFieldName(FocusField field, char *buf, size_t buflen) {
    switch (field) {
        case FocusField::GroupSignal:
            snprintf(buf, buflen, "Signal generator");
            break;
        case FocusField::GroupPwm:
            snprintf(buf, buflen, "PWM");
            break;
        case FocusField::GroupFreq:
            snprintf(buf, buflen, "Frequency");
            break;
        case FocusField::GroupPhase:
            snprintf(buf, buflen, "Phase");
            break;
        case FocusField::GroupShiftUs:
            snprintf(buf, buflen, "Shift us");
            break;
        case FocusField::GroupPulse:
            snprintf(buf, buflen, "Pulse us");
            break;
        case FocusField::GroupDuty:
            snprintf(buf, buflen, "Duty %%");
            break;
        case FocusField::SigEnabled:
        case FocusField::PwmEnabled:
            snprintf(buf, buflen, "Enabled");
            break;
        case FocusField::SigMode:
            snprintf(buf, buflen, "Mode");
            break;
        case FocusField::SigBack:
        case FocusField::FreqBack:
        case FocusField::PhaseBack:
        case FocusField::ShiftUsBack:
        case FocusField::PulseBack:
        case FocusField::DutyBack:
        case FocusField::PwmBack:
            snprintf(buf, buflen, "BACK");
            break;
        case FocusField::Waveform:
            snprintf(buf, buflen, "Wave");
            break;
        case FocusField::Amplitude:
            snprintf(buf, buflen, "Amp V");
            break;
        case FocusField::FreqKHz:
            snprintf(buf, buflen, "F kHz");
            break;
        case FocusField::FreqHundredHz:
            snprintf(buf, buflen, "F 100Hz");
            break;
        case FocusField::FreqTensHz:
            snprintf(buf, buflen, "F 10Hz");
            break;
        case FocusField::FreqHz:
            snprintf(buf, buflen, "F Hz");
            break;
        case FocusField::PhaseTens:
            snprintf(buf, buflen, "Ph x10");
            break;
        case FocusField::PhaseDeg:
            snprintf(buf, buflen, "Ph deg");
            break;
        case FocusField::PhaseFine:
            snprintf(buf, buflen, "Ph .1");
            break;
        case FocusField::PhaseUs1000:
            snprintf(buf, buflen, "Ph 1ms");
            break;
        case FocusField::PhaseUs100:
            snprintf(buf, buflen, "Ph 100us");
            break;
        case FocusField::PhaseUs10:
            snprintf(buf, buflen, "Ph 10us");
            break;
        case FocusField::PhaseUs1:
            snprintf(buf, buflen, "Ph 1us");
            break;
        case FocusField::PulseUs100:
            snprintf(buf, buflen, "Pw 100us");
            break;
        case FocusField::PulseUs10:
            snprintf(buf, buflen, "Pw 10us");
            break;
        case FocusField::PulseUs1:
            snprintf(buf, buflen, "Pw 1us");
            break;
        case FocusField::Duty10:
            snprintf(buf, buflen, "Duty 10");
            break;
        case FocusField::Duty1:
            snprintf(buf, buflen, "Duty 1");
            break;
        case FocusField::DutyTenths:
            snprintf(buf, buflen, "Duty .1");
            break;
        case FocusField::PwmFreq:
            snprintf(buf, buflen, "PWM x10Hz");
            break;
        case FocusField::PwmCh1X20:
            snprintf(buf, buflen, "P1 x20us");
            break;
        case FocusField::PwmCh1X1:
            snprintf(buf, buflen, "P1 x1us");
            break;
        case FocusField::PwmCh2X20:
            snprintf(buf, buflen, "P2 x20us");
            break;
        case FocusField::PwmCh2X1:
            snprintf(buf, buflen, "P2 x1us");
            break;
        case FocusField::Count:
            buf[0] = '\0';
            break;
    }
}

void Display::formatFieldValue(const ParamSnapshot &s, FocusField field, char *buf, size_t buflen) {
    switch (field) {
        case FocusField::GroupSignal:
        case FocusField::GroupPwm:
            snprintf(buf, buflen, ">");
            break;
        case FocusField::GroupFreq:
            snprintf(buf, buflen, "%.1f", static_cast<double>(s.freqHz));
            break;
        case FocusField::GroupPhase:
            snprintf(buf, buflen, "%+.1f", static_cast<double>(s.phaseDegTotal));
            break;
        case FocusField::GroupShiftUs:
            snprintf(buf, buflen, "%+d", s.phaseShiftUs);
            break;
        case FocusField::GroupPulse:
            snprintf(buf, buflen, "%d", s.pulseUs);
            break;
        case FocusField::GroupDuty:
            snprintf(buf, buflen, "%.1f%%", static_cast<double>(s.dutyPercent));
            break;
        case FocusField::SigBack:
        case FocusField::FreqBack:
        case FocusField::PhaseBack:
        case FocusField::ShiftUsBack:
        case FocusField::PulseBack:
        case FocusField::DutyBack:
        case FocusField::PwmBack:
            buf[0] = '\0';
            break;
        case FocusField::SigEnabled:
        case FocusField::PwmEnabled:
            // Value drawn as checkbox, not ON/OFF text.
            buf[0] = '\0';
            break;
        case FocusField::SigMode:
            snprintf(buf, buflen, "%s", dacModeName(s.dacMode));
            break;
        case FocusField::Waveform:
            snprintf(buf, buflen, "%s", waveformName(s.waveform));
            break;
        case FocusField::Amplitude:
            snprintf(buf, buflen, "%.1f", static_cast<double>(s.ampVolts));
            break;
        case FocusField::FreqKHz:
            snprintf(buf, buflen, "%d", s.freqKHz);
            break;
        case FocusField::FreqHundredHz:
            snprintf(buf, buflen, "%d", s.freqHundredHz);
            break;
        case FocusField::FreqTensHz:
            snprintf(buf, buflen, "%d", s.freqTensHz);
            break;
        case FocusField::FreqHz:
            snprintf(buf, buflen, "%.1f", static_cast<double>(s.freqHzPart));
            break;
        case FocusField::PhaseTens:
            snprintf(buf, buflen, "%d", s.phaseTens);
            break;
        case FocusField::PhaseDeg:
            snprintf(buf, buflen, "%d", s.phaseDeg);
            break;
        case FocusField::PhaseFine:
            snprintf(buf, buflen, "%+.1f", static_cast<double>(s.phaseFine));
            break;
        case FocusField::PhaseUs1000:
            snprintf(buf, buflen, "%+d", s.phaseUs1000);
            break;
        case FocusField::PhaseUs100:
            snprintf(buf, buflen, "%+d", s.phaseUs100);
            break;
        case FocusField::PhaseUs10:
            snprintf(buf, buflen, "%+d", s.phaseUs10);
            break;
        case FocusField::PhaseUs1:
            snprintf(buf, buflen, "%+d", s.phaseUs1);
            break;
        case FocusField::PulseUs100:
            snprintf(buf, buflen, "%d", s.pulseUs100);
            break;
        case FocusField::PulseUs10:
            snprintf(buf, buflen, "%d", s.pulseUs10);
            break;
        case FocusField::PulseUs1:
            snprintf(buf, buflen, "%d", s.pulseUs1);
            break;
        case FocusField::Duty10:
            snprintf(buf, buflen, "%d", s.duty10);
            break;
        case FocusField::Duty1:
            snprintf(buf, buflen, "%d", s.duty1);
            break;
        case FocusField::DutyTenths:
            snprintf(buf, buflen, "%d", s.dutyTenths);
            break;
        case FocusField::PwmFreq:
            snprintf(buf, buflen, "%d", s.pwmFreqX10);
            break;
        case FocusField::PwmCh1X20:
            snprintf(buf, buflen, "%d", s.pwmCh1X20);
            break;
        case FocusField::PwmCh1X1:
            snprintf(buf, buflen, "%d", s.pwmCh1X1);
            break;
        case FocusField::PwmCh2X20:
            snprintf(buf, buflen, "%d", s.pwmCh2X20);
            break;
        case FocusField::PwmCh2X1:
            snprintf(buf, buflen, "%d", s.pwmCh2X1);
            break;
        case FocusField::Count:
            buf[0] = '\0';
            break;
    }
}

bool Display::fieldChanged(const ParamSnapshot &a, const ParamSnapshot &b, FocusField field) {
    switch (field) {
        case FocusField::GroupSignal:
        case FocusField::GroupPwm:
        case FocusField::SigBack:
        case FocusField::FreqBack:
        case FocusField::PhaseBack:
        case FocusField::ShiftUsBack:
        case FocusField::PulseBack:
        case FocusField::DutyBack:
        case FocusField::PwmBack:
            return false;
        case FocusField::GroupFreq:
            return a.freqHz != b.freqHz;
        case FocusField::GroupPhase:
            return a.phaseDegTotal != b.phaseDegTotal;
        case FocusField::GroupShiftUs:
            return a.phaseShiftUs != b.phaseShiftUs;
        case FocusField::GroupPulse:
            return a.pulseUs != b.pulseUs;
        case FocusField::GroupDuty:
            return a.dutyPercent != b.dutyPercent;
        case FocusField::SigEnabled:
            return a.signalEnabled != b.signalEnabled;
        case FocusField::PwmEnabled:
            return a.pwmEnabled != b.pwmEnabled;
        case FocusField::SigMode:
            return a.dacMode != b.dacMode;
        case FocusField::Waveform:
            return a.waveform != b.waveform;
        case FocusField::Amplitude:
            return a.ampVolts != b.ampVolts;
        case FocusField::FreqKHz:
            return a.freqKHz != b.freqKHz;
        case FocusField::FreqHundredHz:
            return a.freqHundredHz != b.freqHundredHz;
        case FocusField::FreqTensHz:
            return a.freqTensHz != b.freqTensHz;
        case FocusField::FreqHz:
            return a.freqHzPart != b.freqHzPart;
        case FocusField::PhaseTens:
            return a.phaseTens != b.phaseTens;
        case FocusField::PhaseDeg:
            return a.phaseDeg != b.phaseDeg;
        case FocusField::PhaseFine:
            return a.phaseFine != b.phaseFine;
        case FocusField::PhaseUs1000:
            return a.phaseUs1000 != b.phaseUs1000;
        case FocusField::PhaseUs100:
            return a.phaseUs100 != b.phaseUs100;
        case FocusField::PhaseUs10:
            return a.phaseUs10 != b.phaseUs10;
        case FocusField::PhaseUs1:
            return a.phaseUs1 != b.phaseUs1;
        case FocusField::PulseUs100:
            return a.pulseUs100 != b.pulseUs100;
        case FocusField::PulseUs10:
            return a.pulseUs10 != b.pulseUs10;
        case FocusField::PulseUs1:
            return a.pulseUs1 != b.pulseUs1;
        case FocusField::Duty10:
            return a.duty10 != b.duty10;
        case FocusField::Duty1:
            return a.duty1 != b.duty1;
        case FocusField::DutyTenths:
            return a.dutyTenths != b.dutyTenths;
        case FocusField::PwmFreq:
            return a.pwmFreqX10 != b.pwmFreqX10;
        case FocusField::PwmCh1X20:
            return a.pwmCh1X20 != b.pwmCh1X20;
        case FocusField::PwmCh1X1:
            return a.pwmCh1X1 != b.pwmCh1X1;
        case FocusField::PwmCh2X20:
            return a.pwmCh2X20 != b.pwmCh2X20;
        case FocusField::PwmCh2X1:
            return a.pwmCh2X1 != b.pwmCh2X1;
        case FocusField::Count:
            return false;
    }
    return false;
}

bool Display::plotParamsChanged(const ParamSnapshot &a, const ParamSnapshot &b) {
    if (a.menu != b.menu) {
        return true;
    }
    if (a.menu == MenuLevel::Signal || a.menu == MenuLevel::SigFreq ||
        a.menu == MenuLevel::SigPhase || a.menu == MenuLevel::SigShiftUs ||
        a.menu == MenuLevel::SigPulse || a.menu == MenuLevel::SigDuty) {
        return a.waveform != b.waveform || a.dacMode != b.dacMode || a.ampVolts != b.ampVolts ||
               a.freqHz != b.freqHz || a.phaseDegTotal != b.phaseDegTotal ||
               a.phaseShiftUs != b.phaseShiftUs || a.dutyPercent != b.dutyPercent ||
               a.pulseUs != b.pulseUs;
    }
    if (a.menu == MenuLevel::Pwm) {
        return a.pwmHz != b.pwmHz || a.pwmCh1Us != b.pwmCh1Us || a.pwmCh2Us != b.pwmCh2Us;
    }
    return false;
}

void Display::fillPwmPeriodPreview(const ParamSnapshot &params, uint8_t *ch1, uint8_t *ch2,
                                   int count) {
    if (ch1 == nullptr || ch2 == nullptr || count <= 0) {
        return;
    }

    float hz = params.pwmHz;
    if (hz < 0.1f) {
        hz = 0.1f;
    }
    const float periodUs = 1000000.0f / hz;
    const float high1 = static_cast<float>(params.pwmCh1Us);
    const float high2 = static_cast<float>(params.pwmCh2Us);

    for (int i = 0; i < count; ++i) {
        const float tUs =
                (count == 1) ? 0.0f
                             : (static_cast<float>(i) / static_cast<float>(count)) * periodUs;
        ch1[i] = (tUs < high1) ? 255 : 0;
        ch2[i] = (tUs < high2) ? 255 : 0;
    }
}

void Display::ensureFocusVisible(FocusField focus, int fieldCount, const FocusField *fields,
                                 int visibleRows) {
    int focusIndex = 0;
    for (int i = 0; i < fieldCount; ++i) {
        if (fields[i] == focus) {
            focusIndex = i;
            break;
        }
    }

    if (focusIndex < scrollOffset_) {
        scrollOffset_ = focusIndex;
    } else if (focusIndex >= scrollOffset_ + visibleRows) {
        scrollOffset_ = focusIndex - visibleRows + 1;
    }

    const int maxOffset = fieldCount - visibleRows;
    if (scrollOffset_ < 0) {
        scrollOffset_ = 0;
    }
    if (maxOffset >= 0 && scrollOffset_ > maxOffset) {
        scrollOffset_ = maxOffset;
    }
    if (maxOffset < 0) {
        scrollOffset_ = 0;
    }
}

void Display::drawDottedSeparator(int y, int width) {
    // 1px dotted line drawn inside the item band (bottom pixel row).
    for (int x = 0; x < width; x += 2) {
        tft_.drawPixel(x, y, kSepColor);
    }
}

void Display::drawUpLevelIcon(int x, int y, uint16_t color) {
    tft_.drawBitmap(x, y, kUpLevelIcon, kBackIconW, kBackIconH, color);
}

void Display::drawThickLine(int x0, int y0, int x1, int y1, uint16_t color, int thickness) {
    const int half = thickness / 2;
    for (int i = 0; i < thickness; ++i) {
        const int dy = i - half;
        tft_.drawLine(x0, y0 + dy, x1, y1 + dy, color);
    }
}

void Display::drawBirdCheck(int boxX, int boxY, uint16_t checkFg) {
    // Relative to 14×14 box; strokes may extend slightly outside.
    drawThickLine(boxX - 1, boxY + 6, boxX + 5, boxY + 12, checkFg, kCheckStrokePx);
    drawThickLine(boxX + 5, boxY + 12, boxX + 15, boxY - 1, checkFg, kCheckStrokePx);
}

void Display::drawCheckbox(int rowY, int rowH, bool checked, bool focused, bool editing,
                           uint16_t rowFg) {
    const int boxX = tft_.width() - kCheckRightPad - kCheckSize;
    const int boxY = rowY + (rowH - kCheckSize) / 2;
    tft_.drawRect(boxX, boxY, kCheckSize, kCheckSize, rowFg);
    if (checked) {
        const uint16_t mark = (focused || editing) ? kCheckMarkOnFocus : kCheckMarkGreen;
        drawBirdCheck(boxX, boxY, mark);
    }
}

void Display::drawFieldRow(int screenIndex, int menuH, int visibleRows, const char *name,
                           const char *value, bool focused, bool editing, bool isBack,
                           bool isCheckbox, bool checked) {
    const int y = rowY(screenIndex, menuH, visibleRows);
    const int h = rowH(screenIndex, menuH, visibleRows);
    const int w = tft_.width();

    uint16_t bg = ST77XX_BLACK;
    uint16_t fg = ST77XX_YELLOW;
    if (editing) {
        bg = kEditBg;
        fg = ST77XX_WHITE;
    } else if (focused) {
        bg = ST77XX_BLUE;
        fg = ST77XX_WHITE;
    } else if (isBack) {
        fg = kBackFg;
    }

    tft_.fillRect(0, y, w, h, bg);
    tft_.setFont(&FreeSansBold9pt7b);
    tft_.setTextSize(1);
    tft_.setTextColor(fg, bg);

    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t tw = 0;
    uint16_t th = 0;
    tft_.getTextBounds(name, 0, 0, &x1, &y1, &tw, &th);

    // GFXfont cursor Y is baseline; place glyph inside the row band.
    int baseline = y + (h - static_cast<int>(y1)) / 2;
    if (baseline + static_cast<int>(y1) < y + 1) {
        baseline = y + 1 - static_cast<int>(y1);
    }
    if (baseline > y + h - 2) {
        baseline = y + h - 2;
    }

    int textX = kPadX - x1;
    if (isBack) {
        const int iconY = y + (h - kBackIconH) / 2;
        drawUpLevelIcon(kPadX, iconY, fg);
        textX = kPadX + kBackIconW + kBackIconGap - x1;
    }

    tft_.setCursor(textX, baseline);
    tft_.print(name);

    if (isCheckbox) {
        drawCheckbox(y, h, checked, focused, editing, fg);
    } else if (value[0] != '\0') {
        tft_.getTextBounds(value, 0, 0, &x1, &y1, &tw, &th);
        tft_.setCursor(w - kPadX - static_cast<int>(tw) - x1, baseline);
        tft_.print(value);
    }

    // tft-ui: 1px dark-dark gray dotted separator only when item height < 22.
    if (h < kSepMaxItemH) {
        drawDottedSeparator(y + h - 1, w);
    }
}

void Display::drawWavePlot(const uint8_t *ch1, const uint8_t *ch2, int count) {
    const int plotY = kMenuHWithPlot;
    const int w = tft_.width();
    const int h = kPlotH;

    tft_.fillRect(0, plotY, w, h, ST77XX_BLACK);
    tft_.drawRect(0, plotY, w, h, kPlotBorderColor);

    const int midY = plotY + h / 2;
    tft_.drawFastHLine(1, midY, w - 2, kPlotMidColor);

    if (ch1 == nullptr || ch2 == nullptr || count < 2) {
        return;
    }

    const int x0 = 1;
    const int plotW = w - 2;
    const int yTop = plotY + 2;
    const int yBot = plotY + h - 3;
    const int plotInnerH = yBot - yTop;
    if (plotInnerH <= 0 || plotW <= 0) {
        return;
    }

    auto sampleY = [&](uint8_t sample) -> int {
        // 255 → top, 0 → bottom
        return yBot - (static_cast<int>(sample) * plotInnerH) / 255;
    };

    int prevX = x0;
    int prevY1 = sampleY(ch1[0]);
    int prevY2 = sampleY(ch2[0]);
    for (int i = 1; i < count; ++i) {
        const int x = x0 + (i * (plotW - 1)) / (count - 1);
        const int y1 = sampleY(ch1[i]);
        const int y2 = sampleY(ch2[i]);
        // Sample-and-hold: flat hold then vertical edge (DAC stair look).
        if (x > prevX) {
            tft_.drawFastHLine(prevX, prevY1, x - prevX, kPlotCh1Color);
            tft_.drawFastHLine(prevX, prevY2, x - prevX, kPlotCh2Color);
        }
        if (y1 != prevY1) {
            tft_.drawFastVLine(x, (y1 < prevY1) ? y1 : prevY1, abs(y1 - prevY1) + 1,
                               kPlotCh1Color);
        }
        if (y2 != prevY2) {
            tft_.drawFastVLine(x, (y2 < prevY2) ? y2 : prevY2, abs(y2 - prevY2) + 1,
                               kPlotCh2Color);
        }
        prevX = x;
        prevY1 = y1;
        prevY2 = y2;
    }
}

void Display::begin() {
    spiTft_.begin(PIN_TFT_SCLK, PIN_TFT_MISO, PIN_TFT_MOSI, PIN_TFT_CS);
    tft_.init(TFT_WIDTH, TFT_HEIGHT);
    tft_.setRotation(2); // 180°
    tft_.fillScreen(ST77XX_BLACK);
    hasLast_ = false;
    scrollOffset_ = 0;
    lastScrollOffset_ = -1;
    lastShowedPlot_ = false;
}

void Display::render(const ParamSnapshot &state, const WavePlotSamples &plot) {
    char nameBuf[24];
    char valueBuf[16];

    int fieldCount = 0;
    const FocusField *fields = menuFields(state, fieldCount);

    const int menuH = menuHeight(state.menu);
    const int visRows = visibleFieldRows(state.menu);
    const bool showPlot = showsPlot(state.menu);

    const bool menuChanged =
            !hasLast_ || last_.menu != state.menu ||
            (state.menu == MenuLevel::Signal && last_.dacMode != state.dacMode);
    if (menuChanged) {
        scrollOffset_ = 0;
    }

    ensureFocusVisible(state.focus, fieldCount, fields, visRows);
    const bool scrollChanged = menuChanged || !hasLast_ || scrollOffset_ != lastScrollOffset_;

    // Leaving a plot menu for Top: clear former plot band (menu stretch uses full height).
    if (menuChanged && lastShowedPlot_ && !showPlot) {
        tft_.fillRect(0, kMenuHWithPlot, tft_.width(), kPlotH, ST77XX_BLACK);
    }

    for (int screen = 0; screen < visRows; ++screen) {
        const int fieldIndex = scrollOffset_ + screen;
        if (fieldIndex >= fieldCount) {
            if (scrollChanged || menuChanged) {
                // Clear leftover rows when the new menu is shorter.
                const int y = rowY(screen, menuH, visRows);
                const int h = rowH(screen, menuH, visRows);
                tft_.fillRect(0, y, tft_.width(), h, ST77XX_BLACK);
            }
            continue;
        }
        const FocusField field = fields[fieldIndex];
        const bool focused = state.focus == field;
        const bool editing = focused && state.editing;
        const bool wasFocused = hasLast_ && !menuChanged && last_.focus == field;
        const bool wasEditing = wasFocused && last_.editing;
        const bool need = scrollChanged || focused != wasFocused || editing != wasEditing ||
                          fieldChanged(last_, state, field);
        if (need) {
            formatFieldName(field, nameBuf, sizeof(nameBuf));
            formatFieldValue(state, field, valueBuf, sizeof(valueBuf));
            const bool isBack = field == FocusField::SigBack || field == FocusField::FreqBack ||
                                field == FocusField::PhaseBack || field == FocusField::ShiftUsBack ||
                                field == FocusField::PulseBack || field == FocusField::DutyBack ||
                                field == FocusField::PwmBack;
            const bool isCheckbox =
                    field == FocusField::SigEnabled || field == FocusField::PwmEnabled;
            const bool checked = field == FocusField::SigEnabled ? state.signalEnabled
                                                                 : state.pwmEnabled;
            drawFieldRow(screen, menuH, visRows, nameBuf, valueBuf, focused, editing, isBack,
                         isCheckbox, isCheckbox && checked);
        }
    }

    if (showPlot) {
        const bool plotNeed = menuChanged || !hasLast_ || !lastShowedPlot_ ||
                              plotParamsChanged(last_, state);
        if (plotNeed && plot.ch1 != nullptr && plot.ch2 != nullptr && plot.count > 0) {
            drawWavePlot(plot.ch1, plot.ch2, plot.count);
        }
    }

    last_ = state;
    hasLast_ = true;
    lastScrollOffset_ = scrollOffset_;
    lastShowedPlot_ = showPlot;
}
