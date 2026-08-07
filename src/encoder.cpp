#include "encoder.h"

namespace {
    constexpr int8_t kQuadTable[16] = {
        0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0,
    };
} // namespace

void Encoder::begin(int pinA, int pinB, int pinBtn) {
    pinA_ = pinA;
    pinB_ = pinB;
    pinBtn_ = pinBtn;

    pinMode(pinA_, INPUT_PULLUP);
    pinMode(pinB_, INPUT_PULLUP);
    // GPIO34 (and other input-only pins) have no internal pull-up.
    pinMode(pinBtn_, INPUT);

    const uint8_t a = digitalRead(pinA_) ? 1 : 0;
    const uint8_t b = digitalRead(pinB_) ? 1 : 0;
    lastAb_ = static_cast<uint8_t>((a << 1) | b);
    steps_ = 0;

    btnStable_ = digitalRead(pinBtn_);
    btnReading_ = btnStable_;
    btnLastChangeMs_ = millis();
    pressPending_ = false;
    hold1FiredThisHold_ = false;
}

void Encoder::update() {
    // Poll quadrature here — a  high-rate DAC timer ISR would starve encoder ISRs.
    const uint8_t a = digitalRead(pinA_) ? 1 : 0;
    const uint8_t b = digitalRead(pinB_) ? 1 : 0;
    const uint8_t ab = static_cast<uint8_t>((a << 1) | b);
    if (ab != lastAb_) {
        const int8_t delta = kQuadTable[(lastAb_ << 2) | ab];
        lastAb_ = ab;
        if (delta != 0) {
            steps_ += delta;
        }
    }

    const int reading = digitalRead(pinBtn_);
    if (reading != btnReading_) {
        btnReading_ = reading;
        btnLastChangeMs_ = millis();
    }
    if ((millis() - btnLastChangeMs_) >= kDebounceMs && reading != btnStable_) {
        btnStable_ = reading;
        if (btnStable_ == LOW) {
            btnDownMs_ = millis();
            hold1FiredThisHold_ = false;
        } else {
            if (!hold1FiredThisHold_) {
                pressPending_ = true;
            }
        }
    }

    if (btnStable_ == LOW) {
        const unsigned long held = millis() - btnDownMs_;
        if (!hold1FiredThisHold_ && held >= kHoldPressMs) {
            hold1FiredThisHold_ = true;
        }
    }
}

int Encoder::consumeSteps() {
    const int detents = steps_ / kStepsPerDetent;
    steps_ = steps_ % kStepsPerDetent;
    return detents;
}

bool Encoder::consumePress() {
    if (!pressPending_) {
        return false;
    }
    pressPending_ = false;
    return true;
}
