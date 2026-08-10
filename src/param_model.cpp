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

    constexpr FocusField kTopFields[] = {
            FocusField::GroupSignal,
            FocusField::GroupPwm,
    };

    constexpr FocusField kSignalFields[] = {
            FocusField::SigBack,    FocusField::SigEnabled, FocusField::FreqKHz,
            FocusField::FreqHundredHz, FocusField::FreqTensHz, FocusField::FreqHz,
            FocusField::PhaseTens,  FocusField::PhaseDeg,   FocusField::PhaseFine,
            FocusField::Waveform,   FocusField::Amplitude,
    };

    constexpr FocusField kPwmFields[] = {
            FocusField::PwmBack,  FocusField::PwmEnabled, FocusField::PwmFreq,
            FocusField::PwmCh1X20, FocusField::PwmCh1X1,  FocusField::PwmCh2X20,
            FocusField::PwmCh2X1,
    };

    int indexOfField(const FocusField *fields, int count, FocusField field) {
        for (int i = 0; i < count; ++i) {
            if (fields[i] == field) {
                return i;
            }
        }
        return 0;
    }
} // namespace

const FocusField *menuFields(MenuLevel level, int &count) {
    switch (level) {
        case MenuLevel::Signal:
            count = static_cast<int>(sizeof(kSignalFields) / sizeof(kSignalFields[0]));
            return kSignalFields;
        case MenuLevel::Pwm:
            count = static_cast<int>(sizeof(kPwmFields) / sizeof(kPwmFields[0]));
            return kPwmFields;
        case MenuLevel::Top:
        default:
            count = static_cast<int>(sizeof(kTopFields) / sizeof(kTopFields[0]));
            return kTopFields;
    }
}

void ParamModel::begin() {
    state_ = ParamSnapshot{};
    recompute();
}

void ParamModel::recompute() {
    float f = static_cast<float>(state_.freqKHz) * 1000.0f +
              static_cast<float>(state_.freqHundredHz) * 100.0f +
              static_cast<float>(state_.freqTensHz) * 10.0f + state_.freqHzPart;
    // Max: 19*1000 + 9*100 + 100*10 + 99 = 20999
    state_.freqHz = clampf(f, 0.1f, 20999.0f);

    float p = static_cast<float>(state_.phaseTens) * 10.0f + static_cast<float>(state_.phaseDeg) +
              state_.phaseFine;
    state_.phaseDegTotal = clampf(p, -360.0f, 360.0f);
    state_.ampVolts = clampf(state_.ampVolts, 0.0f, 3.3f);

    state_.pwmFreqX10 = clampi(state_.pwmFreqX10, 2, 300);
    state_.pwmCh1X20 = clampi(state_.pwmCh1X20, 0, 2499);
    state_.pwmCh1X1 = clampi(state_.pwmCh1X1, 0, 19);
    state_.pwmCh2X20 = clampi(state_.pwmCh2X20, 0, 2499);
    state_.pwmCh2X1 = clampi(state_.pwmCh2X1, 0, 19);

    state_.pwmHz = static_cast<float>(state_.pwmFreqX10) * 10.0f;
    state_.pwmCh1Us = state_.pwmCh1X20 * 20 + state_.pwmCh1X1;
    state_.pwmCh2Us = state_.pwmCh2X20 * 20 + state_.pwmCh2X1;
}

void ParamModel::toggleEdit() {
    switch (state_.focus) {
        case FocusField::GroupSignal:
            state_.menu = MenuLevel::Signal;
            state_.focus = FocusField::SigEnabled;
            state_.editing = false;
            return;
        case FocusField::GroupPwm:
            state_.menu = MenuLevel::Pwm;
            state_.focus = FocusField::PwmEnabled;
            state_.editing = false;
            return;
        case FocusField::SigBack:
            state_.menu = MenuLevel::Top;
            state_.focus = FocusField::GroupSignal;
            state_.editing = false;
            return;
        case FocusField::PwmBack:
            state_.menu = MenuLevel::Top;
            state_.focus = FocusField::GroupPwm;
            state_.editing = false;
            return;
        default:
            state_.editing = !state_.editing;
            break;
    }
}

void ParamModel::moveFocus(int steps) {
    if (steps == 0 || state_.editing) {
        return;
    }

    int count = 0;
    const FocusField *fields = menuFields(state_.menu, count);
    if (count <= 0) {
        return;
    }

    int idx = indexOfField(fields, count, state_.focus);
    int next = (idx + steps) % count;
    if (next < 0) {
        next += count;
    }
    state_.focus = fields[next];
}

void ParamModel::applyEncoderDelta(int steps) {
    if (steps == 0) {
        return;
    }

    switch (state_.focus) {
        case FocusField::GroupSignal:
        case FocusField::GroupPwm:
        case FocusField::SigBack:
        case FocusField::PwmBack:
            break;
        case FocusField::SigEnabled:
            state_.signalEnabled = !state_.signalEnabled;
            break;
        case FocusField::PwmEnabled:
            state_.pwmEnabled = !state_.pwmEnabled;
            break;
        case FocusField::FreqKHz:
            state_.freqKHz = clampi(state_.freqKHz + steps, 0, 19);
            break;
        case FocusField::FreqHundredHz:
            state_.freqHundredHz = clampi(state_.freqHundredHz + steps, 0, 9);
            break;
        case FocusField::FreqTensHz:
            state_.freqTensHz = clampi(state_.freqTensHz + steps, 0, 100);
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
            state_.phaseDeg = clampi(state_.phaseDeg + steps, -90, 90);
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
        case FocusField::PwmFreq:
            state_.pwmFreqX10 = clampi(state_.pwmFreqX10 + steps, 2, 300);
            break;
        case FocusField::PwmCh1X20:
            state_.pwmCh1X20 = clampi(state_.pwmCh1X20 + steps, 0, 2499);
            break;
        case FocusField::PwmCh1X1:
            state_.pwmCh1X1 = clampi(state_.pwmCh1X1 + steps, 0, 19);
            break;
        case FocusField::PwmCh2X20:
            state_.pwmCh2X20 = clampi(state_.pwmCh2X20 + steps, 0, 2499);
            break;
        case FocusField::PwmCh2X1:
            state_.pwmCh2X1 = clampi(state_.pwmCh2X1 + steps, 0, 19);
            break;
        case FocusField::Count:
            break;
    }

    recompute();
}

const ParamSnapshot &ParamModel::snapshot() const {
    return state_;
}
