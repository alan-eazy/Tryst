#pragma once

#include "../Core/ParameterRegistry.h"
#include <cstdint>
#include <span>
#include <string_view>

namespace tryst::dsp
{
struct ProcessContext
{
    double sampleRate = 48000.0;
    std::span<float> left;
    std::span<float> right;
};

struct NoteEvent
{
    std::int32_t note = 60;
    float velocity = 1.0f;
    float pressure = 0.0f;
    float timbre = 0.0f;
    float pitchBendSemitones = 0.0f;
};

enum class EngineKind
{
    analog,
    wavetable,
    sampler,
    granular,
    spectral,
    additive,
    physicalModel,
    noise
};

class Engine
{
public:
    virtual ~Engine() = default;

    [[nodiscard]] virtual EngineKind kind() const noexcept = 0;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    virtual void prepare(double sampleRate, std::uint32_t maxBlockSize) = 0;
    virtual void reset() noexcept = 0;
    virtual void noteOn(const NoteEvent& note) noexcept = 0;
    virtual void noteOff(std::int32_t note, float releaseVelocity) noexcept = 0;
    virtual void render(ProcessContext& context) noexcept = 0;
    [[nodiscard]] virtual bool setParameter(tryst::params::DestinationId destination, float normalized) noexcept = 0;
};
}
