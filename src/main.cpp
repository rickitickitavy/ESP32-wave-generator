#include <Arduino.h>

#include "display.h"
#include "encoder.h"
#include "param_model.h"
#include "pins.h"
#include "pwm_generator.h"
#include "signal_generator.h"

namespace {
    ParamModel paramModel;
    Display display;
    Encoder encoder;
    SignalGenerator signalGenerator;
    PwmGenerator pwmGenerator;

    void IRAM_ATTR onEncoderIsr() {
        encoder.onQuadratureIsr();
    }

    void IRAM_ATTR onDacTimerIsr() {
        signalGenerator.onTimer();
    }

    void applyAndPaint() {
        const ParamSnapshot &params = paramModel.snapshot();
        // LEDC PWM is HW-timed and must keep running while DAC is paused for TFT SPI.
        pwmGenerator.apply(params);
        signalGenerator.pause();
        signalGenerator.apply(params);
        display.render(params);
        signalGenerator.resume();
    }
} // namespace

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("esp32-Sound-array: dual DAC + PWM generator");

    paramModel.begin();
    display.begin();
    encoder.begin(PIN_ENCODER_A, PIN_ENCODER_B, PIN_ENCODER_BTN, onEncoderIsr);
    pwmGenerator.begin();

    // Paint UI before the DAC timer is running.
    display.render(paramModel.snapshot());
    pwmGenerator.apply(paramModel.snapshot());

    signalGenerator.begin(onDacTimerIsr);
    applyAndPaint();
}

void loop() {
    encoder.update();

    bool changed = false;

    if (encoder.consumePress()) {
        paramModel.toggleEdit();
        changed = true;
    }

    const int steps = encoder.consumeSteps();
    if (steps != 0) {
        if (paramModel.snapshot().editing) {
            paramModel.applyEncoderDelta(steps);
        } else {
            paramModel.moveFocus(steps);
        }
        changed = true;
    }

    if (changed) {
        applyAndPaint();
    }
}
