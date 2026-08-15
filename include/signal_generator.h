#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "param_model.h"

struct dac_continuous_s;
using DacContinuousHandle = struct dac_continuous_s *;

class SignalGenerator {
public:
    void begin();

    // Mute DAC to midscale around SPI TFT updates / while disabled.
    // DMA keeps running; refill task writes 128,128 while paused.
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

    // One-period CH1/CH2 preview: horizontal axis is one wave period, but values are
    // the real DAC samples at kSampleRateHz (sample-and-hold into `count` pixels →
    // coarse stairs at high freq, smooth at low freq).
    void fillPeriodPreview(const ParamSnapshot &params, uint8_t *ch1, uint8_t *ch2,
                           int count) const;

    static constexpr float sampleRateHz() { return kSampleRateHz; }

private:
    void fillLut(Waveform waveform);
    void setAnalogPwmDuty(float dutyPercent);
    static uint8_t sampleAt(Waveform waveform, int index);
    static uint32_t freqToPhaseInc(float freqHz);
    static uint8_t scaleSample(uint8_t sample, uint16_t gainQ8);

    // Render one stereo sample from DDS state (does not advance phase_).
    static void renderPair(uint32_t phase, uint32_t phaseOffset, const uint8_t *lut,
                           uint16_t gainQ8, bool analogPwm, bool sineNeg90, uint32_t pulseEnd,
                           uint32_t scaleQ16, uint8_t *ch1, uint8_t *ch2);

    void fillDmaChunk(uint8_t *dst, size_t byteCount);
    void refillTaskLoop();
    static void refillTaskEntry(void *arg);

    // 32768 → phase step 360/32768 ≈ 0.011° (must be ≤ 0.05° real resolution).
    static constexpr int kLutSize = 32768;
    static constexpr int kLutIndexShift = 17; // 32 - log2(32768)
    static constexpr int kLutQuarter = kLutSize / 4; // 90° in LUT indices
    static constexpr float kSampleRateHz = 100000.0f;
    // ALTER mode: 2 bytes per stereo sample → DMA byte rate = 2 * sample rate.
    static constexpr uint32_t kDmaFreqHz = static_cast<uint32_t>(kSampleRateHz) * 2u;
    static constexpr uint32_t kDmaDescNum = 8;
    static constexpr size_t kDmaBufSize = 512;
    static constexpr float kDacFullScaleV = 3.3f;
    static constexpr uint8_t kMidscale = 128;

    static_assert((1 << (32 - kLutIndexShift)) == kLutSize, "LUT size/shift mismatch");
    static_assert(360.0f / static_cast<float>(kLutSize) <= 0.05f, "phase step must be <= 0.05 deg");

    // Single shared wave table; rewritten when waveform changes.
    uint8_t lut_[kLutSize]{};
    Waveform waveform_ = Waveform::Sine;

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

    volatile bool paused_ = true;

    DacContinuousHandle dac_ = nullptr;
    QueueHandle_t dmaEventQue_ = nullptr;
    TaskHandle_t refillTask_ = nullptr;
};
