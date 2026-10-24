#include "SampleData.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace tryst::dsp
{
SampleData::SampleData(std::vector<float> leftSamples,
                       std::vector<float> rightSamples,
                       double sourceRate,
                       float rootNote) noexcept
    : left(std::move(leftSamples)),
      right(std::move(rightSamples)),
      sampleRate(sourceRate > 1.0 ? sourceRate : 48000.0),
      rootMidiNote(rootNote)
{
}

std::shared_ptr<const SampleData> SampleData::createBasic()
{
    static const auto basic = [] {
        constexpr double rate = 48000.0;
        constexpr std::size_t frameCount = 12000;
        std::vector<float> mono(frameCount, 0.0f);

        for (std::size_t i = 0; i < frameCount; ++i)
        {
            const auto time = static_cast<float>(i / rate);
            const auto phase = 2.0f * std::numbers::pi_v<float> * 261.625565f * time;
            const auto envelope = std::exp(-8.0f * time);
            mono[i] = (0.75f * std::sin(phase) + 0.25f * std::sin(phase * 2.01f)) * envelope;
        }

        return SampleData::fromMono(std::move(mono), rate, 60.0f);
    }();

    return basic;
}

std::shared_ptr<const SampleData> SampleData::fromMono(std::vector<float> mono,
                                                        double sourceRate,
                                                        float rootNote)
{
    if (mono.empty())
        return {};

    for (auto& sample : mono)
        sample = std::clamp(sample, -1.0f, 1.0f);

    auto right = mono;
    return std::shared_ptr<const SampleData>(new SampleData(std::move(mono), std::move(right), sourceRate, rootNote));
}

std::shared_ptr<const SampleData> SampleData::fromStereo(std::vector<float> leftSamples,
                                                          std::vector<float> rightSamples,
                                                          double sourceRate,
                                                          float rootNote)
{
    if (leftSamples.empty() || leftSamples.size() != rightSamples.size())
        return {};

    for (auto& sample : leftSamples)
        sample = std::clamp(sample, -1.0f, 1.0f);
    for (auto& sample : rightSamples)
        sample = std::clamp(sample, -1.0f, 1.0f);

    return std::shared_ptr<const SampleData>(new SampleData(std::move(leftSamples), std::move(rightSamples), sourceRate, rootNote));
}

float SampleData::sampleLeft(std::size_t frame) const noexcept
{
    return frame < left.size() ? left[frame] : 0.0f;
}

float SampleData::sampleRight(std::size_t frame) const noexcept
{
    return frame < right.size() ? right[frame] : 0.0f;
}
}
