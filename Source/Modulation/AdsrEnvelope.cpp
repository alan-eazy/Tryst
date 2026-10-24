#include "AdsrEnvelope.h"

namespace tryst::mod
{
void AdsrEnvelope::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate > 1.0 ? newSampleRate : 48000.0;
    reset();
}

void AdsrEnvelope::reset() noexcept
{
    stage = Stage::idle;
    value = 0.0f;
    delta = 0.0f;
}

void AdsrEnvelope::setParameters(Parameters next) noexcept
{
    next.attackSeconds = std::max(0.0f, next.attackSeconds);
    next.decaySeconds = std::max(0.0f, next.decaySeconds);
    next.sustainLevel = std::clamp(next.sustainLevel, 0.0f, 1.0f);
    next.releaseSeconds = std::max(0.0f, next.releaseSeconds);
    parameters = next;
}

void AdsrEnvelope::noteOn() noexcept
{
    enterAttack();
}

void AdsrEnvelope::noteOff() noexcept
{
    if (stage != Stage::idle)
        enterRelease();
}

float AdsrEnvelope::nextSample() noexcept
{
    switch (stage)
    {
        case Stage::idle:
            return 0.0f;

        case Stage::attack:
            value += delta;
            if (value >= 1.0f)
            {
                value = 1.0f;
                enterDecay();
            }
            break;

        case Stage::decay:
            value += delta;
            if (value <= parameters.sustainLevel)
            {
                value = parameters.sustainLevel;
                stage = Stage::sustain;
                delta = 0.0f;
            }
            break;

        case Stage::sustain:
            value = parameters.sustainLevel;
            break;

        case Stage::release:
            value += delta;
            if (value <= 0.0f)
            {
                value = 0.0f;
                stage = Stage::idle;
                delta = 0.0f;
            }
            break;
    }

    return value;
}

float AdsrEnvelope::secondsToSamples(float seconds) const noexcept
{
    return static_cast<float>(sampleRate) * std::max(0.0f, seconds);
}

void AdsrEnvelope::enterAttack() noexcept
{
    stage = Stage::attack;
    const auto samples = secondsToSamples(parameters.attackSeconds);
    if (samples <= 1.0f)
    {
        value = 1.0f;
        enterDecay();
        return;
    }

    delta = (1.0f - value) / samples;
}

void AdsrEnvelope::enterDecay() noexcept
{
    stage = Stage::decay;
    const auto samples = secondsToSamples(parameters.decaySeconds);
    if (samples <= 1.0f)
    {
        value = parameters.sustainLevel;
        stage = Stage::sustain;
        delta = 0.0f;
        return;
    }

    delta = (parameters.sustainLevel - value) / samples;
}

void AdsrEnvelope::enterRelease() noexcept
{
    stage = Stage::release;
    const auto samples = secondsToSamples(parameters.releaseSeconds);
    if (samples <= 1.0f)
    {
        reset();
        return;
    }

    delta = -value / samples;
}
}
