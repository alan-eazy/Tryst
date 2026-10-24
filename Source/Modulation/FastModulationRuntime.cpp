#include "FastModulationRuntime.h"
#include <algorithm>

namespace tryst::mod
{
void FastModulationRuntime::prepare(double sampleRate) noexcept
{
    for (auto& lfo : lfos)
        lfo.prepare(sampleRate);
    refreshSources();
}

void FastModulationRuntime::resetPhases() noexcept
{
    for (auto& lfo : lfos)
        lfo.reset();
    refreshSources();
}

bool FastModulationRuntime::configureLfo(std::size_t index, float rateHz, LfoWaveform waveform) noexcept
{
    if (index >= lfos.size())
        return false;

    lfos[index].setRateHz(rateHz);
    lfos[index].setWaveform(waveform);
    return true;
}

bool FastModulationRuntime::addRoute(Route route) noexcept
{
    if (route.source.type != SourceType::lfo || route.source.index >= lfos.size())
        return false;

    if (!trackTarget(route.target))
        return false;

    return matrix.addRoute(route);
}

void FastModulationRuntime::clearRoutes() noexcept
{
    matrix.clear();
    baseValues.clear();
    targetCount = 0;
}

bool FastModulationRuntime::setBase(TargetId target, float normalized) noexcept
{
    if (!isTracked(target))
        return true;
    return baseValues.set(target, normalized);
}

void FastModulationRuntime::advance(std::uint32_t samples) noexcept
{
    for (auto& lfo : lfos)
        lfo.advanceSamples(samples);
}

void FastModulationRuntime::refreshSources() noexcept
{
    for (std::size_t i = 0; i < lfos.size(); ++i)
        (void) sources.set({ SourceType::lfo, static_cast<std::uint16_t>(i) }, lfos[i].currentValue());
}

bool FastModulationRuntime::trackTarget(TargetId target) noexcept
{
    for (std::size_t i = 0; i < targetCount; ++i)
        if (targets[i] == target)
            return true;

    if (targetCount >= targets.size())
        return false;

    targets[targetCount++] = target;
    return true;
}

bool FastModulationRuntime::isTracked(TargetId target) const noexcept
{
    for (std::size_t i = 0; i < targetCount; ++i)
        if (targets[i] == target)
            return true;
    return false;
}
}
