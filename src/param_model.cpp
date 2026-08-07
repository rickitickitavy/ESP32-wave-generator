#include "param_model.h"

#include <cmath>

namespace {
    float clampf(float v, float lo, float hi) {
        if (v < lo) {
            return lo;
        }
        if (v > hi) {
            return hi;
        }
        return v;
    }

    int clampi(int v, int lo, int hi) {
        if (v < lo) {
            return lo;
        }
        if (v > hi) {
            return hi;
        }
        return v;
    }

    float roundToStep(float v, float step) {
        return std::round(v / step) * step;
    }
} // namespace

void ParamModel::begin() {
    state_ = ParamSnapshot{};
    state_.freqHzPart = 1.0f;
    state_.ampVolts = 3.3f;
    state_.waveform = Waveform::Sine;
    state_.focus = FocusField::FreqHz;
    recompute();
}

void ParamModel::recompute() {
    float f = static_cast<float>(state_.freqKHz) * 1000.0f +
              static_cast<float>(state_.freqHundredHz) * 100.0f + state_.freqHzPart;
    state_.freqHz = clampf(f, 0.1f, 8000.0f);

    float p = static_cast<float>(state_.phaseTens) * 10.0f + static_cast<float>(state_.phaseDeg) +
              state_.phaseFine;
    state_.phaseDegTotal = clampf(p, -360.0f, 360.0f);
    state_.ampVolts = clampf(state_.ampVolts, 0.0f, 3.3f);
}

void ParamModel::nextFocus() {
    const int next = (static_cast<int>(state_.focus) + 1) % static_cast<int>(FocusField::Count);
    state_.focus = static_cast<FocusField>(next);
}

void ParamModel::applyEncoderDelta(int steps) {
    if (steps == 0) {
        return;
    }

    switch (state_.focus) {
        case FocusField::FreqKHz:
            state_.freqKHz = clampi(state_.freqKHz + steps, 0, 7);
            break;
        case FocusField::FreqHundredHz:
            state_.freqHundredHz = clampi(state_.freqHundredHz + steps, 0, 9);
            break;
        case FocusField::FreqHz: {
            float v = state_.freqHzPart + static_cast<float>(steps) * 0.1f;
            v = roundToStep(v, 0.1f);
            state_.freqHzPart = clampf(v, 0.1f, 99.0f);
            break;
        }
        case FocusField::PhaseTens:
            state_.phaseTens = clampi(state_.phaseTens + steps, -35, 35);
            break;
        case FocusField::PhaseDeg:
            state_.phaseDeg = clampi(state_.phaseDeg + steps, -9, 9);
            break;
        case FocusField::PhaseFine: {
            float v = state_.phaseFine + static_cast<float>(steps) * 0.01f;
            v = roundToStep(v, 0.01f);
            state_.phaseFine = clampf(v, -1.0f, 1.0f);
            break;
        }
        case FocusField::Waveform: {
            int w = static_cast<int>(state_.waveform) + steps;
            const int count = static_cast<int>(Waveform::Count);
            w %= count;
            if (w < 0) {
                w += count;
            }
            state_.waveform = static_cast<Waveform>(w);
            break;
        }
        case FocusField::Amplitude: {
            float v = state_.ampVolts + static_cast<float>(steps) * 0.1f;
            v = roundToStep(v, 0.1f);
            state_.ampVolts = clampf(v, 0.0f, 3.3f);
            break;
        }
        case FocusField::Count:
            break;
    }

    recompute();
}

const ParamSnapshot &ParamModel::snapshot() const {
    return state_;
}
