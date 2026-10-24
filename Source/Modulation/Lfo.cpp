#include "Lfo.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace tryst::mod
{
void Lfo::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate > 1.0 ? newSampleRate : 48000.0;
    reset();
}

void Lfo::reset(float newPhase) noexcept
{
    phase = newPhase - std::floor(newPhase);
}

void Lfo::setRateHz(float hz) noexcept
{
    rateHz = std::clamp(hz, 0.0f, 100.0f);
}

float Lfo::nextSample() noexcept
{
    const auto output = valueAt(phase);
    phase += static_cast<float>(rateHz / sampleRate);
    phase -= std::floor(phase);
    return output;
}

float Lfo::valueAt(float p) const noexcept
{
    switch (waveform)
    {
        case LfoWaveform::sine:
            return std::sin(2.0f * std::numbers::pi_v<float> * p);
        case LfoWaveform::triangle:
            return 1.0f - 4.0f * std::abs(p - 0.5f);
        case LfoWaveform::saw:
            return (2.0f * p) - 1.0f;
        case LfoWaveform::square:
            return p < 0.5f ? 1.0f : -1.0f;
    }

    return 0.0f;
}
