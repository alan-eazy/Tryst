#pragma once

#include "Engine.h"
#include "SampleData.h"
#include <memory>

namespace tryst::dsp
{
class SamplerEngine final : public Engine
{
public:
    SamplerEngine();

    [[nodiscard]] EngineKind kind() const noexcept override { return EngineKind::sampler; }
    [[nodiscard]] std::string_view name() const noexcept override { return "Sampler"; }

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
    [[nodiscard]] float interpolate(bool rightChannel, double position) const noexcept;
    [[nodiscard]] double regionStartFrame() const noexcept;
    [[nodiscard]] double regionEndFrame() const noexcept;
    [[nodiscard]] double loopStartFrame() const noexcept;
    [[nodiscard]] double loopEndFrame() const noexcept;
    void updateIncrement() noexcept;

    std::shared_ptr<const SampleData> sample;
    double hostSampleRate = 48000.0;
    double playhead = 0.0;
    double increment = 1.0;
    float baseMidiNote = 60.0f;
    float playedMidiNote = 60.0f;
    float detuneSemitones = 0.0f;
    float velocity = 0.0f;
    float level = 0.65f;
    float startNormalized = 0.0f;
    float endNormalized = 1.0f;
    float loopStartNormalized = 0.0f;
    float loopEndNormalized = 1.0f;
    bool loopEnabled = false;
    bool reverse = false;
    bool active = false;
};
}
