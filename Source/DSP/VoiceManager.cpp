#include "VoiceManager.h"
#include <algorithm>

namespace tryst::dsp
{
void VoiceManager::prepare(double sampleRate, std::uint32_t maxBlockSize)
{
    for (auto& voice : voices)
    {
        voice.prepare(sampleRate, maxBlockSize);
        if (!voice.setEngineKind(0, EngineKind::analog))
            voice.clearEngine(0);
    }

    fxRack.prepare(sampleRate, maxBlockSize);
    ageCounter = 0;
}

void VoiceManager::reset() noexcept
{
    for (auto& voice : voices)
        voice.reset();
    fxRack.reset();
    ageCounter = 0;
}

bool VoiceManager::setEngineKind(std::size_t slot, EngineKind kind)
{
    if (slot >= Voice::generatorSlots)
        return false;

    bool success = true;
    for (auto& voice : voices)
        success = voice.setEngineKind(slot, kind) && success;

    for (std::size_t i = 0; i < baseTargets.size(); ++i)
    {
        const auto& entry = baseTargets[i];
        if (entry.target.domain == tryst::mod::TargetDomain::generator && entry.target.slot == slot)
            refreshTarget(entry.target);
    }

    return success;
}

void VoiceManager::clearEngine(std::size_t slot) noexcept
{
    for (auto& voice : voices)
        voice.clearEngine(slot);
}

bool VoiceManager::setEngineParameter(std::size_t slot,
                                      tryst::params::DestinationId destination,
                                      float normalized) noexcept
{
    if (slot >= Voice::generatorSlots)
        return false;

    const tryst::mod::TargetId target { tryst::mod::TargetDomain::generator,
                                        static_cast<std::uint8_t>(slot),
                                        destination };
    if (!baseTargets.set(target, normalized))
        return false;

    return broadcastTarget(target, modulation.evaluate(target, normalized));
}

bool VoiceManager::setEngineAsset(std::size_t slot, std::shared_ptr<const EngineAsset> asset) noexcept
{
    if (slot >= Voice::generatorSlots || !asset)
        return false;

    bool success = true;
    for (auto& voice : voices)
        success = voice.setEngineAsset(slot, asset) && success;
    return success;
}

bool VoiceManager::setFilterParameter(tryst::params::DestinationId destination, float normalized) noexcept
{
    const tryst::mod::TargetId target { tryst::mod::TargetDomain::filter, 0, destination };
    if (!baseTargets.set(target, normalized))
        return false;

    return broadcastTarget(target, modulation.evaluate(target, normalized));
}

bool VoiceManager::setEffectKind(std::size_t slot, tryst::fx::EffectKind kind)
{
    if (slot >= tryst::fx::FxRack::maxSlots)
        return false;
    if (!fxRack.setEffectKind(slot, kind))
        return false;

    for (std::size_t i = 0; i < baseTargets.size(); ++i)
    {
        const auto& entry = baseTargets[i];
        if (entry.target.domain == tryst::mod::TargetDomain::effect && entry.target.slot == slot)
            refreshTarget(entry.target);
    }
    return true;
}

void VoiceManager::clearEffect(std::size_t slot) noexcept
{
    fxRack.clearEffect(slot);
}

bool VoiceManager::swapEffects(std::size_t a, std::size_t b) noexcept
{
    // Rack order is independent from slot identity, so slot-targeted automation
    // and modulation continue to address the same effect after reordering.
    return fxRack.swapSlots(a, b);
}

bool VoiceManager::setEffectOrder(std::span<const std::uint8_t> slotOrder) noexcept
{
    return fxRack.setOrder(slotOrder);
}

bool VoiceManager::setEffectParameter(std::size_t slot,
                                      tryst::params::DestinationId destination,
                                      float normalized) noexcept
{
    if (slot >= tryst::fx::FxRack::maxSlots)
        return false;
    const tryst::mod::TargetId target { tryst::mod::TargetDomain::effect,
                                        static_cast<std::uint8_t>(slot),
                                        destination };
    if (!baseTargets.set(target, normalized))
        return false;
    return broadcastTarget(target, modulation.evaluate(target, normalized));
}

void VoiceManager::setAmpEnvelope(tryst::mod::AdsrEnvelope::Parameters parameters) noexcept
{
    for (auto& voice : voices)
        voice.setAmpEnvelope(parameters);
}

bool VoiceManager::addModulationRoute(tryst::mod::Route route) noexcept
{
    if (route.target.domain == tryst::mod::TargetDomain::generator && route.target.slot >= Voice::generatorSlots)
        return false;
    if (route.target.domain == tryst::mod::TargetDomain::effect && route.target.slot >= tryst::fx::FxRack::maxSlots)
        return false;

    if (!ensureTargetBase(route.target))
        return false;

    if (!modulation.addRoute(route))
        return false;

    refreshTarget(route.target);
    return true;
}

void VoiceManager::clearModulationRoutes() noexcept
{
    modulation.clearRoutes();
    refreshAllModulation();
}

bool VoiceManager::setMacro(std::size_t index, float normalized) noexcept
{
    if (!modulation.setMacro(index, normalized))
        return false;

    refreshAllModulation();
    return true;
}

void VoiceManager::setMorphX(float normalized) noexcept
{
    modulation.setMorphX(normalized);
    refreshAllModulation();
}

void VoiceManager::setMorphY(float normalized) noexcept
{
    modulation.setMorphY(normalized);
    refreshAllModulation();
}

bool VoiceManager::setModulationSource(tryst::mod::SourceId source, float value) noexcept
{
    if (!modulation.setSource(source, value))
        return false;

    refreshAllModulation();
    return true;
}

bool VoiceManager::configureVoiceLfo(std::size_t index, float rateHz, tryst::mod::LfoWaveform waveform) noexcept
{
    bool success = true;
    for (auto& voice : voices)
        success = voice.configureFastLfo(index, rateHz, waveform) && success;
    return success;
}

bool VoiceManager::configureVoiceMseg(std::size_t index,
                                      std::span<const tryst::mod::MsegPoint> points,
                                      float rateHz,
                                      bool loop) noexcept
{
    bool success = true;
    for (auto& voice : voices)
        success = voice.configureFastMseg(index, points, rateHz, loop) && success;
    return success;
}

bool VoiceManager::configureVoiceStep(std::size_t index,
                                      std::span<const float> values,
                                      float rateHz,
                                      bool loop) noexcept
{
    bool success = true;
    for (auto& voice : voices)
        success = voice.configureFastStep(index, values, rateHz, loop) && success;
    return success;
}

bool VoiceManager::addFastModulationRoute(tryst::mod::Route route) noexcept
{
    if (route.target.domain == tryst::mod::TargetDomain::global
        || route.target.domain == tryst::mod::TargetDomain::effect)
        return false;
    if (route.target.domain == tryst::mod::TargetDomain::generator && route.target.slot >= Voice::generatorSlots)
        return false;

    const auto* descriptor = tryst::params::ParameterRegistry::find(route.target.parameter);
    if (descriptor == nullptr || !descriptor->audioRateCapable)
        return false;

    if (!ensureTargetBase(route.target))
        return false;

    bool success = true;
    for (auto& voice : voices)
        success = voice.addFastModulationRoute(route) && success;

    refreshTarget(route.target);
    return success;
}

void VoiceManager::clearFastModulationRoutes() noexcept
{
    for (auto& voice : voices)
        voice.clearFastModulationRoutes();
    refreshAllModulation();
}

std::size_t VoiceManager::fastModulationRouteCount() const noexcept
{
    return voices.front().fastModulationRouteCount();
}

bool VoiceManager::setTuningOffsetSemitones(std::int32_t note, float semitones) noexcept
{
    return tuning.setOffsetSemitones(note, semitones);
}

bool VoiceManager::setOctaveTuning(std::span<const float> offsets) noexcept
{
    return tuning.setOctaveOffsets(offsets);
}

void VoiceManager::resetTuning() noexcept
{
    tuning.resetEqualTemperament();
}

float VoiceManager::tuningOffsetSemitones(std::int32_t note) const noexcept
{
    return tuning.offsetSemitones(note);
}

void VoiceManager::noteOn(const NoteEvent& event) noexcept
{
    auto tunedEvent = event;
    tunedEvent.tuningOffsetSemitones += tuning.offsetSemitones(event.note);
    auto& voice = chooseVoice();
    voice.start(tunedEvent, ++ageCounter);
}

void VoiceManager::noteOff(std::int32_t note, float releaseVelocity) noexcept
{
    for (auto& voice : voices)
        if (voice.matches(note))
            voice.release(note, releaseVelocity);
}

void VoiceManager::updateNoteExpression(std::int32_t note,
                                        float pressure,
                                        float timbre,
                                        float pitchBendSemitones) noexcept
{
    for (auto& voice : voices)
        if (voice.matches(note))
            voice.updateExpression(pressure, timbre, pitchBendSemitones);
}

void VoiceManager::render(ProcessContext& context) noexcept
{
    for (auto& voice : voices)
        if (voice.isActive())
            voice.render(context);
    fxRack.process(context.left, context.right);
}

std::size_t VoiceManager::activeVoiceCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(voices.begin(), voices.end(), [](const Voice& voice) {
        return voice.isActive();
    }));
}

Voice& VoiceManager::chooseVoice() noexcept
{
    if (auto it = std::find_if(voices.begin(), voices.end(), [](const Voice& voice) { return !voice.isActive(); });
        it != voices.end())
        return *it;

    auto* oldest = &voices.front();
    for (auto& voice : voices)
        if (voice.age() < oldest->age())
            oldest = &voice;

    return *oldest;
}

bool VoiceManager::broadcastTarget(tryst::mod::TargetId target, float normalized) noexcept
{
    if (target.domain == tryst::mod::TargetDomain::generator)
    {
        bool success = true;
        for (auto& voice : voices)
            success = voice.setEngineParameter(target.slot, target.parameter, normalized) && success;
        return success;
    }

    if (target.domain == tryst::mod::TargetDomain::filter)
    {
        bool success = true;
        for (auto& voice : voices)
            success = voice.setFilterParameter(target.parameter, normalized) && success;
        return success;
    }

    if (target.domain == tryst::mod::TargetDomain::effect)
        return fxRack.setParameter(target.slot, target.parameter, normalized);

    return false;
}

bool VoiceManager::ensureTargetBase(tryst::mod::TargetId target) noexcept
{
    if (baseTargets.find(target) != nullptr)
        return true;

    const auto* descriptor = tryst::params::ParameterRegistry::find(target.parameter);
    if (descriptor == nullptr)
        return false;

    return baseTargets.set(target, descriptor->defaultNormalized);
}

void VoiceManager::refreshTarget(tryst::mod::TargetId target) noexcept
{
    const auto* entry = baseTargets.find(target);
    if (entry == nullptr)
        return;

    (void) broadcastTarget(target, modulation.evaluate(target, entry->baseValue));
}

void VoiceManager::refreshAllModulation() noexcept
{
    for (std::size_t i = 0; i < baseTargets.size(); ++i)
        refreshTarget(baseTargets[i].target);
}
}
