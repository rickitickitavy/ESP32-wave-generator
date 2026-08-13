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

int Display::rowY(int screenIndex) {
    // Stretch bands across full TFT height (26/27 px for 12 rows).
    return (screenIndex * kLogicalH) / kVisibleRows;
}

int Display::rowH(int screenIndex) {
    return rowY(screenIndex + 1) - rowY(screenIndex);
}

void Display::formatFieldName(FocusField field, char *buf, size_t buflen) {
    switch (field) {
        case FocusField::GroupSignal:
            snprintf(buf, buflen, "Signal generator");
            break;
        case FocusField::GroupPwm:
            snprintf(buf, buflen, "PWM");
            break;
        case FocusField::SigEnabled:
        case FocusField::PwmEnabled:
            snprintf(buf, buflen, "Enabled");
            break;
        case FocusField::SigBack:
        case FocusField::PwmBack:
            snprintf(buf, buflen, "BACK");
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
        case FocusField::GroupSignal:
        case FocusField::GroupPwm:
            snprintf(buf, buflen, ">");
            break;
        case FocusField::SigBack:
        case FocusField::PwmBack:
            buf[0] = '\0';
            break;
        case FocusField::SigEnabled:
        case FocusField::PwmEnabled:
            // Value drawn as checkbox, not ON/OFF text.
            buf[0] = '\0';
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
    snprintf(buf, buflen, "%s %.0fHz %+.0f° %s P%.0f %d/%d", s.signalEnabled ? "DAC" : "dac",
             static_cast<double>(s.freqHz), static_cast<double>(s.phaseDegTotal),
             s.pwmEnabled ? "PWM" : "pwm", static_cast<double>(s.pwmHz), s.pwmCh1Us, s.pwmCh2Us);
}

bool Display::fieldChanged(const ParamSnapshot &a, const ParamSnapshot &b, FocusField field) {
    switch (field) {
        case FocusField::GroupSignal:
        case FocusField::GroupPwm:
        case FocusField::SigBack:
        case FocusField::PwmBack:
            return false;
        case FocusField::SigEnabled:
            return a.signalEnabled != b.signalEnabled;
        case FocusField::PwmEnabled:
            return a.pwmEnabled != b.pwmEnabled;
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

void Display::ensureFocusVisible(FocusField focus, int fieldCount, const FocusField *fields) {
    int focusIndex = 0;
    for (int i = 0; i < fieldCount; ++i) {
        if (fields[i] == focus) {
            focusIndex = i;
            break;
        }
    }

    if (focusIndex < scrollOffset_) {
        scrollOffset_ = focusIndex;
    } else if (focusIndex >= scrollOffset_ + kVisibleFieldRows) {
        scrollOffset_ = focusIndex - kVisibleFieldRows + 1;
    }

    const int maxOffset = fieldCount - kVisibleFieldRows;
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

void Display::drawFieldRow(int screenIndex, const char *name, const char *value, bool focused,
                           bool editing, bool isBack, bool isCheckbox, bool checked) {
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
    char nameBuf[24];
    char valueBuf[16];
    char summaryBuf[48];

    int fieldCount = 0;
    const FocusField *fields = menuFields(state.menu, fieldCount);

    const bool menuChanged = !hasLast_ || last_.menu != state.menu;
    if (menuChanged) {
        scrollOffset_ = 0;
    }

    ensureFocusVisible(state.focus, fieldCount, fields);
    const bool scrollChanged = menuChanged || !hasLast_ || scrollOffset_ != lastScrollOffset_;

    for (int screen = 0; screen < kVisibleFieldRows; ++screen) {
        const int fieldIndex = scrollOffset_ + screen;
        if (fieldIndex >= fieldCount) {
            if (scrollChanged || menuChanged) {
                // Clear leftover rows when the new menu is shorter.
                const int y = rowY(screen);
                const int h = rowH(screen);
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
            const bool isBack = field == FocusField::SigBack || field == FocusField::PwmBack;
            const bool isCheckbox =
                    field == FocusField::SigEnabled || field == FocusField::PwmEnabled;
            const bool checked = field == FocusField::SigEnabled ? state.signalEnabled
                                                                 : state.pwmEnabled;
            drawFieldRow(screen, nameBuf, valueBuf, focused, editing, isBack, isCheckbox,
                         isCheckbox && checked);
        }
    }

    const bool summaryNeed =
            scrollChanged || !hasLast_ || last_.freqHz != state.freqHz ||
            last_.phaseDegTotal != state.phaseDegTotal || last_.pwmHz != state.pwmHz ||
            last_.pwmCh1Us != state.pwmCh1Us || last_.pwmCh2Us != state.pwmCh2Us ||
            last_.signalEnabled != state.signalEnabled || last_.pwmEnabled != state.pwmEnabled;
    if (summaryNeed) {
        formatSummary(state, summaryBuf, sizeof(summaryBuf));
        drawSummaryRow(summaryBuf);
    }

    last_ = state;
    hasLast_ = true;
    lastScrollOffset_ = scrollOffset_;
}
