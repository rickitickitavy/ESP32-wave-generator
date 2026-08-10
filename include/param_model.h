#pragma once

#include <Arduino.h>

enum class Waveform : uint8_t {
    Sine = 0,
    Rectangular,
    Triangle,
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
    FreqKHz,
    FreqHundredHz,
    FreqTensHz,
    FreqHz,
    PhaseTens,
    PhaseDeg,
    PhaseFine,
    Waveform,
    Amplitude,
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
    Waveform waveform = Waveform::Sine;
    float ampVolts = 3.3f;

    int pwmFreqX10 = 10;
    int pwmCh1X20 = 5;
    int pwmCh1X1 = 0;
    int pwmCh2X20 = 5;
    int pwmCh2X1 = 0;

    float freqHz = 0.0f;
    float phaseDegTotal = 0.0f;
    float pwmHz = 100.0f;
    int pwmCh1Us = 100;
    int pwmCh2Us = 100;

    bool signalEnabled = false;
    bool pwmEnabled = false;

    MenuLevel menu = MenuLevel::Top;
    FocusField focus = FocusField::GroupSignal;
    bool editing = false;
};

// Fields visible for a menu level (for focus wrap + display).
const FocusField *menuFields(MenuLevel level, int &count);

class ParamModel {
public:
    void begin();

    void toggleEdit();
    void moveFocus(int steps);
    void applyEncoderDelta(int steps);

    const ParamSnapshot &snapshot() const;

private:
    void recompute();

    ParamSnapshot state_{};
};
