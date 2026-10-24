#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace tryst::params
{
using DestinationId = std::uint32_t;

[[nodiscard]] constexpr DestinationId destinationId(std::string_view text) noexcept
{
    DestinationId hash = 2166136261u;
    for (const auto c : text)
    {
        hash ^= static_cast<std::uint8_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

namespace destination
{
inline constexpr DestinationId generatorLevel = destinationId("generator.level");
inline constexpr DestinationId generatorTune = destinationId("generator.tune");
inline constexpr DestinationId analogShape = destinationId("engine.analog.shape");
inline constexpr DestinationId wavetablePosition = destinationId("engine.wavetable.position");
inline constexpr DestinationId samplerStart = destinationId("engine.sampler.start");
inline constexpr DestinationId samplerEnd = destinationId("engine.sampler.end");
inline constexpr DestinationId samplerLoopEnabled = destinationId("engine.sampler.loop_enabled");
inline constexpr DestinationId samplerLoopStart = destinationId("engine.sampler.loop_start");
inline constexpr DestinationId samplerLoopEnd = destinationId("engine.sampler.loop_end");
inline constexpr DestinationId samplerReverse = destinationId("engine.sampler.reverse");
inline constexpr DestinationId granularPosition = destinationId("engine.granular.position");
inline constexpr DestinationId granularSize = destinationId("engine.granular.size");
inline constexpr DestinationId granularDensity = destinationId("engine.granular.density");
inline constexpr DestinationId granularSpray = destinationId("engine.granular.spray");
inline constexpr DestinationId granularPitch = destinationId("engine.granular.pitch");
inline constexpr DestinationId noiseColor = destinationId("engine.noise.color");
inline constexpr DestinationId noiseStereo = destinationId("engine.noise.stereo");
inline constexpr DestinationId noiseTexture = destinationId("engine.noise.texture");
inline constexpr DestinationId filter1Enabled = destinationId("filter.1.enabled");
inline constexpr DestinationId filter1Cutoff = destinationId("filter.1.cutoff");
inline constexpr DestinationId filter1Resonance = destinationId("filter.1.resonance");
inline constexpr DestinationId filter1Mode = destinationId("filter.1.mode");
inline constexpr DestinationId filter2Enabled = destinationId("filter.2.enabled");
inline constexpr DestinationId filter2Cutoff = destinationId("filter.2.cutoff");
inline constexpr DestinationId filter2Resonance = destinationId("filter.2.resonance");
inline constexpr DestinationId filter2Mode = destinationId("filter.2.mode");
inline constexpr DestinationId filterRouting = destinationId("filter.routing");
inline constexpr DestinationId filterParallelBlend = destinationId("filter.parallel_blend");
inline constexpr DestinationId fxMix = destinationId("fx.mix");
inline constexpr DestinationId fxDriveAmount = destinationId("fx.drive.amount");
inline constexpr DestinationId fxDelayTime = destinationId("fx.delay.time");
inline constexpr DestinationId fxDelayFeedback = destinationId("fx.delay.feedback");
}

struct ParameterDescriptor
{
    DestinationId destination = 0;
    std::string_view id;
    std::string_view name;
    float defaultNormalized = 0.0f;
    bool audioRateCapable = false;
};

class ParameterRegistry
{
public:
    [[nodiscard]] static std::span<const ParameterDescriptor> all() noexcept;
    [[nodiscard]] static const ParameterDescriptor* find(DestinationId destination) noexcept;
    [[nodiscard]] static const ParameterDescriptor* find(std::string_view id) noexcept;
};
}
