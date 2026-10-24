#pragma once

#include <cstdint>

namespace tryst::mod
{
enum class SourceType : std::uint8_t
{
    lfo,
    envelope,
    mseg,
    step,
    random,
    velocity,
    keyTrack,
    pressure,
    timbre,
    pitchBend,
    macro,
    morphX,
    morphY,
    envelopeFollower
};

struct SourceId
{
    SourceType type = SourceType::lfo;
    std::uint16_t index = 0;

    friend constexpr bool operator==(SourceId, SourceId) = default;
};
}
