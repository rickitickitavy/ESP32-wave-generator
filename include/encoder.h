#pragma once

#include <Arduino.h>

class Encoder {
public:
    // quadIsr: free-function trampoline owned by main.cpp (calls onQuadratureIsr).
    void begin(int pinA, int pinB, int pinBtn, void (*quadIsr)());

    void update();

    // Cumulative detents since last consume (positive = CW).
    int consumeSteps();

    // Short click: press+release before 1 second.
    bool consumePress();

    // Called from ISR trampoline owned by main.cpp.
    void IRAM_ATTR onQuadratureIsr();

private:
    int pinA_ = -1;
    int pinB_ = -1;
    int pinBtn_ = -1;

    volatile int steps_ = 0;
    volatile uint8_t lastAb_ = 0;

    int btnStable_ = HIGH;
    int btnReading_ = HIGH;
    unsigned long btnLastChangeMs_ = 0;
    unsigned long btnDownMs_ = 0;
    bool pressPending_ = false;
    bool hold1FiredThisHold_ = false;

    static constexpr unsigned long kDebounceMs = 40;
    static constexpr unsigned long kHoldPressMs = 1000;
    static constexpr int kStepsPerDetent = 4;
};
