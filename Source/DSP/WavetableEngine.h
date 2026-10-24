#pragma once

#include "Engine.h"
#include "WavetableData.h"
#include <memory>

namespace tryst::dsp
{
class WavetableEngine final : public Engine
{
public:
    WavetableEngine();

    [[nodiscard]] EngineKind kind() const noexcept override { return EngineKind::wavetable; }
    [[nodiscard]] std::string_view name() const noexcept override { return "Wavetable"; }

    void prepare(double newSampleRate, std::uint32_t maxBlockSize) override;
    void reset() noexcept override;
    void noteOn(const NoteEvent& note) noexcept override;
    void noteOff(std::int32_t note, float releaseVelocity) noexcept override;
    void render(ProcessContext& context) noexcept override;
    [[nodiscard]] bool setParameter(tryst::params::DestinationId destination, float normalized) noexcept override;
    [[nodiscard]] bool setAsset(std::shared_ptr<const EngineAsset> next) noexcept override;

    void setWavetable(std::shared_ptr<const WavetableData> next) noexcept;
    void setPosition(float normalized) noexcept;
    void setLevel(float normalized) noexcept;
    void setDetuneSemitones(float semitones) noexcept;

private:
    [[nodiscard]] float tableSample(float phaseValue) const noexcept;
    [[nodiscard]] static float midiToHz(float midiNote) noexcept;
    void updateFrequency() noexcept;

    std::shared_ptr<const WavetableData> wavetable;
    double sampleRate = 48000.0;
    float phase = 0.0f;
    float frequency = 440.0f;
    float playedMidiNote = 69.0f;
    float velocity = 0.0f;
    float level = 0.65f;
    float position = 0.0f;
    float detuneSemitones = 0.0f;
    bool active = false;
};
}
