#include "FastModulationRuntime.h"
#include <algorithm>

namespace tryst::mod
{
void FastModulationRuntime::prepare(double sampleRate) noexcept
{
    for (auto& lfo : lfos)
        lfo.prepare(sampleRate);
    for (auto& mseg : msegs)
        mseg.prepare(sampleRate);
    for (auto& step : steps)
        step.prepare(sampleRate);
    refreshSources();
}

void FastModulationRuntime::resetPhases() noexcept
{
    for (auto& lfo : lfos)
        lfo.reset();
    for (auto& mseg : msegs)
        mseg.reset();
    for (auto& step : steps)
        step.reset();
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

bool FastModulationRuntime::configureMseg(std::size_t index,
                                          std::span<const MsegPoint> points,
                                          float rateHz,
                                          bool loop) noexcept
{
    if (index >= msegs.size())
        return false;
    return msegs[index].configure(points, rateHz, loop);
}

bool FastModulationRuntime::configureStep(std::size_t index,
                                          std::span<const float> values,
                                          float rateHz,
                                          bool loop) noexcept
{
    if (index >= steps.size())
        return false;
    return steps[index].configure(values, rateHz, loop);
}

bool FastModulationRuntime::setExpressionSource(SourceId source, float value) noexcept
{
    switch (source.type)
    {
        case SourceType::velocity:
        case SourceType::keyTrack:
        case SourceType::pressure:
        case SourceType::timbre:
        case SourceType::pitchBend:
            if (source.index != 0)
                return false;
            return sources.set(source, value);
        default:
            return false;
    }
}

bool FastModulationRuntime::addRoute(Route route) noexcept
{
    switch (route.source.type)
    {
        case SourceType::lfo:
            if (route.source.index >= lfos.size())
                return false;
            break;
        case SourceType::mseg:
            if (route.source.index >= msegs.size())
                return false;
            break;
        case SourceType::step:
            if (route.source.index >= steps.size())
                return false;
            break;
        case SourceType::velocity:
        case SourceType::keyTrack:
        case SourceType::pressure:
        case SourceType::timbre:
        case SourceType::pitchBend:
            if (route.source.index != 0)
                return false;
            break;
        default:
            return false;
    }

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
    for (auto& mseg : msegs)
        mseg.advanceSamples(samples);
    for (auto& step : steps)
        step.advanceSamples(samples);
}

void FastModulationRuntime::refreshSources() noexcept
{
    for (std::size_t i = 0; i < lfos.size(); ++i)
        (void) sources.set({ SourceType::lfo, static_cast<std::uint16_t>(i) }, lfos[i].currentValue());
    for (std::size_t i = 0; i < msegs.size(); ++i)
        (void) sources.set({ SourceType::mseg, static_cast<std::uint16_t>(i) }, msegs[i].currentValue());
    for (std::size_t i = 0; i < steps.size(); ++i)
        (void) sources.set({ SourceType::step, static_cast<std::uint16_t>(i) }, steps[i].currentValue());
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
