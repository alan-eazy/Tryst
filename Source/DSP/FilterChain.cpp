#include "FilterChain.h"
#include <algorithm>

namespace tryst::dsp
{
void FilterChain::prepare(double sampleRate) noexcept
{
    filter1.prepare(sampleRate);
    filter2.prepare(sampleRate);
    reset();
}

void FilterChain::reset() noexcept
{
    filter1.reset();
    filter2.reset();
}

bool FilterChain::setParameter(tryst::params::DestinationId destination, float normalized) noexcept
{
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    if (destination == tryst::params::destination::filter1Enabled)
    {
        filter1.setEnabled(normalized >= 0.5f);
        return true;
    }
    if (destination == tryst::params::destination::filter1Cutoff)
    {
        filter1.setCutoffNormalized(normalized);
        return true;
    }
    if (destination == tryst::params::destination::filter1Resonance)
    {
        filter1.setResonanceNormalized(normalized);
        return true;
    }
    if (destination == tryst::params::destination::filter1Mode)
    {
        filter1.setModeNormalized(normalized);
        return true;
    }
    if (destination == tryst::params::destination::filter2Enabled)
    {
        filter2.setEnabled(normalized >= 0.5f);
        return true;
    }
    if (destination == tryst::params::destination::filter2Cutoff)
    {
        filter2.setCutoffNormalized(normalized);
        return true;
    }
    if (destination == tryst::params::destination::filter2Resonance)
    {
        filter2.setResonanceNormalized(normalized);
        return true;
    }
    if (destination == tryst::params::destination::filter2Mode)
    {
        filter2.setModeNormalized(normalized);
        return true;
    }
    if (destination == tryst::params::destination::filterRouting)
    {
        routing = normalized < 0.5f ? FilterRouting::serial : FilterRouting::parallel;
        return true;
    }
    if (destination == tryst::params::destination::filterParallelBlend)
    {
        parallelBlend = normalized;
        return true;
    }

    return false;
}

void FilterChain::process(std::span<float> left, std::span<float> right) noexcept
{
    const auto samples = std::min(left.size(), right.size());

    for (std::size_t i = 0; i < samples; ++i)
    {
        if (routing == FilterRouting::serial)
        {
            filter1.process(left[i], right[i]);
            filter2.process(left[i], right[i]);
            continue;
        }

        const auto inputLeft = left[i];
        const auto inputRight = right[i];

        auto filter1Left = inputLeft;
        auto filter1Right = inputRight;
        auto filter2Left = inputLeft;
        auto filter2Right = inputRight;

        filter1.process(filter1Left, filter1Right);
        filter2.process(filter2Left, filter2Right);

        left[i] = filter1Left + (filter2Left - filter1Left) * parallelBlend;
        right[i] = filter1Right + (filter2Right - filter1Right) * parallelBlend;
    }
}
}
