#include "DelayEffect.h"
#include <algorithm>
#include <cmath>

namespace tryst::fx
{
void DelayEffect::prepare(double newSampleRate, std::uint32_t)
{
    sampleRate = newSampleRate > 1.0 ? newSampleRate : 48000.0;
    const auto capacity = static_cast<std::size_t>(std::ceil(sampleRate * 2.0)) + 4;
    leftBuffer.assign(capacity, 0.0f);
    rightBuffer.assign(capacity, 0.0f);
    writeIndex = 0;
}

void DelayEffect::reset() noexcept
{
    std::fill(leftBuffer.begin(), leftBuffer.end(), 0.0f);
    std::fill(rightBuffer.begin(), rightBuffer.end(), 0.0f);
    writeIndex = 0;
}

float DelayEffect::readInterpolated(const std::vector<float>& buffer, double delaySamples) const noexcept
{
    if (buffer.empty())
        return 0.0f;
    const auto size = static_cast<double>(buffer.size());
    auto read = static_cast<double>(writeIndex) - delaySamples;
    while (read < 0.0)
        read += size;
    while (read >= size)
        read -= size;
    const auto i0 = static_cast<std::size_t>(std::floor(read));
    const auto i1 = (i0 + 1) % buffer.size();
    const auto blend = static_cast<float>(read - std::floor(read));
    return buffer[i0] + (buffer[i1] - buffer[i0]) * blend;
}

void DelayEffect::process(std::span<float> left, std::span<float> right) noexcept
{
    if (leftBuffer.empty() || rightBuffer.empty())
        return;

    const auto samples = std::min(left.size(), right.size());
    // 1 ms .. 1500 ms, squared for useful short-delay resolution.
    const auto shaped = static_cast<double>(time) * static_cast<double>(time);
    const auto secondsL = 0.001 + shaped * 1.499;
    // A small fixed offset produces stereo motion without another parameter.
    const auto secondsR = std::min(1.95, secondsL * 1.071);
    const auto delayL = std::clamp(secondsL * sampleRate, 1.0, static_cast<double>(leftBuffer.size() - 2));
    const auto delayR = std::clamp(secondsR * sampleRate, 1.0, static_cast<double>(rightBuffer.size() - 2));

    for (std::size_t i = 0; i < samples; ++i)
    {
        const auto dryL = left[i];
        const auto dryR = right[i];
        const auto wetL = readInterpolated(leftBuffer, delayL);
        const auto wetR = readInterpolated(rightBuffer, delayR);

        leftBuffer[writeIndex] = dryL + wetR * feedback;
        rightBuffer[writeIndex] = dryR + wetL * feedback;
        writeIndex = (writeIndex + 1) % leftBuffer.size();

        left[i] = dryL + (wetL - dryL) * mix;
        right[i] = dryR + (wetR - dryR) * mix;
    }
}

bool DelayEffect::setParameter(tryst::params::DestinationId destination, float normalized) noexcept
{
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    if (destination == tryst::params::destination::fxMix)
    {
        mix = normalized;
        return true;
    }
    if (destination == tryst::params::destination::fxDelayTime)
    {
        time = normalized;
        return true;
    }
    if (destination == tryst::params::destination::fxDelayFeedback)
    {
        feedback = normalized * 0.95f;
        return true;
    }
    return false;
}
}
