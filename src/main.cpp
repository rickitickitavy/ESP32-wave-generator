#include <Arduino.h>

#include "display.h"
#include "encoder.h"
#include "param_model.h"
#include "pins.h"
#include "signal_generator.h"

namespace {
    ParamModel paramModel;
    Display display;
    Encoder encoder;
    SignalGenerator signalGenerator;

    void IRAM_ATTR onDacTimerIsr() {
        signalGenerator.onTimer();
    }

    void applyAndPaint() {
        const ParamSnapshot &params = paramModel.snapshot();
        signalGenerator.pause();
        signalGenerator.apply(params);
        display.render(params);
        signalGenerator.resume();
    }
} // namespace

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("esp32-Sound-array: dual DAC generator");

    paramModel.begin();
    display.begin();
    encoder.begin(PIN_ENCODER_A, PIN_ENCODER_B, PIN_ENCODER_BTN);

    // Paint UI before the DAC timer is running.
    display.render(paramModel.snapshot());

    signalGenerator.begin(onDacTimerIsr);
    applyAndPaint();
}

void loop() {
    encoder.update();

    bool changed = false;

    if (encoder.consumePress()) {
        paramModel.nextFocus();
        changed = true;
    }

    const int steps = encoder.consumeSteps();
    if (steps != 0) {
        paramModel.applyEncoderDelta(steps);
        changed = true;
    }

    if (changed) {
        applyAndPaint();
    }
}
