#include "signal_generator.h"

#include <cmath>

#include "pins.h"

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
    if (freqHz > 8000.0f) {
        freqHz = 8000.0f;
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
    const uint32_t offset = phaseOffset_;

    const uint8_t s1 = lut[phase >> 24];
    const uint8_t s2 = lut[(phase + offset) >> 24];
    dacWrite(PIN_DAC_CH1, scaleSample(s1, gain));
    dacWrite(PIN_DAC_CH2, scaleSample(s2, gain));
}

void SignalGenerator::begin(void (*timerIsr)()) {
    buildLuts();
    lut_ = lutSine_;
    phase_ = 0;
    phaseInc_ = freqToPhaseInc(1.0f);
    phaseOffset_ = 0;
    ampGainQ8_ = 256;

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
    if (phaseDeg < -360.0f) {
        phaseDeg = -360.0f;
    }
    if (phaseDeg > 360.0f) {
        phaseDeg = 360.0f;
    }
    const double frac = static_cast<double>(phaseDeg) / 360.0;
    const int64_t raw = static_cast<int64_t>(frac * 4294967296.0);
    phaseOffset_ = static_cast<uint32_t>(raw);
}

void SignalGenerator::setWaveform(Waveform waveform) {
    switch (waveform) {
        case Waveform::Rectangular:
            lut_ = lutRect_;
            break;
        case Waveform::Triangle:
            lut_ = lutTriangle_;
            break;
        case Waveform::Sine:
        default:
            lut_ = lutSine_;
            break;
    }
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
    setFrequency(params.freqHz);
    setPhaseDeg(params.phaseDegTotal);
    setWaveform(params.waveform);
    setAmplitudeVolts(params.ampVolts);
}
