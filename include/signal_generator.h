#pragma once

#include <Arduino.h>

#include "param_model.h"

class SignalGenerator {
public:
    void begin(void (*timerIsr)());

    // Stop/start DAC ISR around SPI TFT updates (high-rate ISR starves SPI / loop).
    void pause();
    void resume();

    void setFrequency(float freqHz);
    void setPhaseDeg(float phaseDeg);
    void setWaveform(Waveform waveform);
    void setAmplitudeVolts(float volts);

    void apply(const ParamSnapshot &params);

    // Called from ISR trampoline owned by main.cpp.
    void IRAM_ATTR onTimer();

private:
    void buildLuts();
    static uint32_t freqToPhaseInc(float freqHz);
    static uint8_t IRAM_ATTR scaleSample(uint8_t sample, uint16_t gainQ8);

    static constexpr int kLutSize = 256;
    // 25 kHz leaves CPU time for encoder polling and TFT; still enough for 8 kHz output.
    static constexpr float kSampleRateHz = 25000.0f;
    static constexpr uint64_t kTimerAlarmUs = 40; // 1 MHz timer → 25 kHz
    static constexpr float kDacFullScaleV = 3.3f;

    uint8_t lutSine_[kLutSize]{};
    uint8_t lutTriangle_[kLutSize]{};
    uint8_t lutRect_[kLutSize]{};

    const uint8_t *volatile lut_ = nullptr;
    volatile uint32_t phase_ = 0;
    volatile uint32_t phaseInc_ = 0;
    volatile uint32_t phaseOffset_ = 0;
    // Q8 fixed-point gain: 256 == full scale (ampVolts / 3.3)
    volatile uint16_t ampGainQ8_ = 256;

    hw_timer_t *timer_ = nullptr;
};
