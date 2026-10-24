#include "Mseg.h"
#include <algorithm>
#include <cmath>

namespace tryst::mod
{
void Mseg::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate > 1.0 ? newSampleRate : 48000.0;
    reset();
}

void Mseg::reset(float newPhase) noexcept
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

bool Mseg::configure(std::span<const MsegPoint> newPoints,
                     float newRateHz,
                     bool shouldLoop) noexcept
{
    if (newPoints.size() < 2 || newPoints.size() > points.size()
        || newRateHz < 0.0f || newRateHz > 100.0f)
        return false;

    constexpr float epsilon = 0.00001f;
    if (std::abs(newPoints.front().phase) > epsilon
        || std::abs(newPoints.back().phase - 1.0f) > epsilon)
        return false;

    float previousPhase = -1.0f;
    for (const auto& point : newPoints)
    {
        if (point.phase < 0.0f || point.phase > 1.0f
            || point.value < -1.0f || point.value > 1.0f
            || point.phase <= previousPhase)
            return false;
        previousPhase = point.phase;
    }

    std::copy(newPoints.begin(), newPoints.end(), points.begin());
    count = newPoints.size();
    rate = newRateHz;
    loop = shouldLoop;
    reset();
    return true;
}

float Mseg::currentValue() const noexcept
{
    return valueAt(phase);
}

void Mseg::advanceSamples(std::uint32_t samples) noexcept
{
    if (count < 2 || samples == 0 || rate <= 0.0f)
        return;

    phase += static_cast<float>((rate / sampleRate) * static_cast<double>(samples));
    if (loop)
        phase -= std::floor(phase);
    else
        phase = std::min(phase, 1.0f);
}

float Mseg::valueAt(float normalizedPhase) const noexcept
{
    if (count < 2)
        return 0.0f;

    const auto p = std::clamp(normalizedPhase, 0.0f, 1.0f);
    if (p <= points[0].phase)
        return points[0].value;
    if (p >= points[count - 1].phase)
        return points[count - 1].value;

    for (std::size_t i = 1; i < count; ++i)
    {
        if (p > points[i].phase)
            continue;

        const auto& left = points[i - 1];
        const auto& right = points[i];
        const auto width = right.phase - left.phase;
        const auto t = width > 0.0f ? (p - left.phase) / width : 0.0f;
        return left.value + ((right.value - left.value) * t);
    }

    return points[count - 1].value;
}
}
