#pragma once

#include "Engine.h"
#include "SampleData.h"
#include <array>
#include <cstdint>
#include <memory>

namespace tryst::dsp
{
class GranularEngine final : public Engine
{
public:
    static constexpr std::size_t maxGrains = 32;

    GranularEngine();

    [[nodiscard]] EngineKind kind() const noexcept override { return EngineKind::granular; }
    [[nodiscard]] std::string_view name() const noexcept override { return "Granular"; }

    void prepare(double newSampleRate, std::uint32_t maxBlockSize) override;
    void reset() noexcept override;
    void noteOn(const NoteEvent& note) noexcept override;
    void noteOff(std::int32_t note, float releaseVelocity) noexcept override;
    [[nodiscard]] bool setPitchBendSemitones(float semitones) noexcept override;
    void render(ProcessContext& context) noexcept override;
    [[nodiscard]] bool setParameter(tryst::params::DestinationId destination, float normalized) noexcept override;
    [[nodiscard]] bool setAsset(std::shared_ptr<const EngineAsset> next) noexcept override;

    void setSample(std::shared_ptr<const SampleData> next) noexcept;

private:
    struct Grain
    {
        double position = 0.0;
        double increment = 1.0;
        std::uint32_t age = 0;
        std::uint32_t duration = 1;
        bool active = false;
    };

    [[nodiscard]] float interpolate(bool rightChannel, double position) const noexcept;
    [[nodiscard]] float nextRandomBipolar() noexcept;
    [[nodiscard]] double grainIncrement() const noexcept;
    [[nodiscard]] double grainSizeSeconds() const noexcept;
    [[nodiscard]] double grainsPerSecond() const noexcept;
    [[nodiscard]] double spawnIntervalSamples() const noexcept;
    [[nodiscard]] float overlapNormalization() const noexcept;
    void spawnGrain() noexcept;
    void clearGrains() noexcept;

    std::shared_ptr<const SampleData> sample;
    std::array<Grain, maxGrains> grains {};
    double hostSampleRate = 48000.0;
    double spawnCountdown = 0.0;
    float baseMidiNote = 60.0f;
    float playedMidiNote = 60.0f;
    float detuneSemitones = 0.0f;
    float grainPitchSemitones = 0.0f;
    float velocity = 0.0f;
    float level = 0.65f;
    float positionNormalized = 0.5f;
    float sizeNormalized = 0.30f;
    float densityNormalized = 0.30f;
    float sprayNormalized = 0.0f;
    std::uint32_t randomState = 0x6d2b79f5u;
    bool active = false;
};
}
