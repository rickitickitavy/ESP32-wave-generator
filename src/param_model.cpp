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

    void splitPulseUs(int pulseUs, int &x100, int &x10, int &x1) {
        pulseUs = clampi(pulseUs, 0, 9999999);
        x100 = pulseUs / 100;
        const int rem = pulseUs % 100;
        x10 = rem / 10;
        x1 = rem % 10;
    }

    void splitDutyPercent(float dutyPercent, int &d10, int &d1, int &dTenths) {
        dutyPercent = clampf(dutyPercent, 0.0f, 100.0f);
        dutyPercent = roundToStep(dutyPercent, 0.1f);
        if (dutyPercent >= 100.0f) {
            d10 = 10;
            d1 = 0;
            dTenths = 0;
            return;
        }
        const int tenths = static_cast<int>(std::lround(dutyPercent * 10.0f));
        d10 = tenths / 100;
        int rem = tenths % 100;
        d1 = rem / 10;
        dTenths = rem % 10;
    }

    float dutyFromDigits(int d10, int d1, int dTenths) {
        if (d10 >= 10) {
            return 100.0f;
        }
        return static_cast<float>(d10) * 10.0f + static_cast<float>(d1) +
               static_cast<float>(dTenths) * 0.1f;
    }

    int pulseFromDigits(int x100, int x10, int x1) {
        return x100 * 100 + x10 * 10 + x1;
    }

    constexpr FocusField kTopFields[] = {
            FocusField::GroupSignal,
            FocusField::GroupPwm,
    };

    constexpr FocusField kSignalOscFields[] = {
            FocusField::SigBack,  FocusField::SigEnabled, FocusField::SigMode,
            FocusField::Waveform, FocusField::Amplitude,  FocusField::GroupFreq,
            FocusField::GroupPhase,
    };

    constexpr FocusField kSignalAnalogPwmFields[] = {
            FocusField::SigBack,      FocusField::SigEnabled, FocusField::SigMode,
            FocusField::Waveform,     FocusField::Amplitude,  FocusField::GroupFreq,
            FocusField::GroupShiftUs, FocusField::GroupPulse, FocusField::GroupDuty,
    };

    constexpr FocusField kSigFreqFields[] = {
            FocusField::FreqBack,      FocusField::FreqKHz, FocusField::FreqHundredHz,
            FocusField::FreqTensHz,    FocusField::FreqHz,
    };

    constexpr FocusField kSigPhaseFields[] = {
            FocusField::PhaseBack, FocusField::PhaseTens, FocusField::PhaseDeg,
            FocusField::PhaseFine,
    };

    constexpr FocusField kSigShiftUsFields[] = {
            FocusField::ShiftUsBack, FocusField::PhaseUs1000, FocusField::PhaseUs100,
            FocusField::PhaseUs10,   FocusField::PhaseUs1,
    };

    constexpr FocusField kSigPulseFields[] = {
            FocusField::PulseBack, FocusField::PulseUs100, FocusField::PulseUs10,
            FocusField::PulseUs1,
    };

    constexpr FocusField kSigDutyFields[] = {
            FocusField::DutyBack, FocusField::Duty10, FocusField::Duty1,
            FocusField::DutyTenths,
    };

    constexpr FocusField kPwmFields[] = {
            FocusField::PwmBack,   FocusField::PwmEnabled, FocusField::PwmFreq,
            FocusField::PwmCh1X20, FocusField::PwmCh1X1,   FocusField::PwmCh2X20,
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

const FocusField *menuFields(const ParamSnapshot &state, int &count) {
    switch (state.menu) {
        case MenuLevel::Signal:
            if (state.dacMode == DacMode::AnalogPwm) {
                count = static_cast<int>(sizeof(kSignalAnalogPwmFields) /
                                         sizeof(kSignalAnalogPwmFields[0]));
                return kSignalAnalogPwmFields;
            }
            count = static_cast<int>(sizeof(kSignalOscFields) / sizeof(kSignalOscFields[0]));
            return kSignalOscFields;
        case MenuLevel::SigFreq:
            count = static_cast<int>(sizeof(kSigFreqFields) / sizeof(kSigFreqFields[0]));
            return kSigFreqFields;
        case MenuLevel::SigPhase:
            count = static_cast<int>(sizeof(kSigPhaseFields) / sizeof(kSigPhaseFields[0]));
            return kSigPhaseFields;
        case MenuLevel::SigShiftUs:
            count = static_cast<int>(sizeof(kSigShiftUsFields) / sizeof(kSigShiftUsFields[0]));
            return kSigShiftUsFields;
        case MenuLevel::SigPulse:
            count = static_cast<int>(sizeof(kSigPulseFields) / sizeof(kSigPulseFields[0]));
            return kSigPulseFields;
        case MenuLevel::SigDuty:
            count = static_cast<int>(sizeof(kSigDutyFields) / sizeof(kSigDutyFields[0]));
            return kSigDutyFields;
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
    pulseDutyAnchor_ = PulseDutyAnchor::Duty;
    recompute();
}

void ParamModel::ensureFocusVisibleInMenu() {
    int count = 0;
    const FocusField *fields = menuFields(state_, count);
    for (int i = 0; i < count; ++i) {
        if (fields[i] == state_.focus) {
            return;
        }
    }
    state_.focus = FocusField::SigMode;
    state_.editing = false;
}

void ParamModel::syncPulseFromDutyPercent(float periodUs, int maxPulseUs) {
    state_.dutyPercent = clampf(state_.dutyPercent, 0.0f, 100.0f);
    state_.pulseUs = static_cast<int>(std::lround(state_.dutyPercent / 100.0f * periodUs));
    state_.pulseUs = clampi(state_.pulseUs, 0, maxPulseUs);
    splitPulseUs(state_.pulseUs, state_.pulseUs100, state_.pulseUs10, state_.pulseUs1);
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

    state_.phaseUs1000 = clampi(state_.phaseUs1000, -9, 9);
    state_.phaseUs100 = clampi(state_.phaseUs100, -9, 9);
    state_.phaseUs10 = clampi(state_.phaseUs10, -9, 9);
    state_.phaseUs1 = clampi(state_.phaseUs1, -9, 9);
    state_.phaseShiftUs = clampi(state_.phaseUs1000 * 1000 + state_.phaseUs100 * 100 +
                                         state_.phaseUs10 * 10 + state_.phaseUs1,
                                 -9999, 9999);

    state_.ampVolts = clampf(state_.ampVolts, 0.0f, 3.3f);

    const float periodUs = 1000000.0f / state_.freqHz;
    const int maxPulseUs = static_cast<int>(std::lround(periodUs));

    // Cross-link only the opposite group: duty edits rewrite pulse digits; pulse edits
    // rewrite duty digits. Digits on the edited side are left as the user set them.
    // Frequency changes keep dutyPercent and refresh pulse µs (Analog PWM).
    state_.pulseUs100 = clampi(state_.pulseUs100, 0, 99999);
    state_.pulseUs10 = clampi(state_.pulseUs10, 0, 9);
    state_.pulseUs1 = clampi(state_.pulseUs1, 0, 9);
    state_.duty10 = clampi(state_.duty10, 0, 10);
    state_.duty1 = clampi(state_.duty1, 0, 9);
    state_.dutyTenths = clampi(state_.dutyTenths, 0, 9);

    if (state_.dacMode == DacMode::AnalogPwm) {
        if (pulseDutyAnchor_ == PulseDutyAnchor::Pulse) {
            state_.pulseUs = pulseFromDigits(state_.pulseUs100, state_.pulseUs10, state_.pulseUs1);
            if (periodUs > 0.0f) {
                state_.dutyPercent = (static_cast<float>(state_.pulseUs) / periodUs) * 100.0f;
            } else {
                state_.dutyPercent = 0.0f;
            }
            state_.dutyPercent = clampf(state_.dutyPercent, 0.0f, 100.0f);
            splitDutyPercent(state_.dutyPercent, state_.duty10, state_.duty1, state_.dutyTenths);
            state_.pulseUs = clampi(state_.pulseUs, 0, maxPulseUs);
        } else if (pulseDutyAnchor_ == PulseDutyAnchor::FreqKeepDuty) {
            // Keep current duty; impulse length follows period at that duty.
            syncPulseFromDutyPercent(periodUs, maxPulseUs);
            pulseDutyAnchor_ = PulseDutyAnchor::Duty;
        } else {
            state_.dutyPercent =
                    dutyFromDigits(state_.duty10, state_.duty1, state_.dutyTenths);
            syncPulseFromDutyPercent(periodUs, maxPulseUs);
        }
    }

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
        case FocusField::GroupFreq:
            state_.menu = MenuLevel::SigFreq;
            state_.focus = FocusField::FreqKHz;
            state_.editing = false;
            return;
        case FocusField::GroupPhase:
            state_.menu = MenuLevel::SigPhase;
            state_.focus = FocusField::PhaseTens;
            state_.editing = false;
            return;
        case FocusField::GroupShiftUs:
            state_.menu = MenuLevel::SigShiftUs;
            state_.focus = FocusField::PhaseUs1000;
            state_.editing = false;
            return;
        case FocusField::GroupPulse:
            state_.menu = MenuLevel::SigPulse;
            state_.focus = FocusField::PulseUs100;
            state_.editing = false;
            return;
        case FocusField::GroupDuty:
            state_.menu = MenuLevel::SigDuty;
            state_.focus = FocusField::Duty10;
            state_.editing = false;
            return;
        case FocusField::SigBack:
            state_.menu = MenuLevel::Top;
            state_.focus = FocusField::GroupSignal;
            state_.editing = false;
            return;
        case FocusField::FreqBack:
            state_.menu = MenuLevel::Signal;
            state_.focus = FocusField::GroupFreq;
            state_.editing = false;
            return;
        case FocusField::PhaseBack:
            state_.menu = MenuLevel::Signal;
            state_.focus = FocusField::GroupPhase;
            state_.editing = false;
            return;
        case FocusField::ShiftUsBack:
            state_.menu = MenuLevel::Signal;
            state_.focus = FocusField::GroupShiftUs;
            state_.editing = false;
            return;
        case FocusField::PulseBack:
            state_.menu = MenuLevel::Signal;
            state_.focus = FocusField::GroupPulse;
            state_.editing = false;
            return;
        case FocusField::DutyBack:
            state_.menu = MenuLevel::Signal;
            state_.focus = FocusField::GroupDuty;
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
    const FocusField *fields = menuFields(state_, count);
    if (count <= 0) {
        return;
    }

    int idx = indexOfField(fields, count, state_.focus);
    // If focus is not in the current list, snap before moving.
    bool found = false;
    for (int i = 0; i < count; ++i) {
        if (fields[i] == state_.focus) {
            idx = i;
            found = true;
            break;
        }
    }
    if (!found) {
        ensureFocusVisibleInMenu();
        idx = indexOfField(fields, count, state_.focus);
    }

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
        case FocusField::GroupFreq:
        case FocusField::GroupPhase:
        case FocusField::GroupShiftUs:
        case FocusField::GroupPulse:
        case FocusField::GroupDuty:
        case FocusField::SigBack:
        case FocusField::FreqBack:
        case FocusField::PhaseBack:
        case FocusField::ShiftUsBack:
        case FocusField::PulseBack:
        case FocusField::DutyBack:
        case FocusField::PwmBack:
            break;
        case FocusField::SigEnabled:
            state_.signalEnabled = !state_.signalEnabled;
            break;
        case FocusField::PwmEnabled:
            state_.pwmEnabled = !state_.pwmEnabled;
            break;
        case FocusField::SigMode: {
            int m = static_cast<int>(state_.dacMode) + steps;
            const int count = static_cast<int>(DacMode::Count);
            m %= count;
            if (m < 0) {
                m += count;
            }
            const DacMode next = static_cast<DacMode>(m);
            if (next == DacMode::AnalogPwm && state_.dacMode != DacMode::AnalogPwm) {
                // Seed 50% duty; pulse derived from frequency.
                state_.duty10 = 5;
                state_.duty1 = 0;
                state_.dutyTenths = 0;
                pulseDutyAnchor_ = PulseDutyAnchor::Duty;
            }
            state_.dacMode = next;
            ensureFocusVisibleInMenu();
            break;
        }
        case FocusField::FreqKHz:
            state_.freqKHz = clampi(state_.freqKHz + steps, 0, 19);
            if (state_.dacMode == DacMode::AnalogPwm) {
                pulseDutyAnchor_ = PulseDutyAnchor::FreqKeepDuty;
            }
            break;
        case FocusField::FreqHundredHz:
            state_.freqHundredHz = clampi(state_.freqHundredHz + steps, 0, 9);
            if (state_.dacMode == DacMode::AnalogPwm) {
                pulseDutyAnchor_ = PulseDutyAnchor::FreqKeepDuty;
            }
            break;
        case FocusField::FreqTensHz:
            state_.freqTensHz = clampi(state_.freqTensHz + steps, 0, 100);
            if (state_.dacMode == DacMode::AnalogPwm) {
                pulseDutyAnchor_ = PulseDutyAnchor::FreqKeepDuty;
            }
            break;
        case FocusField::FreqHz: {
            float v = state_.freqHzPart + static_cast<float>(steps) * 0.1f;
            v = roundToStep(v, 0.1f);
            state_.freqHzPart = clampf(v, 0.1f, 99.0f);
            if (state_.dacMode == DacMode::AnalogPwm) {
                pulseDutyAnchor_ = PulseDutyAnchor::FreqKeepDuty;
            }
            break;
        }
        case FocusField::PhaseTens:
            state_.phaseTens = clampi(state_.phaseTens + steps, -35, 35);
            break;
        case FocusField::PhaseDeg:
            state_.phaseDeg = clampi(state_.phaseDeg + steps, -90, 90);
            break;
        case FocusField::PhaseFine: {
            float v = state_.phaseFine + static_cast<float>(steps) * 0.1f;
            v = roundToStep(v, 0.1f);
            state_.phaseFine = clampf(v, -1.0f, 1.0f);
            break;
        }
        case FocusField::PhaseUs1000:
            state_.phaseUs1000 = clampi(state_.phaseUs1000 + steps, -9, 9);
            break;
        case FocusField::PhaseUs100:
            state_.phaseUs100 = clampi(state_.phaseUs100 + steps, -9, 9);
            break;
        case FocusField::PhaseUs10:
            state_.phaseUs10 = clampi(state_.phaseUs10 + steps, -9, 9);
            break;
        case FocusField::PhaseUs1:
            state_.phaseUs1 = clampi(state_.phaseUs1 + steps, -9, 9);
            break;
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
        case FocusField::PulseUs100:
            state_.pulseUs100 = clampi(state_.pulseUs100 + steps, 0, 99999);
            pulseDutyAnchor_ = PulseDutyAnchor::Pulse;
            break;
        case FocusField::PulseUs10:
            state_.pulseUs10 = clampi(state_.pulseUs10 + steps, 0, 9);
            pulseDutyAnchor_ = PulseDutyAnchor::Pulse;
            break;
        case FocusField::PulseUs1:
            state_.pulseUs1 = clampi(state_.pulseUs1 + steps, 0, 9);
            pulseDutyAnchor_ = PulseDutyAnchor::Pulse;
            break;
        case FocusField::Duty10:
            state_.duty10 = clampi(state_.duty10 + steps, 0, 10);
            pulseDutyAnchor_ = PulseDutyAnchor::Duty;
            break;
        case FocusField::Duty1:
            state_.duty1 = clampi(state_.duty1 + steps, 0, 9);
            pulseDutyAnchor_ = PulseDutyAnchor::Duty;
            break;
        case FocusField::DutyTenths:
            state_.dutyTenths = clampi(state_.dutyTenths + steps, 0, 9);
            pulseDutyAnchor_ = PulseDutyAnchor::Duty;
            break;
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
    if (state_.menu == MenuLevel::Signal || state_.menu == MenuLevel::SigFreq ||
        state_.menu == MenuLevel::SigPhase || state_.menu == MenuLevel::SigShiftUs ||
        state_.menu == MenuLevel::SigPulse || state_.menu == MenuLevel::SigDuty) {
        ensureFocusVisibleInMenu();
    }
}

const ParamSnapshot &ParamModel::snapshot() const {
    return state_;
}
