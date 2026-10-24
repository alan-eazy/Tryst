#pragma once

#include "Voice.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace tryst::dsp
{
class VoiceManager
{
public:
    static constexpr std::size_t maxVoices = 16;

    void prepare(double sampleRate, std::uint32_t maxBlockSize);
    void reset() noexcept;

    [[nodiscard]] bool setEngineKind(std::size_t slot, EngineKind kind);
    void clearEngine(std::size_t slot) noexcept;
    [[nodiscard]] bool setEngineParameter(std::size_t slot, tryst::params::DestinationId destination, float normalized) noexcept;
    void setAmpEnvelope(tryst::mod::AdsrEnvelope::Parameters parameters) noexcept;

    void noteOn(const NoteEvent& event) noexcept;
    void noteOff(std::int32_t note, float releaseVelocity = 0.0f) noexcept;
    void render(ProcessContext& context) noexcept;

    [[nodiscard]] std::size_t activeVoiceCount() const noexcept;

private:
    [[nodiscard]] Voice& chooseVoice() noexcept;

    std::array<Voice, maxVoices> voices;
    std::uint64_t ageCounter = 0;
};
}
