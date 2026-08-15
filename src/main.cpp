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

    uint8_t plotCh1[Display::kPlotSampleCount];
    uint8_t plotCh2[Display::kPlotSampleCount];

    void IRAM_ATTR onEncoderIsr() {
        encoder.onQuadratureIsr();
    }

    WavePlotSamples fillPlotIfNeeded(const ParamSnapshot &params) {
        WavePlotSamples plot;
        if (params.menu == MenuLevel::Signal || params.menu == MenuLevel::SigFreq ||
            params.menu == MenuLevel::SigPhase || params.menu == MenuLevel::SigShiftUs ||
            params.menu == MenuLevel::SigPulse || params.menu == MenuLevel::SigDuty) {
            signalGenerator.fillPeriodPreview(params, plotCh1, plotCh2, Display::kPlotSampleCount);
            plot.ch1 = plotCh1;
            plot.ch2 = plotCh2;
            plot.count = Display::kPlotSampleCount;
        } else if (params.menu == MenuLevel::Pwm) {
            Display::fillPwmPeriodPreview(params, plotCh1, plotCh2, Display::kPlotSampleCount);
            plot.ch1 = plotCh1;
            plot.ch2 = plotCh2;
            plot.count = Display::kPlotSampleCount;
        }
        return plot;
    }

    void applyAndPaint() {
        const ParamSnapshot &params = paramModel.snapshot();
        // LEDC PWM is HW-timed and keeps running while DAC is muted for TFT paint.
        pwmGenerator.apply(params);
        signalGenerator.pause();
        signalGenerator.apply(params);
        display.render(params, fillPlotIfNeeded(params));
        if (params.signalEnabled) {
            signalGenerator.resume();
        }
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

    // Paint UI before DAC DMA starts (Top menu — no plot samples).
    display.render(paramModel.snapshot(), WavePlotSamples{});
    pwmGenerator.apply(paramModel.snapshot());

    signalGenerator.begin();
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
