#pragma once

#include "Engine.h"
#include <array>
#include <cstdint>

namespace tryst::dsp
{
class NoiseEngine final : public Engine
{
public:
    [[nodiscard]] EngineKind kind() const noexcept override { return EngineKind::noise; }
    [[nodiscard]] std::string_view name() const noexcept override { return "Noise / Texture"; }

    void prepare(double newSampleRate, std::uint32_t maxBlockSize) override;
    void reset() noexcept override;
    void noteOn(const NoteEvent& note) noexcept override;
    void noteOff(std::int32_t note, float releaseVelocity) noexcept override;
    void render(ProcessContext& context) noexcept override;
    [[nodiscard]] bool setParameter(tryst::params::DestinationId destination, float normalized) noexcept override;

private:
    struct ChannelState
    {
        std::uint32_t random = 0x9e3779b9u;
        std::array<float, 7> pink {};
        float brown = 0.0f;
        float held = 0.0f;
    };

    [[nodiscard]] static float nextWhite(ChannelState& state) noexcept;
    [[nodiscard]] static float nextPink(ChannelState& state, float white) noexcept;
    [[nodiscard]] static float nextBrown(ChannelState& state, float white) noexcept;
    [[nodiscard]] float colored(ChannelState& state) const noexcept;
    [[nodiscard]] std::uint32_t holdLengthSamples() const noexcept;

    ChannelState leftState;
    ChannelState rightState;
    double sampleRate = 48000.0;
    float velocity = 0.0f;
    float level = 0.65f;
    float color = 0.0f;
    float stereoWidth = 1.0f;
    float texture = 0.0f;
    float tuneSemitones = 0.0f;
    std::uint32_t holdCounter = 0;
    bool active = false;
};
}
