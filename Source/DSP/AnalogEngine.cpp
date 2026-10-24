#include "AnalogEngine.h"
#include <algorithm>
#include <numbers>

namespace tryst::dsp
{
void AnalogEngine::prepare(double newSampleRate, std::uint32_t)
{
    sampleRate = newSampleRate > 1.0 ? newSampleRate : 48000.0;
    reset();
}

void AnalogEngine::reset() noexcept
{
    phase = 0.0f;
    active = false;
    velocity = 0.0f;
}

void AnalogEngine::noteOn(const NoteEvent& note) noexcept
{
    playedMidiNote = static_cast<float>(note.note) + note.pitchBendSemitones;
    updateFrequency();
    velocity = std::clamp(note.velocity, 0.0f, 1.0f);
    active = true;
}

void AnalogEngine::noteOff(std::int32_t, float) noexcept
{
    active = false;
}

void AnalogEngine::render(ProcessContext& context) noexcept
{
    if (!active || context.left.empty())
        return;

    const auto samples = std::min(context.left.size(), context.right.size());
    const auto increment = static_cast<float>(frequency / sampleRate);

    for (std::size_t i = 0; i < samples; ++i)
    {
        const float sample = oscillator(phase) * velocity * level;
        context.left[i] += sample;
        context.right[i] += sample;

        phase += increment;
        phase -= std::floor(phase);
    }
}

bool AnalogEngine::setParameter(tryst::params::DestinationId destination, float normalized) noexcept
{
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    if (destination == tryst::params::destination::generatorLevel)
    {
        setLevel(normalized);
        return true;
    }

    if (destination == tryst::params::destination::generatorTune)
    {
        setDetuneSemitones((normalized - 0.5f) * 48.0f);
        return true;
    }

    if (destination == tryst::params::destination::analogShape)
    {
        setShape(normalized);
        return true;
    }

    return false;
}

void AnalogEngine::setShape(float value) noexcept
{
    shape = std::clamp(value, 0.0f, 1.0f);
}

void AnalogEngine::setLevel(float value) noexcept
{
    level = std::clamp(value, 0.0f, 1.0f);
}

void AnalogEngine::setDetuneSemitones(float value) noexcept
{
    detuneSemitones = std::clamp(value, -24.0f, 24.0f);
    updateFrequency();
}

float AnalogEngine::oscillator(float p) const noexcept
{
    const float sine = std::sin(2.0f * std::numbers::pi_v<float> * p);
    const float saw = (2.0f * p) - 1.0f;
    const float square = p < 0.5f ? 1.0f : -1.0f;

    if (shape <= 0.5f)
    {
        const float t = shape * 2.0f;
        return sine + (saw - sine) * t;
    }

    const float t = (shape - 0.5f) * 2.0f;
    return saw + (square - saw) * t;
}

float AnalogEngine::midiToHz(float midiNote) noexcept
{
    return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
}

void AnalogEngine::updateFrequency() noexcept
{
    frequency = midiToHz(playedMidiNote + detuneSemitones);
}
}
