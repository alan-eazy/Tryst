#pragma once

#include "Engine.h"
#include <cmath>

namespace tryst::dsp
{
class AnalogEngine final : public Engine
{
public:
    [[nodiscard]] EngineKind kind() const noexcept override { return EngineKind::analog; }
    [[nodiscard]] std::string_view name() const noexcept override { return "Analog"; }

    void prepare(double newSampleRate, std::uint32_t) override;
    void reset() noexcept override;
    void noteOn(const NoteEvent& note) noexcept override;
    void noteOff(std::int32_t note, float releaseVelocity) noexcept override;
    void render(ProcessContext& context) noexcept override;
    [[nodiscard]] bool setParameter(tryst::params::DestinationId destination, float normalized) noexcept override;

    void setShape(float value) noexcept;
    void setLevel(float value) noexcept;
    void setDetuneSemitones(float value) noexcept;

private:
    [[nodiscard]] float oscillator(float phaseValue) const noexcept;
    [[nodiscard]] static float midiToHz(float midiNote) noexcept;
    void updateFrequency() noexcept;

    double sampleRate = 48000.0;
    float phase = 0.0f;
    float frequency = 440.0f;
    float playedMidiNote = 69.0f;
    float velocity = 0.0f;
    float level = 0.65f;
    float shape = 0.0f; // 0=sine, .5=saw, 1=square; continuous morph
    float detuneSemitones = 0.0f;
    bool active = false;
};
}
