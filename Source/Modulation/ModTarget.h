#pragma once

#include "../Core/ParameterRegistry.h"
#include <cstdint>

namespace tryst::mod
{
enum class TargetDomain : std::uint8_t
{
    generator,
    filter,
    global
};

struct TargetId
{
    TargetDomain domain = TargetDomain::generator;
    std::uint8_t slot = 0;
    tryst::params::DestinationId parameter = 0;

    friend constexpr bool operator==(TargetId, TargetId) = default;
};
}
