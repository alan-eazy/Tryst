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
    ParameterDescriptor { destination::filter1Enabled, "filter.1.enabled", "Filter 1 Enabled", 0.00f, false },
    ParameterDescriptor { destination::filter1Cutoff, "filter.1.cutoff", "Filter 1 Cutoff", 1.00f, true },
    ParameterDescriptor { destination::filter1Resonance, "filter.1.resonance", "Filter 1 Resonance", 0.00f, true },
    ParameterDescriptor { destination::filter1Mode, "filter.1.mode", "Filter 1 Mode", 0.00f, false },
    ParameterDescriptor { destination::filter2Enabled, "filter.2.enabled", "Filter 2 Enabled", 0.00f, false },
    ParameterDescriptor { destination::filter2Cutoff, "filter.2.cutoff", "Filter 2 Cutoff", 1.00f, true },
    ParameterDescriptor { destination::filter2Resonance, "filter.2.resonance", "Filter 2 Resonance", 0.00f, true },
    ParameterDescriptor { destination::filter2Mode, "filter.2.mode", "Filter 2 Mode", 0.00f, false },
    ParameterDescriptor { destination::filterRouting, "filter.routing", "Filter Routing", 0.00f, false },
    ParameterDescriptor { destination::filterParallelBlend, "filter.parallel_blend", "Parallel Filter Blend", 0.50f, true },
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
