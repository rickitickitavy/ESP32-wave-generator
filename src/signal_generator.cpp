#include "signal_generator.h"

#include <cmath>

#include "driver/dac_oneshot.h"
#include "hal/dac_ll.h"
#include "hal/dac_types.h"
#include "pins.h"

namespace {
    dac_oneshot_handle_t gDacCh1 = nullptr;
    dac_oneshot_handle_t gDacCh2 = nullptr;

    void initDacChannels() {
        if (gDacCh1 == nullptr) {
            dac_oneshot_config_t cfg = {.chan_id = DAC_CHAN_0};
            ESP_ERROR_CHECK(dac_oneshot_new_channel(&cfg, &gDacCh1));
        }
        if (gDacCh2 == nullptr) {
            dac_oneshot_config_t cfg = {.chan_id = DAC_CHAN_1};
            ESP_ERROR_CHECK(dac_oneshot_new_channel(&cfg, &gDacCh2));
        }
        // Avoid ADC RTC sampling glitching DAC output.
        dac_ll_rtc_sync_by_adc(false);
    }

    inline void IRAM_ATTR writeDacs(uint8_t ch1, uint8_t ch2) {
        // Direct register poke — keeps CH1/CH2 nearly simultaneous.
        // (dacWrite/dac_oneshot_output_voltage is ~7–11 µs each and skews phase.)
        dac_ll_update_output_value(DAC_CHAN_0, ch1);
        dac_ll_update_output_value(DAC_CHAN_1, ch2);
    }
} // namespace

void SignalGenerator::buildLuts() {
    for (int i = 0; i < kLutSize; ++i) {
        const float t =
                (2.0f * static_cast<float>(M_PI) * static_cast<float>(i)) / static_cast<float>(kLutSize);
        const float s = (std::sin(t) + 1.0f) * 0.5f;
        lutSine_[i] = static_cast<uint8_t>(std::lround(s * 255.0f));

        int tri;
        if (i < kLutSize / 2) {
            tri = (i * 255 * 2) / (kLutSize / 2);
            if (tri > 255) {
                tri = 255;
            }
        } else {
            tri = 255 - (((i - kLutSize / 2) * 255 * 2) / (kLutSize / 2));
            if (tri < 0) {
                tri = 0;
            }
        }
        lutTriangle_[i] = static_cast<uint8_t>(tri);
        lutRect_[i] = (i < kLutSize / 2) ? 255 : 0;
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

void SignalGenerator::onTimer() {
    const uint32_t phase = phase_;
    phase_ = phase + phaseInc_;

    const uint8_t *lut = lut_;
    const uint16_t gain = ampGainQ8_;
    // phaseOffset_: phase of CH2 relative to CH1 (positive => CH2 leads CH1).
    const uint32_t offset = phaseOffset_;
    const bool analogPwm = analogPwm_;
    const bool sineNeg90 = analogPwmSineNeg90_;
    const uint32_t pulseEnd = analogPwmPulseEnd_;
    const uint32_t scaleQ16 = analogPwmScaleQ16_;

    uint32_t idx1 = phase >> kLutIndexShift;
    uint32_t idx2 = (phase + offset) >> kLutIndexShift;
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

    writeDacs(scaleSample(raw1, gain), scaleSample(raw2, gain));
}

void SignalGenerator::begin(void (*timerIsr)()) {
    buildLuts();
    lut_ = lutSine_;
    phase_ = 0;
    phaseInc_ = freqToPhaseInc(1.0f);
    phaseOffset_ = 0;
    ampGainQ8_ = 256;
    analogPwm_ = false;
    analogPwmSineNeg90_ = false;
    analogPwmPulseEnd_ = 0;
    analogPwmScaleQ16_ = 0;

    initDacChannels();

    // Arduino-ESP32 3.x: timer frequency in Hz; alarm period → kSampleRateHz
    timer_ = timerBegin(1000000);
    timerAttachInterrupt(timer_, timerIsr);
    timerAlarm(timer_, kTimerAlarmUs, true, 0);
}

void SignalGenerator::pause() {
    if (timer_ != nullptr) {
        timerStop(timer_);
    }
}

void SignalGenerator::resume() {
    if (timer_ != nullptr) {
        timerStart(timer_);
    }
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

const uint8_t *SignalGenerator::baseLut(Waveform waveform) const {
    switch (waveform) {
        case Waveform::Rectangular:
            return lutRect_;
        case Waveform::Triangle:
            return lutTriangle_;
        case Waveform::Sine:
        default:
            return lutSine_;
    }
}

void SignalGenerator::setWaveform(Waveform waveform) {
    lut_ = baseLut(waveform);
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
    if (!params.signalEnabled) {
        pause();
        writeDacs(128, 128);
        return;
    }

    setFrequency(params.freqHz);
    setAmplitudeVolts(params.ampVolts);
    setWaveform(params.waveform);
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
