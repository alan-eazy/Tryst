#pragma once

#include "EngineAsset.h"
#include <cstddef>
#include <memory>
#include <vector>

namespace tryst::dsp
{
class SampleData : public EngineAsset
{
public:
    [[nodiscard]] static std::shared_ptr<const SampleData> createBasic();
    [[nodiscard]] static std::shared_ptr<const SampleData> fromMono(std::vector<float> mono,
                                                                    double sampleRate,
                                                                    float rootMidiNote = 60.0f);
    [[nodiscard]] static std::shared_ptr<const SampleData> fromStereo(std::vector<float> left,
                                                                      std::vector<float> right,
                                                                      double sampleRate,
                                                                      float rootMidiNote = 60.0f);

    [[nodiscard]] std::size_t frames() const noexcept { return left.size(); }
    [[nodiscard]] double sourceSampleRate() const noexcept { return sampleRate; }
    [[nodiscard]] float rootNote() const noexcept { return rootMidiNote; }
    [[nodiscard]] float sampleLeft(std::size_t frame) const noexcept;
    [[nodiscard]] float sampleRight(std::size_t frame) const noexcept;

private:
    SampleData(std::vector<float> leftSamples,
               std::vector<float> rightSamples,
               double sourceRate,
               float rootNote) noexcept;

    std::vector<float> left;
    std::vector<float> right;
    double sampleRate = 48000.0;
    float rootMidiNote = 60.0f;
};
}
