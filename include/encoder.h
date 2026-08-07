#pragma once

#include <Arduino.h>

class Encoder {
public:
    void begin(int pinA, int pinB, int pinBtn);

    // Poll quadrature + debounce button (call every loop).
    void update();

    // Cumulative detents since last consume (positive = CW).
    int consumeSteps();

    // Short click: press+release before 1 second.
    bool consumePress();

private:
    int pinA_ = -1;
    int pinB_ = -1;
    int pinBtn_ = -1;

    int steps_ = 0;
    uint8_t lastAb_ = 0;

    int btnStable_ = HIGH;
    int btnReading_ = HIGH;
    unsigned long btnLastChangeMs_ = 0;
    unsigned long btnDownMs_ = 0;
    bool pressPending_ = false;
    bool hold1FiredThisHold_ = false;

    static constexpr unsigned long kDebounceMs = 40;
    static constexpr unsigned long kHoldPressMs = 1000;
    // KY-040-style modules typically produce 2 gray-code steps per detent.
    static constexpr int kStepsPerDetent = 2;
};
