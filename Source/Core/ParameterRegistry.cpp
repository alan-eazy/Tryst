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
    ParameterDescriptor { destination::samplerStart, "engine.sampler.start", "Sample Start", 0.00f, true },
    ParameterDescriptor { destination::samplerEnd, "engine.sampler.end", "Sample End", 1.00f, true },
    ParameterDescriptor { destination::samplerLoopEnabled, "engine.sampler.loop_enabled", "Sample Loop Enabled", 0.00f, false },
    ParameterDescriptor { destination::samplerLoopStart, "engine.sampler.loop_start", "Sample Loop Start", 0.00f, true },
    ParameterDescriptor { destination::samplerLoopEnd, "engine.sampler.loop_end", "Sample Loop End", 1.00f, true },
    ParameterDescriptor { destination::samplerReverse, "engine.sampler.reverse", "Sample Reverse", 0.00f, false },
    ParameterDescriptor { destination::granularPosition, "engine.granular.position", "Grain Position", 0.50f, true },
    ParameterDescriptor { destination::granularSize, "engine.granular.size", "Grain Size", 0.30f, true },
    ParameterDescriptor { destination::granularDensity, "engine.granular.density", "Grain Density", 0.30f, true },
    ParameterDescriptor { destination::granularSpray, "engine.granular.spray", "Grain Spray", 0.00f, true },
    ParameterDescriptor { destination::granularPitch, "engine.granular.pitch", "Grain Pitch", 0.50f, true },
    ParameterDescriptor { destination::noiseColor, "engine.noise.color", "Noise Color", 0.00f, true },
    ParameterDescriptor { destination::noiseStereo, "engine.noise.stereo", "Noise Stereo Width", 1.00f, true },
    ParameterDescriptor { destination::noiseTexture, "engine.noise.texture", "Noise Texture", 0.00f, true },
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
    ParameterDescriptor { destination::fxMix, "fx.mix", "FX Mix", 1.00f, false },
    ParameterDescriptor { destination::fxDriveAmount, "fx.drive.amount", "Drive Amount", 0.25f, false },
    ParameterDescriptor { destination::fxDelayTime, "fx.delay.time", "Delay Time", 0.20f, false },
    ParameterDescriptor { destination::fxDelayFeedback, "fx.delay.feedback", "Delay Feedback", 0.25f, false },
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
