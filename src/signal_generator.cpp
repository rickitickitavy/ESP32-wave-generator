#include "signal_generator.h"

#include <cmath>
#include <cstring>

#include "driver/dac_continuous.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

namespace {
    // Convert-done ISR: only posts the DMA buffer descriptor to the refill queue.
    // user_data is the QueueHandle_t (no SignalGenerator singleton).
    bool IRAM_ATTR onConvertDone(dac_continuous_handle_t /*handle*/, const dac_event_data_t *event,
                                 void *userData) {
        auto *que = static_cast<QueueHandle_t>(userData);
        if (que == nullptr || event == nullptr) {
            return false;
        }
        BaseType_t hpTaskWoken = pdFALSE;
        if (xQueueIsQueueFullFromISR(que) == pdTRUE) {
            dac_event_data_t dummy;
            xQueueReceiveFromISR(que, &dummy, &hpTaskWoken);
        }
        xQueueSendFromISR(que, event, &hpTaskWoken);
        return hpTaskWoken == pdTRUE;
    }
} // namespace

uint8_t SignalGenerator::sampleAt(Waveform waveform, int index) {
    if (index < 0) {
        index = 0;
    }
    if (index >= kLutSize) {
        index = kLutSize - 1;
    }

    switch (waveform) {
        case Waveform::Rectangular:
            return (index < kLutSize / 2) ? 255 : 0;
        case Waveform::Triangle: {
            int tri;
            if (index < kLutSize / 2) {
                tri = (index * 255 * 2) / (kLutSize / 2);
                if (tri > 255) {
                    tri = 255;
                }
            } else {
                tri = 255 - (((index - kLutSize / 2) * 255 * 2) / (kLutSize / 2));
                if (tri < 0) {
                    tri = 0;
                }
            }
            return static_cast<uint8_t>(tri);
        }
        case Waveform::Sine:
        default: {
            const float t = (2.0f * static_cast<float>(M_PI) * static_cast<float>(index)) /
                            static_cast<float>(kLutSize);
            const float s = (std::sin(t) + 1.0f) * 0.5f;
            return static_cast<uint8_t>(std::lround(s * 255.0f));
        }
    }
}

void SignalGenerator::fillLut(Waveform waveform) {
    for (int i = 0; i < kLutSize; ++i) {
        lut_[i] = sampleAt(waveform, i);
    }
}

uint8_t SignalGenerator::scaleSample(uint8_t sample, uint16_t gainQ8) {
    const int centered = static_cast<int>(sample) - 128;
    const int scaled = 128 + ((centered * static_cast<int>(gainQ8)) >> 8);
    if (scaled < 0) {
        return 0;
    }
    if (scaled > 255) {
        return 255;
    }
    return static_cast<uint8_t>(scaled);
}

uint32_t SignalGenerator::freqToPhaseInc(float freqHz) {
    if (freqHz < 0.1f) {
        freqHz = 0.1f;
    }
    if (freqHz > 20999.0f) {
        freqHz = 20999.0f;
    }
    const double inc =
            (static_cast<double>(freqHz) / static_cast<double>(kSampleRateHz)) * 4294967296.0;
    if (inc < 1.0) {
        return 1;
    }
    return static_cast<uint32_t>(inc);
}

void SignalGenerator::renderPair(uint32_t phase, uint32_t phaseOffset, const uint8_t *lut,
                                 uint16_t gainQ8, bool analogPwm, bool sineNeg90, uint32_t pulseEnd,
                                 uint32_t scaleQ16, uint8_t *ch1, uint8_t *ch2) {
    uint32_t idx1 = phase >> kLutIndexShift;
    uint32_t idx2 = (phase + phaseOffset) >> kLutIndexShift;
    uint8_t raw1;
    uint8_t raw2;
    if (analogPwm) {
        if (pulseEnd == 0 || idx1 >= pulseEnd) {
            raw1 = 0;
        } else {
            uint32_t src1 = (idx1 * scaleQ16) >> 16;
            if (src1 >= static_cast<uint32_t>(kLutSize)) {
                src1 = static_cast<uint32_t>(kLutSize - 1);
            }
            // sin(A - 90°) ≡ LUT index shifted by -kLutQuarter (both channels).
            if (sineNeg90) {
                src1 = (src1 + static_cast<uint32_t>(kLutSize - kLutQuarter)) &
                       static_cast<uint32_t>(kLutSize - 1);
            }
            raw1 = lut[src1];
        }
        if (pulseEnd == 0 || idx2 >= pulseEnd) {
            raw2 = 0;
        } else {
            uint32_t src2 = (idx2 * scaleQ16) >> 16;
            if (src2 >= static_cast<uint32_t>(kLutSize)) {
                src2 = static_cast<uint32_t>(kLutSize - 1);
            }
            if (sineNeg90) {
                src2 = (src2 + static_cast<uint32_t>(kLutSize - kLutQuarter)) &
                       static_cast<uint32_t>(kLutSize - 1);
            }
            raw2 = lut[src2];
        }
    } else {
        raw1 = lut[idx1];
        raw2 = lut[idx2];
    }
    *ch1 = scaleSample(raw1, gainQ8);
    *ch2 = scaleSample(raw2, gainQ8);
}

void SignalGenerator::fillDmaChunk(uint8_t *dst, size_t byteCount) {
    if (dst == nullptr || byteCount < 2) {
        return;
    }

    if (paused_) {
        std::memset(dst, kMidscale, byteCount);
        return;
    }

    const uint16_t gain = ampGainQ8_;
    const uint32_t offset = phaseOffset_;
    const bool analogPwm = analogPwm_;
    const bool sineNeg90 = analogPwmSineNeg90_;
    const uint32_t pulseEnd = analogPwmPulseEnd_;
    const uint32_t scaleQ16 = analogPwmScaleQ16_;
    const uint32_t phaseInc = phaseInc_;

    uint32_t phase = phase_;
    // ALTER layout: [ch1, ch2, ch1, ch2, ...]
    const size_t pairs = byteCount / 2;
    for (size_t i = 0; i < pairs; ++i) {
        uint8_t ch1;
        uint8_t ch2;
        renderPair(phase, offset, lut_, gain, analogPwm, sineNeg90, pulseEnd, scaleQ16, &ch1, &ch2);
        dst[i * 2] = ch1;
        dst[i * 2 + 1] = ch2;
        phase += phaseInc;
    }
    phase_ = phase;

    if ((byteCount & 1u) != 0u) {
        dst[byteCount - 1] = kMidscale;
    }
}

void SignalGenerator::refillTaskEntry(void *arg) {
    static_cast<SignalGenerator *>(arg)->refillTaskLoop();
}

void SignalGenerator::refillTaskLoop() {
    // Scratch for 8-bit interleaved samples; driver expands each byte to a 16-bit DMA slot
    // (CONFIG_DAC_DMA_AUTO_16BIT_ALIGN), so max source bytes = dma_buf_size / 2.
    uint8_t chunk[kDmaBufSize / 2];

    while (true) {
        dac_event_data_t evt{};
        if (xQueueReceive(dmaEventQue_, &evt, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (dac_ == nullptr || evt.buf == nullptr || evt.buf_size == 0) {
            continue;
        }

        size_t n = evt.buf_size / 2;
        if (n > sizeof(chunk)) {
            n = sizeof(chunk);
        }
        if (n < 2) {
            continue;
        }
        fillDmaChunk(chunk, n);

        size_t loaded = 0;
        const esp_err_t err = dac_continuous_write_asynchronously(
                dac_, static_cast<uint8_t *>(evt.buf), evt.buf_size, chunk, n, &loaded);
        if (err != ESP_OK) {
            // Avoid spamming Serial from a tight refill loop.
            static uint32_t lastLogMs = 0;
            const uint32_t now = millis();
            if (now - lastLogMs > 1000) {
                lastLogMs = now;
                Serial.printf("DAC DMA write_async failed: %s\n", esp_err_to_name(err));
            }
        }
    }
}

void SignalGenerator::begin() {
    waveform_ = Waveform::Sine;
    fillLut(waveform_);
    phase_ = 0;
    phaseInc_ = freqToPhaseInc(1.0f);
    phaseOffset_ = 0;
    ampGainQ8_ = 256;
    analogPwm_ = false;
    analogPwmSineNeg90_ = false;
    analogPwmPulseEnd_ = 0;
    analogPwmScaleQ16_ = 0;
    paused_ = true;

    dac_continuous_config_t contCfg = {
            .chan_mask = DAC_CHANNEL_MASK_ALL,
            .desc_num = kDmaDescNum,
            .buf_size = kDmaBufSize,
            .freq_hz = kDmaFreqHz,
            .offset = 0,
            .clk_src = DAC_DIGI_CLK_SRC_DEFAULT,
            // ALTER: buffer A B C D → CH0: A C …, CH1: B D … (independent phase).
            .chan_mode = DAC_CHANNEL_MODE_ALTER,
    };
    esp_err_t err = dac_continuous_new_channels(&contCfg, &dac_);
    if (err != ESP_OK) {
        Serial.printf("DAC DMA new_channels failed: %s\n", esp_err_to_name(err));
        dac_ = nullptr;
        return;
    }

    dmaEventQue_ = xQueueCreate(kDmaDescNum + 2, sizeof(dac_event_data_t));
    if (dmaEventQue_ == nullptr) {
        Serial.println("DAC DMA queue create failed");
        dac_continuous_del_channels(dac_);
        dac_ = nullptr;
        return;
    }

    dac_event_callbacks_t cbs = {
            .on_convert_done = onConvertDone,
            .on_stop = nullptr,
    };
    err = dac_continuous_register_event_callback(dac_, &cbs, dmaEventQue_);
    if (err != ESP_OK) {
        Serial.printf("DAC DMA register callback failed: %s\n", esp_err_to_name(err));
        vQueueDelete(dmaEventQue_);
        dmaEventQue_ = nullptr;
        dac_continuous_del_channels(dac_);
        dac_ = nullptr;
        return;
    }

    err = dac_continuous_enable(dac_);
    if (err != ESP_OK) {
        Serial.printf("DAC DMA enable failed: %s\n", esp_err_to_name(err));
        vQueueDelete(dmaEventQue_);
        dmaEventQue_ = nullptr;
        dac_continuous_del_channels(dac_);
        dac_ = nullptr;
        return;
    }

    // High priority so DMA buffers stay filled ahead of SPI / UI work.
    BaseType_t ok = xTaskCreatePinnedToCore(refillTaskEntry, "dac_dma_refill", 4096, this, 20,
                                            &refillTask_, 0);
    if (ok != pdPASS) {
        Serial.println("DAC DMA refill task create failed");
        dac_continuous_disable(dac_);
        vQueueDelete(dmaEventQue_);
        dmaEventQue_ = nullptr;
        dac_continuous_del_channels(dac_);
        dac_ = nullptr;
        refillTask_ = nullptr;
        return;
    }

    err = dac_continuous_start_async_writing(dac_);
    if (err != ESP_OK) {
        Serial.printf("DAC DMA start_async failed: %s\n", esp_err_to_name(err));
    } else {
        Serial.printf("DAC DMA ready: ALTER %lu Hz byte-rate (%lu Hz/ch)\n",
                      static_cast<unsigned long>(kDmaFreqHz),
                      static_cast<unsigned long>(kSampleRateHz));
    }
}

void SignalGenerator::pause() {
    paused_ = true;
}

void SignalGenerator::resume() {
    paused_ = false;
}

void SignalGenerator::setFrequency(float freqHz) {
    phaseInc_ = freqToPhaseInc(freqHz);
}

void SignalGenerator::setPhaseDeg(float phaseDeg) {
    // Phase of CH2 relative to CH1, degrees. Positive => CH2 leads CH1.
    if (phaseDeg < -360.0f) {
        phaseDeg = -360.0f;
    }
    if (phaseDeg > 360.0f) {
        phaseDeg = 360.0f;
    }
    // offset = φ/360 * 2^32 (wraps for negative φ)
    const int64_t ticks = std::llround(static_cast<double>(phaseDeg) * (4294967296.0 / 360.0));
    phaseOffset_ = static_cast<uint32_t>(ticks);
}

void SignalGenerator::setPhaseUs(int phaseUs, float freqHz) {
    // Phase of CH2 relative to CH1, microseconds. Positive => CH2 leads CH1.
    if (phaseUs < -9999) {
        phaseUs = -9999;
    }
    if (phaseUs > 9999) {
        phaseUs = 9999;
    }
    if (freqHz < 0.1f) {
        freqHz = 0.1f;
    }
    // fraction of period = Δt * f; offset = fraction * 2^32
    const double fraction = static_cast<double>(phaseUs) * 1.0e-6 * static_cast<double>(freqHz);
    const int64_t ticks = std::llround(fraction * 4294967296.0);
    phaseOffset_ = static_cast<uint32_t>(ticks);
}

void SignalGenerator::setWaveform(Waveform waveform) {
    if (waveform == waveform_) {
        return;
    }
    // Caller pauses DMA mute around apply, so refill will not read lut_ mid-rewrite.
    fillLut(waveform);
    waveform_ = waveform;
}

void SignalGenerator::setAnalogPwmDuty(float dutyPercent) {
    if (dutyPercent < 0.0f) {
        dutyPercent = 0.0f;
    }
    if (dutyPercent > 100.0f) {
        dutyPercent = 100.0f;
    }

    int n = static_cast<int>(std::lround(dutyPercent / 100.0f * static_cast<float>(kLutSize)));
    if (n < 0) {
        n = 0;
    }
    if (n > kLutSize) {
        n = kLutSize;
    }

    analogPwmPulseEnd_ = static_cast<uint32_t>(n);
    if (n <= 0) {
        analogPwmScaleQ16_ = 0;
    } else {
        analogPwmScaleQ16_ =
                static_cast<uint32_t>((static_cast<uint64_t>(kLutSize) << 16) / static_cast<uint32_t>(n));
    }
    analogPwm_ = true;
}

void SignalGenerator::setAmplitudeVolts(float volts) {
    if (volts < 0.0f) {
        volts = 0.0f;
    }
    if (volts > kDacFullScaleV) {
        volts = kDacFullScaleV;
    }
    const float gain = volts / kDacFullScaleV;
    uint16_t q8 = static_cast<uint16_t>(std::lround(gain * 256.0f));
    if (q8 > 256) {
        q8 = 256;
    }
    ampGainQ8_ = q8;
}

void SignalGenerator::apply(const ParamSnapshot &params) {
    // Keep lut_ matched to the menu waveform for TFT preview even when muted.
    setWaveform(params.waveform);

    if (!params.signalEnabled) {
        pause();
        return;
    }

    setFrequency(params.freqHz);
    setAmplitudeVolts(params.ampVolts);
    if (params.dacMode == DacMode::AnalogPwm) {
        setPhaseUs(params.phaseShiftUs, params.freqHz);
        setAnalogPwmDuty(params.dutyPercent);
        analogPwmSineNeg90_ = (params.waveform == Waveform::Sine);
    } else {
        setPhaseDeg(params.phaseDegTotal);
        analogPwm_ = false;
        analogPwmSineNeg90_ = false;
        analogPwmPulseEnd_ = 0;
        analogPwmScaleQ16_ = 0;
    }
}

void SignalGenerator::fillPeriodPreview(const ParamSnapshot &params, uint8_t *ch1, uint8_t *ch2,
                                        int count) const {
    if (ch1 == nullptr || ch2 == nullptr || count <= 0) {
        return;
    }

    // Uses lut_ filled by setWaveform/apply (called before preview in applyAndPaint).
    const uint8_t *lut = lut_;

    float volts = params.ampVolts;
    if (volts < 0.0f) {
        volts = 0.0f;
    }
    if (volts > kDacFullScaleV) {
        volts = kDacFullScaleV;
    }
    uint16_t gainQ8 = static_cast<uint16_t>(std::lround((volts / kDacFullScaleV) * 256.0f));
    if (gainQ8 > 256) {
        gainQ8 = 256;
    }

    uint32_t phaseOffset = 0;
    bool analogPwm = false;
    bool sineNeg90 = false;
    uint32_t pulseEnd = 0;
    uint32_t scaleQ16 = 0;

    if (params.dacMode == DacMode::AnalogPwm) {
        analogPwm = true;
        sineNeg90 = (params.waveform == Waveform::Sine);

        int phaseUs = params.phaseShiftUs;
        if (phaseUs < -9999) {
            phaseUs = -9999;
        }
        if (phaseUs > 9999) {
            phaseUs = 9999;
        }
        float freqHz = params.freqHz;
        if (freqHz < 0.1f) {
            freqHz = 0.1f;
        }
        const double fraction =
                static_cast<double>(phaseUs) * 1.0e-6 * static_cast<double>(freqHz);
        phaseOffset = static_cast<uint32_t>(std::llround(fraction * 4294967296.0));

        float duty = params.dutyPercent;
        if (duty < 0.0f) {
            duty = 0.0f;
        }
        if (duty > 100.0f) {
            duty = 100.0f;
        }
        int n = static_cast<int>(std::lround(duty / 100.0f * static_cast<float>(kLutSize)));
        if (n < 0) {
            n = 0;
        }
        if (n > kLutSize) {
            n = kLutSize;
        }
        pulseEnd = static_cast<uint32_t>(n);
        if (n > 0) {
            scaleQ16 = static_cast<uint32_t>((static_cast<uint64_t>(kLutSize) << 16) /
                                             static_cast<uint32_t>(n));
        }
    } else {
        float phaseDeg = params.phaseDegTotal;
        if (phaseDeg < -360.0f) {
            phaseDeg = -360.0f;
        }
        if (phaseDeg > 360.0f) {
            phaseDeg = 360.0f;
        }
        phaseOffset = static_cast<uint32_t>(
                std::llround(static_cast<double>(phaseDeg) * (4294967296.0 / 360.0)));
    }

    if (params.plotRealWaveform) {
        const uint32_t phaseInc = freqToPhaseInc(params.freqHz);
        // Real DAC samples in one period at Fs (e.g. 400 kHz / 10 kHz → 40 steps).
        float freqHz = params.freqHz;
        if (freqHz < 0.1f) {
            freqHz = 0.1f;
        }
        int nSamples =
                static_cast<int>(std::lround(static_cast<double>(kSampleRateHz) / freqHz));
        if (nSamples < 1) {
            nSamples = 1;
        }

        for (int i = 0; i < count; ++i) {
            // Nearest-neighbor map of N period samples onto plot width → sample-and-hold stairs.
            const uint32_t si =
                    (count <= 1)
                            ? 0u
                            : static_cast<uint32_t>((static_cast<uint64_t>(i) *
                                                     static_cast<uint32_t>(nSamples)) /
                                                    static_cast<uint32_t>(count));
            const uint32_t phase = si * phaseInc;
            renderPair(phase, phaseOffset, lut, gainQ8, analogPwm, sineNeg90, pulseEnd, scaleQ16,
                       &ch1[i], &ch2[i]);
        }
    } else {
        // Ideal: evenly spaced phase over one period (smooth LUT curve).
        for (int i = 0; i < count; ++i) {
            const uint32_t phase =
                    (count == 1) ? 0u
                                 : static_cast<uint32_t>((static_cast<uint64_t>(i) << 32) /
                                                        static_cast<uint32_t>(count));
            renderPair(phase, phaseOffset, lut, gainQ8, analogPwm, sineNeg90, pulseEnd, scaleQ16,
                       &ch1[i], &ch2[i]);
        }
    }
}
