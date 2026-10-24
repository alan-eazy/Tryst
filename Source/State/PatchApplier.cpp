#include "PatchApplier.h"
#include "Core/ParameterRegistry.h"
#include <algorithm>

namespace tryst::state
{
bool PatchApplier::apply(const PatchState& patch, tryst::dsp::VoiceManager& manager)
{
    if (patch.schemaVersion != PatchState::currentSchemaVersion)
        return false;

    manager.reset();
    manager.clearModulationRoutes();
    manager.clearFastModulationRoutes();

    bool success = true;
    for (std::size_t slot = 0; slot < patch.generators.size(); ++slot)
    {
        if (patch.generators[slot].enabled)
            success = manager.setEngineKind(slot, patch.generators[slot].kind) && success;
        else
            manager.clearEngine(slot);
    }

    manager.setAmpEnvelope(patch.ampEnvelope);

    for (const auto& parameter : patch.parameters)
    {
        const auto* descriptor = tryst::params::ParameterRegistry::find(parameter.parameterId);
        if (descriptor == nullptr)
        {
            success = false;
            continue;
        }

        if (parameter.domain == tryst::mod::TargetDomain::generator)
            success = manager.setEngineParameter(parameter.slot, descriptor->destination, parameter.normalized) && success;
        else if (parameter.domain == tryst::mod::TargetDomain::filter)
            success = manager.setFilterParameter(descriptor->destination, parameter.normalized) && success;
        else
            success = false;
    }

    for (std::size_t i = 0; i < patch.lfos.size(); ++i)
        success = manager.configureVoiceLfo(i, patch.lfos[i].rateHz, patch.lfos[i].waveform) && success;

    for (std::size_t i = 0; i < patch.msegs.size(); ++i)
    {
        const auto& mseg = patch.msegs[i];
        if (!mseg.points.empty())
            success = manager.configureVoiceMseg(i, mseg.points, mseg.rateHz, mseg.loop) && success;
    }

    for (std::size_t i = 0; i < patch.steps.size(); ++i)
    {
        const auto& step = patch.steps[i];
        if (!step.values.empty())
            success = manager.configureVoiceStep(i, step.values, step.rateHz, step.loop) && success;
    }

    for (const auto& savedRoute : patch.routes)
    {
        const auto* descriptor = tryst::params::ParameterRegistry::find(savedRoute.parameterId);
        if (descriptor == nullptr)
        {
            success = false;
            continue;
        }

        tryst::mod::Route route {
            savedRoute.source,
            { savedRoute.domain, savedRoute.slot, descriptor->destination },
            savedRoute.depth,
            savedRoute.curve,
            savedRoute.enabled
        };

        if (savedRoute.fast)
            success = manager.addFastModulationRoute(route) && success;
        else
            success = manager.addModulationRoute(route) && success;
    }

    for (std::size_t i = 0; i < patch.macros.size(); ++i)
        success = manager.setMacro(i, std::clamp(patch.macros[i], 0.0f, 1.0f)) && success;
    manager.setMorphX(std::clamp(patch.morphX, 0.0f, 1.0f));
    manager.setMorphY(std::clamp(patch.morphY, 0.0f, 1.0f));

    return success;
}
}
