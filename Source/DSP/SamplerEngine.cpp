#include "SamplerEngine.h"
#include <algorithm>
#include <cmath>

namespace tryst::dsp
{
SamplerEngine::SamplerEngine()
    : sample(SampleData::createBasic())
{
}

void SamplerEngine::prepare(double newSampleRate, std::uint32_t)
{
    hostSampleRate = newSampleRate > 1.0 ? newSampleRate : 48000.0;
    updateIncrement();
    reset();
}

void SamplerEngine::reset() noexcept
{
    playhead = reverse ? regionEndFrame() : regionStartFrame();
    velocity = 0.0f;
    active = false;
}

void SamplerEngine::noteOn(const NoteEvent& note) noexcept
{
    playedMidiNote = static_cast<float>(note.note) + note.pitchBendSemitones;
    velocity = std::clamp(note.velocity, 0.0f, 1.0f);
    updateIncrement();
    playhead = reverse ? regionEndFrame() : regionStartFrame();
    active = sample && sample->frames() > 1;
}

void SamplerEngine::noteOff(std::int32_t, float) noexcept
{
    // Voice-level amplitude envelopes own release timing. The sample keeps running
    // until that envelope finishes and the voice resets the engine.
}

void SamplerEngine::render(ProcessContext& context) noexcept
{
    if (!active || !sample || context.left.empty() || context.right.empty())
        return;

    const auto samples = std::min(context.left.size(), context.right.size());
    const auto startFrame = regionStartFrame();
    const auto endFrame = regionEndFrame();
    const auto loopStart = std::clamp(loopStartFrame(), startFrame, endFrame);
    const auto loopEnd = std::clamp(loopEndFrame(), loopStart + 1.0, endFrame);

    for (std::size_t i = 0; i < samples && active; ++i)
    {
        context.left[i] += interpolate(false, playhead) * velocity * level;
        context.right[i] += interpolate(true, playhead) * velocity * level;

        playhead += reverse ? -increment : increment;

        if (!reverse)
        {
            if (loopEnabled && loopEnd > loopStart + 1.0 && playhead >= loopEnd)
            {
                const auto overshoot = playhead - loopEnd;
                playhead = loopStart + std::max(0.0, overshoot);
            }
            else if (playhead >= endFrame)
            {
                active = false;
            }
        }
        else
        {
            if (loopEnabled && loopEnd > loopStart + 1.0 && playhead <= loopStart)
            {
                const auto overshoot = loopStart - playhead;
                playhead = loopEnd - std::max(0.0, overshoot);
            }
            else if (playhead <= startFrame)
            {
                active = false;
            }
        }
    }
}

bool SamplerEngine::setParameter(tryst::params::DestinationId destination, float normalized) noexcept
{
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    if (destination == tryst::params::destination::generatorLevel)
    {
        level = normalized;
        return true;
    }
    if (destination == tryst::params::destination::generatorTune)
    {
        detuneSemitones = (normalized - 0.5f) * 48.0f;
        updateIncrement();
        return true;
    }
    if (destination == tryst::params::destination::samplerStart)
    {
        startNormalized = std::min(normalized, endNormalized);
        return true;
    }
    if (destination == tryst::params::destination::samplerEnd)
    {
        endNormalized = std::max(normalized, startNormalized);
        return true;
    }
    if (destination == tryst::params::destination::samplerLoopEnabled)
    {
        loopEnabled = normalized >= 0.5f;
        return true;
    }
    if (destination == tryst::params::destination::samplerLoopStart)
    {
        loopStartNormalized = std::min(normalized, loopEndNormalized);
        return true;
    }
    if (destination == tryst::params::destination::samplerLoopEnd)
    {
        loopEndNormalized = std::max(normalized, loopStartNormalized);
        return true;
    }
    if (destination == tryst::params::destination::samplerReverse)
    {
        reverse = normalized >= 0.5f;
        return true;
    }

    return false;
}

bool SamplerEngine::setAsset(std::shared_ptr<const EngineAsset> next) noexcept
{
    const auto nextSample = std::dynamic_pointer_cast<const SampleData>(std::move(next));
    if (!nextSample)
        return false;

    setSample(nextSample);
    return true;
}

void SamplerEngine::setSample(std::shared_ptr<const SampleData> next) noexcept
{
    if (!next || next->frames() < 2)
        return;

    sample = std::move(next);
    updateIncrement();
    reset();
}

float SamplerEngine::interpolate(bool rightChannel, double position) const noexcept
{
    if (!sample || sample->frames() < 2)
        return 0.0f;

    const auto maxIndex = sample->frames() - 1;
    const auto clamped = std::clamp(position, 0.0, static_cast<double>(maxIndex));
    const auto index0 = static_cast<std::size_t>(std::floor(clamped));
    const auto index1 = std::min(index0 + 1, maxIndex);
    const auto blend = static_cast<float>(clamped - static_cast<double>(index0));
    const auto a = rightChannel ? sample->sampleRight(index0) : sample->sampleLeft(index0);
    const auto b = rightChannel ? sample->sampleRight(index1) : sample->sampleLeft(index1);
    return a + (b - a) * blend;
}

double SamplerEngine::regionStartFrame() const noexcept
{
    if (!sample || sample->frames() < 2)
        return 0.0;
    return startNormalized * static_cast<double>(sample->frames() - 1);
}

double SamplerEngine::regionEndFrame() const noexcept
{
    if (!sample || sample->frames() < 2)
        return 0.0;
    return std::max(regionStartFrame() + 1.0,
                    endNormalized * static_cast<double>(sample->frames() - 1));
}

double SamplerEngine::loopStartFrame() const noexcept
{
    if (!sample || sample->frames() < 2)
        return 0.0;
    return loopStartNormalized * static_cast<double>(sample->frames() - 1);
}

double SamplerEngine::loopEndFrame() const noexcept
{
    if (!sample || sample->frames() < 2)
        return 0.0;
    return loopEndNormalized * static_cast<double>(sample->frames() - 1);
}

void SamplerEngine::updateIncrement() noexcept
{
    if (!sample || hostSampleRate <= 1.0)
    {
        increment = 1.0;
        return;
    }

    const auto semitones = playedMidiNote + detuneSemitones - sample->rootNote();
    const auto pitchRatio = std::pow(2.0, static_cast<double>(semitones) / 12.0);
    increment = (sample->sourceSampleRate() / hostSampleRate) * pitchRatio;
}
}
