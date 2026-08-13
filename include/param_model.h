#pragma once

#include <Arduino.h>

enum class Waveform : uint8_t {
    Sine = 0,
    Rectangular,
    Triangle,
    Count,
};

enum class DacMode : uint8_t {
    Oscillator = 0,
    AnalogPwm,
    Count,
};

enum class MenuLevel : uint8_t {
    Top = 0,
    Signal,
    Pwm,
};

enum class FocusField : uint8_t {
    GroupSignal = 0,
    GroupPwm,
    SigEnabled,
    SigMode,
    Waveform,
    Amplitude,
    FreqKHz,
    FreqHundredHz,
    FreqTensHz,
    FreqHz,
    PhaseTens,
    PhaseDeg,
    PhaseFine,
    PhaseUs1000,
    PhaseUs100,
    PhaseUs10,
    PhaseUs1,
    PulseUs100,
    PulseUs10,
    PulseUs1,
    Duty10,
    Duty1,
    DutyTenths,
    SigBack,
    PwmEnabled,
    PwmFreq,
    PwmCh1X20,
    PwmCh1X1,
    PwmCh2X20,
    PwmCh2X1,
    PwmBack,
    Count,
};

struct ParamSnapshot {
    int freqKHz = 0;
    int freqHundredHz = 3;
    int freqTensHz = 0;
    float freqHzPart = 0.0f;
    int phaseTens = 0;
    int phaseDeg = 0;
    float phaseFine = 0.0f;
    int phaseUs1000 = 0;
    int phaseUs100 = 0;
    int phaseUs10 = 0;
    int phaseUs1 = 0;
    Waveform waveform = Waveform::Sine;
    DacMode dacMode = DacMode::Oscillator;
    float ampVolts = 3.3f;

    int pulseUs100 = 0;
    int pulseUs10 = 0;
    int pulseUs1 = 0;
    int duty10 = 5;
    int duty1 = 0;
    int dutyTenths = 0;

    int pwmFreqX10 = 10;
    int pwmCh1X20 = 5;
    int pwmCh1X1 = 0;
    int pwmCh2X20 = 5;
    int pwmCh2X1 = 0;

    float freqHz = 0.0f;
    float phaseDegTotal = 0.0f;
    int phaseShiftUs = 0;
    int pulseUs = 0;
    float dutyPercent = 50.0f;
    float pwmHz = 100.0f;
    int pwmCh1Us = 100;
    int pwmCh2Us = 100;

    bool signalEnabled = false;
    bool pwmEnabled = false;

    MenuLevel menu = MenuLevel::Top;
    FocusField focus = FocusField::GroupSignal;
    bool editing = false;
};

// Fields visible for a menu level (for focus wrap + display). Mode-aware for Signal.
const FocusField *menuFields(const ParamSnapshot &state, int &count);

class ParamModel {
public:
    void begin();

    void toggleEdit();
    void moveFocus(int steps);
    void applyEncoderDelta(int steps);

    const ParamSnapshot &snapshot() const;

private:
    // Duty: duty digits → pulse µs. Pulse: pulse µs → duty digits.
    // FreqKeepDuty: keep dutyPercent, refresh pulse µs from new period (Analog PWM).
    enum class PulseDutyAnchor : uint8_t { Duty = 0, Pulse, FreqKeepDuty };

    void recompute();
    void ensureFocusVisibleInMenu();
    void syncPulseFromDutyPercent(float periodUs, int maxPulseUs);

    ParamSnapshot state_{};
    PulseDutyAnchor pulseDutyAnchor_ = PulseDutyAnchor::Duty;
};
