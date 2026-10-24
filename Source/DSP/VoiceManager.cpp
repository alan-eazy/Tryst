#include "VoiceManager.h"
#include <algorithm>
#include <limits>

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

    ageCounter = 0;
}

void VoiceManager::reset() noexcept
{
    for (auto& voice : voices)
        voice.reset();
    ageCounter = 0;
}

bool VoiceManager::setEngineKind(std::size_t slot, EngineKind kind)
{
    bool success = true;
    for (auto& voice : voices)
        success = voice.setEngineKind(slot, kind) && success;
    return success;
}

void VoiceManager::clearEngine(std::size_t slot) noexcept
{
    for (auto& voice : voices)
        voice.clearEngine(slot);
}

bool VoiceManager::setEngineParameter(std::size_t slot, tryst::params::DestinationId destination, float normalized) noexcept
{
    bool success = true;
    for (auto& voice : voices)
        success = voice.setEngineParameter(slot, destination, normalized) && success;
    return success;
}

void VoiceManager::setAmpEnvelope(tryst::mod::AdsrEnvelope::Parameters parameters) noexcept
{
    for (auto& voice : voices)
        voice.setAmpEnvelope(parameters);
}

void VoiceManager::noteOn(const NoteEvent& event) noexcept
{
    auto& voice = chooseVoice();
    voice.start(event, ++ageCounter);
}

void VoiceManager::noteOff(std::int32_t note, float releaseVelocity) noexcept
{
    for (auto& voice : voices)
        if (voice.matches(note))
            voice.release(note, releaseVelocity);
}

void VoiceManager::render(ProcessContext& context) noexcept
{
    for (auto& voice : voices)
        if (voice.isActive())
            voice.render(context);
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
}
