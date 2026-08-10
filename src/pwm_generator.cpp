#include "pwm_generator.h"

#include "pins.h"

void PwmGenerator::begin() {
    const uint32_t freqHz = 100;
    ledcAttach(PIN_PWM_CH1, freqHz, kResolutionBits);
    ledcAttach(PIN_PWM_CH2, freqHz, kResolutionBits);
    lastHz_ = static_cast<float>(freqHz);
    lastCh1Us_ = UINT32_MAX;
    lastCh2Us_ = UINT32_MAX;
}

void PwmGenerator::setChannelPulseUs(uint8_t pin, uint32_t pulseUs, float pwmHz) {
    if (pwmHz < 1.0f) {
        pwmHz = 1.0f;
    }
    const float periodUs = 1000000.0f / pwmHz;
    uint32_t clamped = pulseUs;
    if (static_cast<float>(clamped) > periodUs) {
        clamped = static_cast<uint32_t>(periodUs);
    }

    uint32_t duty = 0;
    if (clamped > 0 && periodUs > 0.0f) {
        const double d = (static_cast<double>(clamped) / static_cast<double>(periodUs)) *
                         static_cast<double>(kMaxDuty);
        duty = static_cast<uint32_t>(d);
        if (duty >= kMaxDuty) {
            duty = kMaxDuty - 1;
        }
        if (duty == 0) {
            duty = 1;
        }
    }
    ledcWrite(pin, duty);
}

void PwmGenerator::apply(const ParamSnapshot &params) {
    if (!params.pwmEnabled) {
        ledcWrite(PIN_PWM_CH1, 0);
        ledcWrite(PIN_PWM_CH2, 0);
        lastCh1Us_ = UINT32_MAX;
        lastCh2Us_ = UINT32_MAX;
        return;
    }

    float hz = params.pwmHz;
    if (hz < 20.0f) {
        hz = 20.0f;
    }
    if (hz > 3000.0f) {
        hz = 3000.0f;
    }

    const uint32_t ch1Us = params.pwmCh1Us > 0 ? static_cast<uint32_t>(params.pwmCh1Us) : 0;
    const uint32_t ch2Us = params.pwmCh2Us > 0 ? static_cast<uint32_t>(params.pwmCh2Us) : 0;

    const bool freqChanged = hz != lastHz_;
    if (freqChanged) {
        ledcChangeFrequency(PIN_PWM_CH1, static_cast<uint32_t>(hz), kResolutionBits);
        ledcChangeFrequency(PIN_PWM_CH2, static_cast<uint32_t>(hz), kResolutionBits);
        lastHz_ = hz;
    }

    if (freqChanged || ch1Us != lastCh1Us_) {
        setChannelPulseUs(PIN_PWM_CH1, ch1Us, hz);
        lastCh1Us_ = ch1Us;
    }
    if (freqChanged || ch2Us != lastCh2Us_) {
        setChannelPulseUs(PIN_PWM_CH2, ch2Us, hz);
        lastCh2Us_ = ch2Us;
    }
}
