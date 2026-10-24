#include "PluginEngine.h"
#include "State/PatchApplier.h"
#include "State/PatchCodec.h"
#include <algorithm>
#include <cmath>

namespace tryst::plugin
{
namespace
{
bool startsWith(std::string_view text, std::string_view prefix) noexcept
{
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}
}

PluginEngine::PluginEngine()
{
    hostValues.reserve(catalog.all().size());
    for (const auto& parameter : catalog.all())
        hostValues.push_back(parameter.defaultNormalized);
}

bool PluginEngine::prepare(double sampleRate, std::uint32_t maxBlockSize)
{
    if (sampleRate <= 1.0 || maxBlockSize == 0 || !catalog.valid())
        return false;

    preparedSampleRate = sampleRate;
    preparedBlockSize = maxBlockSize;
    prepared = true;
    return rebuildManager();
}

void PluginEngine::reset() noexcept
{
    if (prepared)
        manager.reset();
}

bool PluginEngine::process(std::span<float> left,
                           std::span<float> right,
                           std::span<const HostEvent> events) noexcept
{
    if (!prepared)
        return false;

    const auto samples = std::min(left.size(), right.size());
    if (samples == 0)
        return true;

    std::fill_n(left.begin(), static_cast<std::ptrdiff_t>(samples), 0.0f);
    std::fill_n(right.begin(), static_cast<std::ptrdiff_t>(samples), 0.0f);

    std::size_t cursor = 0;
    std::size_t eventIndex = 0;
    std::uint32_t previousFrame = 0;
    bool firstEvent = true;

    while (eventIndex < events.size())
    {
        const auto frame = events[eventIndex].frame;
        if (frame > samples || (!firstEvent && frame < previousFrame))
            return false;
        firstEvent = false;
        previousFrame = frame;

        if (frame > cursor)
        {
            renderRange(left, right, cursor, frame - cursor);
            cursor = frame;
        }

        while (eventIndex < events.size() && events[eventIndex].frame == frame)
        {
            dispatchEvent(events[eventIndex]);
            ++eventIndex;
        }
    }

    if (cursor < samples)
        renderRange(left, right, cursor, samples - cursor);

    return true;
}

bool PluginEngine::setHostParameter(std::uint32_t parameterId, float normalized) noexcept
{
    const auto* info = catalog.find(parameterId);
    if (info == nullptr || !std::isfinite(normalized))
        return false;

    normalized = std::clamp(normalized, 0.0f, 1.0f);
    if (info->stepped)
        normalized = normalized >= 0.5f ? 1.0f : 0.0f;

    const auto index = catalogIndex(*info);
    if (index >= hostValues.size())
        return false;
    hostValues[index] = normalized;

    if (!prepared)
        return true;

    // A parameter may belong to an engine/effect which is not currently active.
    // The value is still accepted in fixed storage so it can become effective if
    // that topology is selected later.
    (void) applyHostParameter(*info, normalized);
    return true;
}

float PluginEngine::hostParameterValue(std::uint32_t parameterId) const noexcept
{
    const auto* info = catalog.find(parameterId);
    if (info == nullptr)
        return 0.0f;
    const auto index = catalogIndex(*info);
    return index < hostValues.size() ? hostValues[index] : info->defaultNormalized;
}

bool PluginEngine::setGeneratorKind(std::size_t slot, tryst::dsp::EngineKind kind, bool enabled)
{
    if (slot >= patch.generators.size())
        return false;
    patch.generators[slot] = { enabled, kind };
    return !prepared || rebuildManager();
}

bool PluginEngine::setEffectKind(std::size_t slot, tryst::fx::EffectKind kind, bool enabled)
{
    if (slot >= patch.effects.size())
        return false;
    patch.effects[slot] = { enabled, kind };
    return !prepared || rebuildManager();
}

bool PluginEngine::setEffectOrder(std::span<const std::uint8_t> slotOrder)
{
    if (slotOrder.size() != patch.effectOrder.size())
        return false;
    std::array<bool, 4> seen {};
    for (const auto slot : slotOrder)
    {
        if (slot >= seen.size() || seen[slot])
            return false;
        seen[slot] = true;
    }
    std::copy(slotOrder.begin(), slotOrder.end(), patch.effectOrder.begin());
    return !prepared || rebuildManager();
}

bool PluginEngine::setTuningOffsetSemitones(std::int32_t note, float semitones)
{
    if (note < 0 || note >= static_cast<std::int32_t>(patch.tuningOffsetsSemitones.size())
        || !std::isfinite(semitones) || semitones < -48.0f || semitones > 48.0f)
        return false;

    patch.tuningOffsetsSemitones[static_cast<std::size_t>(note)] = semitones;
    return !prepared || manager.setTuningOffsetSemitones(note, semitones);
}

std::string PluginEngine::saveState() const
{
    return tryst::state::PatchCodec::encode(makeSerializablePatch());
}

bool PluginEngine::loadState(std::string_view text)
{
    auto decoded = tryst::state::PatchCodec::decode(text);
    if (!decoded)
        return false;

    patch = std::move(*decoded.patch);
    syncHostValuesFromPatch(patch);
    return !prepared || rebuildManager();
}

bool PluginEngine::rebuildManager()
{
    if (!prepared)
        return true;

    tryst::dsp::VoiceManager next;
    next.prepare(preparedSampleRate, preparedBlockSize);
    const auto effectivePatch = makeSerializablePatch();
    if (!tryst::state::PatchApplier::apply(effectivePatch, next))
        return false;

    manager = std::move(next);
    return true;
}

bool PluginEngine::applyHostParameter(const HostParameterInfo& info, float normalized) noexcept
{
    switch (info.kind)
    {
        case HostParameterKind::macro:
            return manager.setMacro(info.slot, normalized);
        case HostParameterKind::morphX:
            manager.setMorphX(normalized);
            return true;
        case HostParameterKind::morphY:
            manager.setMorphY(normalized);
            return true;
        case HostParameterKind::patchParameter:
            break;
    }

    if (info.domain == tryst::mod::TargetDomain::generator)
        return manager.setEngineParameter(info.slot, info.destination, normalized);
    if (info.domain == tryst::mod::TargetDomain::filter)
        return manager.setFilterParameter(info.destination, normalized);
    if (info.domain == tryst::mod::TargetDomain::effect)
        return manager.setEffectParameter(info.slot, info.destination, normalized);
    return false;
}

std::size_t PluginEngine::catalogIndex(const HostParameterInfo& info) const noexcept
{
    const auto all = catalog.all();
    if (all.empty() || &info < all.data() || &info >= all.data() + all.size())
        return all.size();
    return static_cast<std::size_t>(&info - all.data());
}

tryst::state::PatchState PluginEngine::makeSerializablePatch() const
{
    auto result = patch;
    result.schemaVersion = tryst::state::PatchState::currentSchemaVersion;
    result.parameters.clear();

    for (const auto& info : catalog.all())
    {
        const auto index = catalogIndex(info);
        const auto value = index < hostValues.size() ? hostValues[index] : info.defaultNormalized;
        if (info.kind == HostParameterKind::macro)
        {
            result.macros[info.slot] = value;
            continue;
        }
        if (info.kind == HostParameterKind::morphX)
        {
            result.morphX = value;
            continue;
        }
        if (info.kind == HostParameterKind::morphY)
        {
            result.morphY = value;
            continue;
        }

        if (!parameterAppliesToPatch(info))
            continue;

        const auto* descriptor = tryst::params::ParameterRegistry::find(info.destination);
        if (descriptor != nullptr)
            result.parameters.push_back({ info.domain, info.slot, std::string(descriptor->id), value });
    }

    return result;
}

void PluginEngine::syncHostValuesFromPatch(const tryst::state::PatchState& source) noexcept
{
    const auto all = catalog.all();
    for (std::size_t i = 0; i < all.size() && i < hostValues.size(); ++i)
        hostValues[i] = all[i].defaultNormalized;

    for (const auto& info : all)
    {
        const auto index = catalogIndex(info);
        if (index >= hostValues.size())
            continue;
        if (info.kind == HostParameterKind::macro)
            hostValues[index] = source.macros[info.slot];
        else if (info.kind == HostParameterKind::morphX)
            hostValues[index] = source.morphX;
        else if (info.kind == HostParameterKind::morphY)
            hostValues[index] = source.morphY;
    }

    for (const auto& parameter : source.parameters)
    {
        const auto* descriptor = tryst::params::ParameterRegistry::find(parameter.parameterId);
        if (descriptor == nullptr)
            continue;
        for (const auto& info : all)
        {
            if (info.kind == HostParameterKind::patchParameter
                && info.domain == parameter.domain && info.slot == parameter.slot
                && info.destination == descriptor->destination)
            {
                const auto index = catalogIndex(info);
                if (index < hostValues.size())
                    hostValues[index] = parameter.normalized;
                break;
            }
        }
    }
}

bool PluginEngine::parameterAppliesToPatch(const HostParameterInfo& info) const noexcept
{
    if (info.kind != HostParameterKind::patchParameter)
        return false;

    const auto* descriptor = tryst::params::ParameterRegistry::find(info.destination);
    if (descriptor == nullptr)
        return false;

    const auto id = descriptor->id;
    if (info.domain == tryst::mod::TargetDomain::filter)
        return true;

    if (info.domain == tryst::mod::TargetDomain::generator)
    {
        if (info.slot >= patch.generators.size() || !patch.generators[info.slot].enabled)
            return false;
        if (startsWith(id, "generator."))
            return true;

        const auto kind = patch.generators[info.slot].kind;
        if (kind == tryst::dsp::EngineKind::analog) return startsWith(id, "engine.analog.");
        if (kind == tryst::dsp::EngineKind::wavetable) return startsWith(id, "engine.wavetable.");
        if (kind == tryst::dsp::EngineKind::sampler) return startsWith(id, "engine.sampler.");
        if (kind == tryst::dsp::EngineKind::granular) return startsWith(id, "engine.granular.");
        if (kind == tryst::dsp::EngineKind::noise) return startsWith(id, "engine.noise.");
        return false;
    }

    if (info.domain == tryst::mod::TargetDomain::effect)
    {
        if (info.slot >= patch.effects.size() || !patch.effects[info.slot].enabled)
            return false;
        if (id == "fx.mix")
            return true;
        const auto kind = patch.effects[info.slot].kind;
        if (kind == tryst::fx::EffectKind::drive) return startsWith(id, "fx.drive.");
        if (kind == tryst::fx::EffectKind::delay) return startsWith(id, "fx.delay.");
    }

    return false;
}

void PluginEngine::dispatchEvent(const HostEvent& event) noexcept
{
    switch (event.type)
    {
        case HostEventType::noteOn:
            manager.noteOn({ event.key, event.velocity, event.pressure, event.timbre,
                             event.pitchBendSemitones, 0.0f, event.noteId });
            break;
        case HostEventType::noteOff:
            manager.noteOff(event.key, event.velocity, event.noteId);
            break;
        case HostEventType::noteExpression:
            manager.updateNoteExpression(event.key, event.pressure, event.timbre,
                                         event.pitchBendSemitones, event.noteId);
            break;
        case HostEventType::parameter:
            (void) setHostParameter(event.parameterId, event.normalizedValue);
            break;
    }
}

void PluginEngine::renderRange(std::span<float> left, std::span<float> right,
                               std::size_t offset, std::size_t count) noexcept
{
    tryst::dsp::ProcessContext context {
        preparedSampleRate,
        left.subspan(offset, count),
        right.subspan(offset, count)
    };
    manager.render(context);
}
}
