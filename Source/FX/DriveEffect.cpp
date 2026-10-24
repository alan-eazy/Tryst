#include "DriveEffect.h"
#include <algorithm>
#include <cmath>

namespace tryst::fx
{
void DriveEffect::process(std::span<float> left, std::span<float> right) noexcept
{
    const auto samples = std::min(left.size(), right.size());
    const auto drive = 1.0f + amount * amount * 31.0f;
    const auto norm = std::tanh(drive);
    for (std::size_t i = 0; i < samples; ++i)
    {
        const auto dryL = left[i];
        const auto dryR = right[i];
        const auto wetL = std::tanh(dryL * drive) / norm;
        const auto wetR = std::tanh(dryR * drive) / norm;
        left[i] = dryL + (wetL - dryL) * mix;
        right[i] = dryR + (wetR - dryR) * mix;
    }
}

bool DriveEffect::setParameter(tryst::params::DestinationId destination, float normalized) noexcept
{
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    if (destination == tryst::params::destination::fxMix)
    {
        mix = normalized;
        return true;
    }
    if (destination == tryst::params::destination::fxDriveAmount)
    {
        amount = normalized;
        return true;
    }
    return false;
}
}
