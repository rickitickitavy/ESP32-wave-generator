#pragma once

#include <Arduino.h>

enum class Waveform : uint8_t {
    Sine = 0,
    Rectangular,
    Triangle,
    Count,
};

enum class FocusField : uint8_t {
    FreqKHz = 0,
    FreqHundredHz,
    FreqHz,
    PhaseTens,
    PhaseDeg,
    PhaseFine,
    Waveform,
    Amplitude,
    Count,
};

struct ParamSnapshot {
    int freqKHz = 0;
    int freqHundredHz = 0;
    float freqHzPart = 1.0f;
    int phaseTens = 0;
    int phaseDeg = 0;
    float phaseFine = 0.0f;
    Waveform waveform = Waveform::Sine;
    float ampVolts = 3.3f;

    float freqHz = 1.0f;
    float phaseDegTotal = 0.0f;
    FocusField focus = FocusField::FreqHz;
};

class ParamModel {
public:
    void begin();

    void nextFocus();
    void applyEncoderDelta(int steps);

    const ParamSnapshot &snapshot() const;

private:
    void recompute();

    ParamSnapshot state_{};
};
