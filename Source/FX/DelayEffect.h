#pragma once
#include "Effect.h"
#include <vector>

namespace tryst::fx
{
class DelayEffect final : public Effect
{
public:
    [[nodiscard]] EffectKind kind() const noexcept override { return EffectKind::delay; }
    [[nodiscard]] std::string_view name() const noexcept override { return "Stereo Delay"; }
    void prepare(double sampleRate, std::uint32_t maxBlockSize) override;
    void reset() noexcept override;
    void process(std::span<float> left, std::span<float> right) noexcept override;
    [[nodiscard]] bool setParameter(tryst::params::DestinationId destination, float normalized) noexcept override;

private:
    [[nodiscard]] float readInterpolated(const std::vector<float>& buffer, double delaySamples) const noexcept;

    std::vector<float> leftBuffer;
    std::vector<float> rightBuffer;
    double sampleRate = 48000.0;
    std::size_t writeIndex = 0;
    float mix = 0.25f;
    float time = 0.20f;
    float feedback = 0.25f;
};
}
