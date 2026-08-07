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

int Display::rowY(int index) {
    return (index * kLogicalH) / kRowCount;
}

int Display::rowH(int index) {
    return rowY(index + 1) - rowY(index);
}

void Display::formatField(const ParamSnapshot &s, FocusField field, char *buf, size_t buflen) {
    switch (field) {
        case FocusField::FreqKHz:
            snprintf(buf, buflen, "F kHz   %d", s.freqKHz);
            break;
        case FocusField::FreqHundredHz:
            snprintf(buf, buflen, "F 100Hz %d", s.freqHundredHz);
            break;
        case FocusField::FreqHz:
            snprintf(buf, buflen, "F Hz    %.1f", static_cast<double>(s.freqHzPart));
            break;
        case FocusField::PhaseTens:
            snprintf(buf, buflen, "Ph x10  %d", s.phaseTens);
            break;
        case FocusField::PhaseDeg:
            snprintf(buf, buflen, "Ph deg  %d", s.phaseDeg);
            break;
        case FocusField::PhaseFine:
            snprintf(buf, buflen, "Ph .01  %+.2f", static_cast<double>(s.phaseFine));
            break;
        case FocusField::Waveform:
            snprintf(buf, buflen, "Wave    %s", waveformName(s.waveform));
            break;
        case FocusField::Amplitude:
            snprintf(buf, buflen, "Amp V   %.1f", static_cast<double>(s.ampVolts));
            break;
        case FocusField::Count:
            buf[0] = '\0';
            break;
    }
}

void Display::formatSummary(const ParamSnapshot &s, char *buf, size_t buflen) {
    snprintf(buf, buflen, "%.1fHz  %+.2fdeg", static_cast<double>(s.freqHz),
             static_cast<double>(s.phaseDegTotal));
}

bool Display::fieldChanged(const ParamSnapshot &a, const ParamSnapshot &b, FocusField field) {
    switch (field) {
        case FocusField::FreqKHz:
            return a.freqKHz != b.freqKHz;
        case FocusField::FreqHundredHz:
            return a.freqHundredHz != b.freqHundredHz;
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
        case FocusField::Count:
            return false;
    }
    return false;
}

void Display::drawRow(int index, const char *text, bool focused) {
    const int y = rowY(index);
    const int h = rowH(index);
    const int w = tft_.width();
    const uint16_t bg = focused ? ST77XX_BLUE : ST77XX_BLACK;
    const uint16_t fg = focused ? ST77XX_WHITE : ST77XX_YELLOW;

    tft_.fillRect(0, y, w, h, bg);
    tft_.setFont(&FreeSansBold9pt7b);
    tft_.setTextSize(1);
    tft_.setTextColor(fg, bg);

    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t tw = 0;
    uint16_t th = 0;
    tft_.getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);

    // GFXfont cursor Y is baseline; place glyph inside the row band.
    int baseline = y + (h - static_cast<int>(y1)) / 2;
    if (baseline + static_cast<int>(y1) < y + 1) {
        baseline = y + 1 - static_cast<int>(y1);
    }
    if (baseline > y + h - 2) {
        baseline = y + h - 2;
    }
    tft_.setCursor(6 - x1, baseline);
    tft_.print(text);
}

void Display::begin() {
    spiTft_.begin(PIN_TFT_SCLK, PIN_TFT_MISO, PIN_TFT_MOSI, PIN_TFT_CS);
    tft_.init(TFT_WIDTH, TFT_HEIGHT);
    tft_.setRotation(2); // 180°
    tft_.fillScreen(ST77XX_BLACK);
    hasLast_ = false;
}

void Display::render(const ParamSnapshot &state) {
    char buf[40];

    for (int i = 0; i < static_cast<int>(FocusField::Count); ++i) {
        const auto field = static_cast<FocusField>(i);
        const bool focused = state.focus == field;
        const bool need =
                !hasLast_ || focused != (last_.focus == field) || fieldChanged(last_, state, field);
        if (need) {
            formatField(state, field, buf, sizeof(buf));
            drawRow(i, buf, focused);
        }
    }

    const bool summaryNeed =
            !hasLast_ || last_.freqHz != state.freqHz || last_.phaseDegTotal != state.phaseDegTotal;
    if (summaryNeed) {
        formatSummary(state, buf, sizeof(buf));
        drawRow(kSummaryRow, buf, false);
    }

    last_ = state;
    hasLast_ = true;
}
