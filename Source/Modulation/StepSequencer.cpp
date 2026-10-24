#include "StepSequencer.h"
#include <algorithm>
#include <cmath>

namespace tryst::mod
{
void StepSequencer::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate > 1.0 ? newSampleRate : 48000.0;
    reset();
}

void StepSequencer::reset(float newPhase) noexcept
{
    if (loop)
    {
        phase = newPhase - std::floor(newPhase);
        if (phase < 0.0f)
            phase += 1.0f;
    }
    else
    {
        phase = std::clamp(newPhase, 0.0f, 1.0f);
    }
}

bool StepSequencer::configure(std::span<const float> newValues,
                              float newRateHz,
                              bool shouldLoop) noexcept
{
    if (newValues.empty() || newValues.size() > values.size()
        || newRateHz < 0.0f || newRateHz > 100.0f)
        return false;

    for (const auto value : newValues)
        if (value < -1.0f || value > 1.0f)
            return false;

    std::copy(newValues.begin(), newValues.end(), values.begin());
    count = newValues.size();
    rate = newRateHz;
    loop = shouldLoop;
    reset();
    return true;
}

float StepSequencer::currentValue() const noexcept
{
    if (count == 0)
        return 0.0f;

    if (!loop && phase >= 1.0f)
        return values[count - 1];

    const auto index = std::min<std::size_t>(
        static_cast<std::size_t>(phase * static_cast<float>(count)), count - 1);
    return values[index];
}

void StepSequencer::advanceSamples(std::uint32_t samples) noexcept
{
    if (count == 0 || samples == 0 || rate <= 0.0f)
        return;

    phase += static_cast<float>((rate / sampleRate) * static_cast<double>(samples));
    if (loop)
        phase -= std::floor(phase);
    else
        phase = std::min(phase, 1.0f);
}
}
