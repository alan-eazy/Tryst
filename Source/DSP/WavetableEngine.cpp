#include "WavetableEngine.h"
#include <algorithm>
#include <cmath>

namespace tryst::dsp
{
WavetableEngine::WavetableEngine()
    : wavetable(WavetableData::createBasic())
{
}

void WavetableEngine::prepare(double newSampleRate, std::uint32_t)
{
    sampleRate = newSampleRate > 1.0 ? newSampleRate : 48000.0;
    reset();
}

void WavetableEngine::reset() noexcept
{
    phase = 0.0f;
    active = false;
    velocity = 0.0f;
}

void WavetableEngine::noteOn(const NoteEvent& note) noexcept
{
    playedMidiNote = static_cast<float>(note.note) + note.pitchBendSemitones;
    updateFrequency();
    velocity = std::clamp(note.velocity, 0.0f, 1.0f);
    active = true;
}

void WavetableEngine::noteOff(std::int32_t, float) noexcept
{
    active = false;
}

void WavetableEngine::render(ProcessContext& context) noexcept
{
    if (!active || !wavetable || context.left.empty() || context.right.empty())
        return;

    const auto samples = std::min(context.left.size(), context.right.size());
    const auto increment = static_cast<float>(frequency / sampleRate);

    for (std::size_t i = 0; i < samples; ++i)
    {
        const auto sampleValue = tableSample(phase) * velocity * level;
        context.left[i] += sampleValue;
        context.right[i] += sampleValue;

        phase += increment;
        phase -= std::floor(phase);
    }
}

bool WavetableEngine::setParameter(tryst::params::DestinationId destination, float normalized) noexcept
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

    if (destination == tryst::params::destination::wavetablePosition)
    {
        setPosition(normalized);
        return true;
    }

    return false;
}

bool WavetableEngine::setAsset(std::shared_ptr<const EngineAsset> next) noexcept
{
    const auto table = std::dynamic_pointer_cast<const WavetableData>(std::move(next));
    if (!table)
        return false;

    setWavetable(table);
    return true;
}

void WavetableEngine::setWavetable(std::shared_ptr<const WavetableData> next) noexcept
{
    if (next && next->frames() > 0 && next->samplesPerFrame() > 1)
        wavetable = std::move(next);
}

void WavetableEngine::setPosition(float normalized) noexcept
{
    position = std::clamp(normalized, 0.0f, 1.0f);
}

void WavetableEngine::setLevel(float normalized) noexcept
{
    level = std::clamp(normalized, 0.0f, 1.0f);
}

void WavetableEngine::setDetuneSemitones(float semitones) noexcept
{
    detuneSemitones = std::clamp(semitones, -24.0f, 24.0f);
    updateFrequency();
}

float WavetableEngine::tableSample(float phaseValue) const noexcept
{
    const auto frames = wavetable->frames();
    const auto tableSize = wavetable->samplesPerFrame();
    if (frames == 0 || tableSize < 2)
        return 0.0f;

    const auto framePosition = position * static_cast<float>(frames - 1);
    const auto frame0 = static_cast<std::size_t>(std::floor(framePosition));
    const auto frame1 = std::min(frame0 + 1, frames - 1);
    const auto frameBlend = framePosition - static_cast<float>(frame0);

    const auto tablePosition = phaseValue * static_cast<float>(tableSize);
    const auto index0 = static_cast<std::size_t>(std::floor(tablePosition)) % tableSize;
    const auto index1 = (index0 + 1) % tableSize;
    const auto sampleBlend = tablePosition - std::floor(tablePosition);

    const auto a0 = wavetable->sample(frame0, index0);
    const auto a1 = wavetable->sample(frame0, index1);
    const auto b0 = wavetable->sample(frame1, index0);
    const auto b1 = wavetable->sample(frame1, index1);

    const auto frameASample = a0 + (a1 - a0) * sampleBlend;
    const auto frameBSample = b0 + (b1 - b0) * sampleBlend;
    return frameASample + (frameBSample - frameASample) * frameBlend;
}

float WavetableEngine::midiToHz(float midiNote) noexcept
{
    return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
}

void WavetableEngine::updateFrequency() noexcept
{
    frequency = midiToHz(playedMidiNote + detuneSemitones);
}
}
