#include "WavetableData.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace tryst::dsp
{
WavetableData::WavetableData(std::vector<float> samples, std::size_t frames, std::size_t size) noexcept
    : data(std::move(samples)), frameCount(frames), tableSize(size)
{
}

std::shared_ptr<const WavetableData> WavetableData::createBasic(std::size_t size)
{
    size = std::max<std::size_t>(size, 32);
    constexpr std::size_t frameCount = 4;
    std::vector<float> samples(frameCount * size, 0.0f);

    for (std::size_t i = 0; i < size; ++i)
    {
        const auto phase = static_cast<float>(i) / static_cast<float>(size);
        const auto sine = std::sin(2.0f * std::numbers::pi_v<float> * phase);
        const auto triangle = 1.0f - 4.0f * std::abs(phase - 0.5f);
        const auto saw = (2.0f * phase) - 1.0f;
        const auto square = phase < 0.5f ? 1.0f : -1.0f;

        samples[i] = sine;
        samples[size + i] = triangle;
        samples[(2 * size) + i] = saw;
        samples[(3 * size) + i] = square;
    }

    return std::shared_ptr<const WavetableData>(new WavetableData(std::move(samples), frameCount, size));
}

std::shared_ptr<const WavetableData> WavetableData::fromFrames(std::vector<float> samples,
                                                               std::size_t frames,
                                                               std::size_t size)
{
    if (frames == 0 || size < 2 || samples.size() != frames * size)
        return {};

    for (auto& sampleValue : samples)
        sampleValue = std::clamp(sampleValue, -1.0f, 1.0f);

    return std::shared_ptr<const WavetableData>(new WavetableData(std::move(samples), frames, size));
}

float WavetableData::sample(std::size_t frame, std::size_t index) const noexcept
{
    if (frameCount == 0 || tableSize == 0)
        return 0.0f;

    frame = std::min(frame, frameCount - 1);
    index %= tableSize;
    return data[(frame * tableSize) + index];
}
}
