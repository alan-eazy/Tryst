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
