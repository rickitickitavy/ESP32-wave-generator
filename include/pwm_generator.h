#pragma once

#include <Arduino.h>

#include "param_model.h"

class PwmGenerator {
public:
    void begin();
    void apply(const ParamSnapshot &params);

private:
    void setChannelPulseUs(uint8_t pin, uint32_t pulseUs, float pwmHz);

    static constexpr uint8_t kResolutionBits = 14;
    static constexpr uint32_t kMaxDuty = (1u << kResolutionBits);

    float lastHz_ = -1.0f;
    uint32_t lastCh1Us_ = UINT32_MAX;
    uint32_t lastCh2Us_ = UINT32_MAX;
};
