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
    // Phase of CH2 relative to CH1 in degrees (positive => CH2 leads CH1).
    void setPhaseDeg(float phaseDeg);
    // Phase of CH2 relative to CH1 in microseconds (positive => CH2 leads CH1).
    void setPhaseUs(int phaseUs, float freqHz);
    void setWaveform(Waveform waveform);
    void setAmplitudeVolts(float volts);

    void apply(const ParamSnapshot &params);

    // Called from ISR trampoline owned by main.cpp.
    void IRAM_ATTR onTimer();

private:
    void buildLuts();
    void setAnalogPwmDuty(float dutyPercent);
    const uint8_t *baseLut(Waveform waveform) const;
    static uint32_t freqToPhaseInc(float freqHz);
    static uint8_t IRAM_ATTR scaleSample(uint8_t sample, uint16_t gainQ8);

    // 32768 → phase step 360/32768 ≈ 0.011° (must be ≤ 0.05° real resolution).
    static constexpr int kLutSize = 32768;
    static constexpr int kLutIndexShift = 17; // 32 - log2(32768)
    static constexpr int kLutQuarter = kLutSize / 4; // 90° in LUT indices
    static constexpr float kSampleRateHz = 100000.0f;
    static constexpr uint64_t kTimerAlarmUs = 10; // 1 MHz timer → 100 kHz
    static constexpr float kDacFullScaleV = 3.3f;

    static_assert((1 << (32 - kLutIndexShift)) == kLutSize, "LUT size/shift mismatch");
    static_assert(360.0f / static_cast<float>(kLutSize) <= 0.05f, "phase step must be <= 0.05 deg");

    uint8_t lutSine_[kLutSize]{};
    uint8_t lutTriangle_[kLutSize]{};
    uint8_t lutRect_[kLutSize]{};

    const uint8_t *volatile lut_ = nullptr;
    volatile uint32_t phase_ = 0;
    volatile uint32_t phaseInc_ = 0;
    volatile uint32_t phaseOffset_ = 0;
    // Q8 fixed-point gain: 256 == full scale (ampVolts / 3.3)
    volatile uint16_t ampGainQ8_ = 256;

    // Analog PWM: compress base LUT into [0, pulseEnd); idle (0) afterward.
    // scaleQ16 = kLutSize * 65536 / pulseEnd (0 when disabled / zero pulse).
    // analogPwmSineNeg90_: index as sin(A-90°) for both channels.
    volatile bool analogPwm_ = false;
    volatile bool analogPwmSineNeg90_ = false;
    volatile uint32_t analogPwmPulseEnd_ = 0;
    volatile uint32_t analogPwmScaleQ16_ = 0;

    hw_timer_t *timer_ = nullptr;
};
