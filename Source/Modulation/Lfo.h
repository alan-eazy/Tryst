#pragma once

#include <cstdint>

namespace tryst::mod
{
enum class LfoWaveform : std::uint8_t
{
    sine,
    triangle,
    saw,
    square
};

class Lfo
{
public:
    void prepare(double newSampleRate) noexcept;
    void reset(float phase = 0.0f) noexcept;
    void setRateHz(float hz) noexcept;
    void setWaveform(LfoWaveform next) noexcept { waveform = next; }

    [[nodiscard]] float nextSample() noexcept;
    [[nodiscard]] float currentPhase() const noexcept { return phase; }

private:
    [[nodiscard]] float valueAt(float p) const noexcept;

    double sampleRate = 48000.0;
    float rateHz = 1.0f;
    float phase = 0.0f;
    LfoWaveform waveform = LfoWaveform::sine;
};
}
