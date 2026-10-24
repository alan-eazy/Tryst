#include "ParameterRegistry.h"
#include <array>

namespace tryst::params
{
namespace
{
constexpr std::array registry {
    ParameterDescriptor { destination::generatorLevel, "generator.level", "Generator Level", 0.65f, true },
    ParameterDescriptor { destination::generatorTune, "generator.tune", "Generator Tune", 0.50f, true },
    ParameterDescriptor { destination::analogShape, "engine.analog.shape", "Analog Shape", 0.00f, true },
    ParameterDescriptor { destination::wavetablePosition, "engine.wavetable.position", "Wavetable Position", 0.00f, true },
};

consteval bool destinationsAreUnique()
{
    for (std::size_t i = 0; i < registry.size(); ++i)
        for (std::size_t j = i + 1; j < registry.size(); ++j)
            if (registry[i].destination == registry[j].destination)
                return false;
    return true;
}

static_assert(destinationsAreUnique(), "Parameter destination hash collision detected");
}

std::span<const ParameterDescriptor> ParameterRegistry::all() noexcept
{
    return registry;
}

const ParameterDescriptor* ParameterRegistry::find(DestinationId destination) noexcept
{
    for (const auto& descriptor : registry)
        if (descriptor.destination == destination)
            return &descriptor;
    return nullptr;
}

const ParameterDescriptor* ParameterRegistry::find(std::string_view id) noexcept
{
    for (const auto& descriptor : registry)
        if (descriptor.id == id)
            return &descriptor;
    return nullptr;
}
}
