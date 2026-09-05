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
    static uint32_t samplesPerPeriod(float freqHz);
    static uint32_t phaseAtPeriodSample(uint32_t sampleIndex, uint32_t periodSamples);
    static uint8_t scaleSample(uint8_t sample, uint16_t gainQ8);

    // Render one stereo sample (does not advance sampleInPeriod_).
    static void renderPair(uint32_t phase, uint32_t phaseOffset, const uint8_t *lut,
                           uint16_t gainQ8, bool analogPwm, bool sineNeg90, bool rectHold,
                           uint32_t pulseEnd, uint8_t *ch1, uint8_t *ch2);

    void fillDmaChunk(uint8_t *dst, size_t byteCount);
    void refillTaskLoop();
    static void refillTaskEntry(void *arg);
xs
    // 131072 index space → phase step ≈ 0.0027°. RAM table is 65536 bytes (ESP32 DRAM).
    static constexpr int kLutSize = 131072;
    static constexpr int kLutIndexShift = 15; // 32 - log2(131072)
    static constexpr int kLutStorage = 65536;
    static constexpr int kLutStorageShift = 1; // kLutSize / kLutStorage
    static constexpr int kLutQuarter = kLutSize / 4; // 90° in LUT indices
    static constexpr float kSampleRateHz = 400000.0f;
    // ESP32 I2S DAC: freq_hz is per-channel sample rate (WS), not 2× stereo byte rate.
    static constexpr uint32_t kDmaFreqHz = static_cast<uint32_t>(kSampleRateHz);
    static constexpr uint32_t kDmaDescNum = 8;
    static constexpr size_t kDmaBufSize = 512;
    static constexpr float kDacFullScaleV = 3.3f;
    static constexpr uint8_t kMidscale = 128;

    static_assert((1 << (32 - kLutIndexShift)) == kLutSize, "LUT size/shift mismatch");
    static_assert((kLutSize / kLutStorage) == (1 << kLutStorageShift), "LUT storage shift mismatch");
    static_assert(360.0f / static_cast<float>(kLutSize) <= 0.05f, "phase step must be <= 0.05 deg");

    // Heap 64 KB table; DDS index is kLutSize. DMA refill must not call sin() per sample.
    uint8_t *lut_ = nullptr;
    Waveform waveform_ = Waveform::Sine;

    // Integer samples per period so sample 0 is always LUT index 0 (no walking stairs).
    volatile uint32_t periodSamples_ = 1;
    volatile uint32_t sampleInPeriod_ = 0;
    volatile uint32_t phaseOffset_ = 0;
    // Q8 fixed-point gain: 256 == full scale (ampVolts / 3.3)
    volatile uint16_t ampGainQ8_ = 256;

    // Analog PWM: compress base LUT into [0, pulseEnd); idle (0) afterward.
    // analogPwmSineNeg90_: index as sin(A-90°) for both channels.
    // analogPwmRectHold_: Rect is high for the whole pulse window (duty = high time).
    volatile bool analogPwm_ = false;
    volatile bool analogPwmSineNeg90_ = false;
    volatile bool analogPwmRectHold_ = false;
    volatile uint32_t analogPwmPulseEnd_ = 0;

    volatile bool paused_ = true;

    DacContinuousHandle dac_ = nullptr;
    QueueHandle_t dmaEventQue_ = nullptr;
    TaskHandle_t refillTask_ = nullptr;
};
