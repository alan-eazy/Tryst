#pragma once

#include "Voice.h"
#include "../Modulation/ModulationRuntime.h"
#include "../Modulation/TargetValueStore.h"
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
    [[nodiscard]] bool setEngineAsset(std::size_t slot, std::shared_ptr<const EngineAsset> asset) noexcept;
    [[nodiscard]] bool setFilterParameter(tryst::params::DestinationId destination, float normalized) noexcept;
    void setAmpEnvelope(tryst::mod::AdsrEnvelope::Parameters parameters) noexcept;

    [[nodiscard]] bool addModulationRoute(tryst::mod::Route route) noexcept;
    void clearModulationRoutes() noexcept;
    [[nodiscard]] bool setMacro(std::size_t index, float normalized) noexcept;
    void setMorphX(float normalized) noexcept;
    void setMorphY(float normalized) noexcept;
    [[nodiscard]] bool setModulationSource(tryst::mod::SourceId source, float value) noexcept;

    [[nodiscard]] bool configureVoiceLfo(std::size_t index, float rateHz, tryst::mod::LfoWaveform waveform) noexcept;
    [[nodiscard]] bool configureVoiceMseg(std::size_t index, std::span<const tryst::mod::MsegPoint> points, float rateHz, bool loop) noexcept;
    [[nodiscard]] bool configureVoiceStep(std::size_t index, std::span<const float> values, float rateHz, bool loop) noexcept;
    [[nodiscard]] bool addFastModulationRoute(tryst::mod::Route route) noexcept;
    void clearFastModulationRoutes() noexcept;
    [[nodiscard]] std::size_t fastModulationRouteCount() const noexcept;

    void noteOn(const NoteEvent& event) noexcept;
    void noteOff(std::int32_t note, float releaseVelocity = 0.0f) noexcept;
    void render(ProcessContext& context) noexcept;

    [[nodiscard]] std::size_t activeVoiceCount() const noexcept;
    [[nodiscard]] std::size_t modulationRouteCount() const noexcept { return modulation.routeCount(); }

private:
    [[nodiscard]] Voice& chooseVoice() noexcept;
    [[nodiscard]] bool broadcastTarget(tryst::mod::TargetId target, float normalized) noexcept;
    [[nodiscard]] bool ensureTargetBase(tryst::mod::TargetId target) noexcept;
    void refreshTarget(tryst::mod::TargetId target) noexcept;
    void refreshAllModulation() noexcept;

    std::array<Voice, maxVoices> voices;
    tryst::mod::ModulationRuntime modulation;
    tryst::mod::TargetValueStore baseTargets;
    std::uint64_t ageCounter = 0;
};
}
