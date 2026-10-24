#pragma once

#include "EngineFactory.h"
#include "EngineSlot.h"
#include "FilterChain.h"
#include "../Modulation/AdsrEnvelope.h"
#include "../Modulation/FastModulationRuntime.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace tryst::dsp
{
class Voice
{
public:
    static constexpr std::size_t generatorSlots = 4;

    void prepare(double newSampleRate, std::uint32_t maxBlockSize);
    void reset() noexcept;

    [[nodiscard]] bool setEngineKind(std::size_t slot, EngineKind kind);
    void clearEngine(std::size_t slot) noexcept;
    [[nodiscard]] bool setEngineParameter(std::size_t slot, tryst::params::DestinationId destination, float normalized) noexcept;
    [[nodiscard]] bool setEngineAsset(std::size_t slot, std::shared_ptr<const EngineAsset> asset) noexcept;
    [[nodiscard]] bool setFilterParameter(tryst::params::DestinationId destination, float normalized) noexcept;
    void setAmpEnvelope(tryst::mod::AdsrEnvelope::Parameters parameters) noexcept;
    [[nodiscard]] bool configureFastLfo(std::size_t index, float rateHz, tryst::mod::LfoWaveform waveform) noexcept;
    [[nodiscard]] bool configureFastMseg(std::size_t index, std::span<const tryst::mod::MsegPoint> points, float rateHz, bool loop) noexcept;
    [[nodiscard]] bool configureFastStep(std::size_t index, std::span<const float> values, float rateHz, bool loop) noexcept;
    [[nodiscard]] bool addFastModulationRoute(tryst::mod::Route route) noexcept;
    void clearFastModulationRoutes() noexcept;
    [[nodiscard]] std::size_t fastModulationRouteCount() const noexcept { return fastModulation.routeCount(); }

    void start(const NoteEvent& event, std::uint64_t newAge) noexcept;
    void release(std::int32_t note, float releaseVelocity) noexcept;
    void render(ProcessContext& context) noexcept;

    [[nodiscard]] bool isActive() const noexcept { return active; }
    [[nodiscard]] bool isReleasing() const noexcept { return releasing; }
    [[nodiscard]] bool matches(std::int32_t note) const noexcept { return active && currentNote == note; }
    [[nodiscard]] std::uint64_t age() const noexcept { return voiceAge; }
    [[nodiscard]] std::int32_t note() const noexcept { return currentNote; }

private:
    void stopEngines() noexcept;
    void applyFastTarget(tryst::mod::TargetId target, float normalized) noexcept;

    std::array<EngineSlot, generatorSlots> slots;
    tryst::mod::AdsrEnvelope ampEnvelope;
    tryst::mod::FastModulationRuntime fastModulation;
    FilterChain filters;
    std::vector<float> scratchLeft;
    std::vector<float> scratchRight;
    double sampleRate = 48000.0;
    std::uint32_t preparedBlockSize = 0;
    std::int32_t currentNote = -1;
    std::uint64_t voiceAge = 0;
    bool active = false;
    bool releasing = false;
};
}
