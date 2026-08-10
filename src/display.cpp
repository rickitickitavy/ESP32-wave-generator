#include "display.h"

#include <Adafruit_GFX.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <cstdio>

#include "pins.h"

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

int Display::rowY(int screenIndex) {
    return screenIndex * kRowH;
}

int Display::rowH(int /*screenIndex*/) {
    return kRowH;
}

void Display::formatFieldName(FocusField field, char *buf, size_t buflen) {
    switch (field) {
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
            snprintf(buf, buflen, "Ph .01");
            break;
        case FocusField::Waveform:
            snprintf(buf, buflen, "Wave");
            break;
        case FocusField::Amplitude:
            snprintf(buf, buflen, "Amp V");
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
            snprintf(buf, buflen, "%+.2f", static_cast<double>(s.phaseFine));
            break;
        case FocusField::Waveform:
            snprintf(buf, buflen, "%s", waveformName(s.waveform));
            break;
        case FocusField::Amplitude:
            snprintf(buf, buflen, "%.1f", static_cast<double>(s.ampVolts));
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

void Display::formatSummary(const ParamSnapshot &s, char *buf, size_t buflen) {
    snprintf(buf, buflen, "%.0fHz %+.0f° P%.0f %d/%dus", static_cast<double>(s.freqHz),
             static_cast<double>(s.phaseDegTotal), static_cast<double>(s.pwmHz), s.pwmCh1Us,
             s.pwmCh2Us);
}

bool Display::fieldChanged(const ParamSnapshot &a, const ParamSnapshot &b, FocusField field) {
    switch (field) {
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
        case FocusField::Waveform:
            return a.waveform != b.waveform;
        case FocusField::Amplitude:
            return a.ampVolts != b.ampVolts;
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

void Display::ensureFocusVisible(FocusField focus) {
    const int focusIndex = static_cast<int>(focus);
    if (focusIndex < scrollOffset_) {
        scrollOffset_ = focusIndex;
    } else if (focusIndex >= scrollOffset_ + kVisibleFieldRows) {
        scrollOffset_ = focusIndex - kVisibleFieldRows + 1;
    }

    const int maxOffset = kFieldCount - kVisibleFieldRows;
    if (scrollOffset_ < 0) {
        scrollOffset_ = 0;
    }
    if (maxOffset >= 0 && scrollOffset_ > maxOffset) {
        scrollOffset_ = maxOffset;
    }
}

void Display::drawFieldRow(int screenIndex, const char *name, const char *value, bool focused,
                           bool editing) {
    const int y = rowY(screenIndex);
    const int h = rowH(screenIndex);
    const int w = tft_.width();

    uint16_t bg = ST77XX_BLACK;
    uint16_t fg = ST77XX_YELLOW;
    if (editing) {
        bg = kEditBg;
        fg = ST77XX_WHITE;
    } else if (focused) {
        bg = ST77XX_BLUE;
        fg = ST77XX_WHITE;
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

    tft_.setCursor(kPadX - x1, baseline);
    tft_.print(name);

    tft_.getTextBounds(value, 0, 0, &x1, &y1, &tw, &th);
    tft_.setCursor(w - kPadX - static_cast<int>(tw) - x1, baseline);
    tft_.print(value);
}

void Display::drawSummaryRow(const char *text) {
    const int y = rowY(kSummaryScreenRow);
    const int h = rowH(kSummaryScreenRow);
    const int w = tft_.width();
    const uint16_t bg = ST77XX_BLACK;
    const uint16_t fg = ST77XX_YELLOW;

    tft_.fillRect(0, y, w, h, bg);
    tft_.setFont(&FreeSansBold9pt7b);
    tft_.setTextSize(1);
    tft_.setTextColor(fg, bg);

    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t tw = 0;
    uint16_t th = 0;
    tft_.getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);

    int baseline = y + (h - static_cast<int>(y1)) / 2;
    if (baseline + static_cast<int>(y1) < y + 1) {
        baseline = y + 1 - static_cast<int>(y1);
    }
    if (baseline > y + h - 2) {
        baseline = y + h - 2;
    }
    tft_.setCursor(kPadX - x1, baseline);
    tft_.print(text);
}

void Display::begin() {
    spiTft_.begin(PIN_TFT_SCLK, PIN_TFT_MISO, PIN_TFT_MOSI, PIN_TFT_CS);
    tft_.init(TFT_WIDTH, TFT_HEIGHT);
    tft_.setRotation(2); // 180°
    tft_.fillScreen(ST77XX_BLACK);
    hasLast_ = false;
    scrollOffset_ = 0;
    lastScrollOffset_ = -1;
}

void Display::render(const ParamSnapshot &state) {
    char nameBuf[16];
    char valueBuf[16];
    char summaryBuf[48];

    ensureFocusVisible(state.focus);
    const bool scrollChanged = !hasLast_ || scrollOffset_ != lastScrollOffset_;

    for (int screen = 0; screen < kVisibleFieldRows; ++screen) {
        const int fieldIndex = scrollOffset_ + screen;
        if (fieldIndex >= kFieldCount) {
            break;
        }
        const auto field = static_cast<FocusField>(fieldIndex);
        const bool focused = state.focus == field;
        const bool editing = focused && state.editing;
        const bool wasFocused = hasLast_ && last_.focus == field;
        const bool wasEditing = wasFocused && last_.editing;
        const bool need = scrollChanged || focused != wasFocused || editing != wasEditing ||
                          fieldChanged(last_, state, field);
        if (need) {
            formatFieldName(field, nameBuf, sizeof(nameBuf));
            formatFieldValue(state, field, valueBuf, sizeof(valueBuf));
            drawFieldRow(screen, nameBuf, valueBuf, focused, editing);
        }
    }

    const bool summaryNeed =
            scrollChanged || !hasLast_ || last_.freqHz != state.freqHz ||
            last_.phaseDegTotal != state.phaseDegTotal || last_.pwmHz != state.pwmHz ||
            last_.pwmCh1Us != state.pwmCh1Us || last_.pwmCh2Us != state.pwmCh2Us;
    if (summaryNeed) {
        formatSummary(state, summaryBuf, sizeof(summaryBuf));
        drawSummaryRow(summaryBuf);
    }

    last_ = state;
    hasLast_ = true;
    lastScrollOffset_ = scrollOffset_;
}
