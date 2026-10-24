#include "NoiseEngine.h"
#include <algorithm>
#include <cmath>

namespace tryst::dsp
{
void NoiseEngine::prepare(double newSampleRate, std::uint32_t)
{
    sampleRate = newSampleRate > 1.0 ? newSampleRate : 48000.0;
    reset();
}

void NoiseEngine::reset() noexcept
{
    leftState = {};
    rightState = {};
    leftState.random = 0x9e3779b9u;
    rightState.random = 0x243f6a88u;
    holdCounter = 0;
    velocity = 0.0f;
    active = false;
}

void NoiseEngine::noteOn(const NoteEvent& note) noexcept
{
    const auto seed = static_cast<std::uint32_t>(note.note + 1) * 0x85ebca6bu;
    leftState.random = 0x9e3779b9u ^ seed;
    rightState.random = 0x243f6a88u ^ (seed * 0xc2b2ae35u);
    if (leftState.random == 0) leftState.random = 1;
    if (rightState.random == 0) rightState.random = 1;
    std::fill(leftState.pink.begin(), leftState.pink.end(), 0.0f);
    std::fill(rightState.pink.begin(), rightState.pink.end(), 0.0f);
    leftState.brown = rightState.brown = 0.0f;
    leftState.held = rightState.held = 0.0f;
    holdCounter = 0;
    velocity = std::clamp(note.velocity, 0.0f, 1.0f);
    active = true;
}

void NoiseEngine::noteOff(std::int32_t, float) noexcept
{
    // The voice amplitude envelope owns release timing.
}

void NoiseEngine::render(ProcessContext& context) noexcept
{
    if (!active)
        return;

    const auto samples = std::min(context.left.size(), context.right.size());
    const auto gain = velocity * level;
    const auto width = std::clamp(stereoWidth, 0.0f, 1.0f);
    const auto textureMix = std::clamp(texture, 0.0f, 1.0f);

    for (std::size_t i = 0; i < samples; ++i)
    {
        const auto rawLeft = colored(leftState);
        const auto rawRight = colored(rightState);

        if (holdCounter == 0)
        {
            leftState.held = rawLeft;
            rightState.held = rawRight;
            holdCounter = holdLengthSamples();
        }
        --holdCounter;

        const auto texturedLeft = rawLeft + (leftState.held - rawLeft) * textureMix;
        const auto texturedRight = rawRight + (rightState.held - rawRight) * textureMix;
        const auto outputRight = texturedLeft + (texturedRight - texturedLeft) * width;

        context.left[i] += texturedLeft * gain;
        context.right[i] += outputRight * gain;
    }
}

bool NoiseEngine::setParameter(tryst::params::DestinationId destination, float normalized) noexcept
{
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    if (destination == tryst::params::destination::generatorLevel)
    {
        level = normalized;
        return true;
    }
    if (destination == tryst::params::destination::generatorTune)
    {
        tuneSemitones = (normalized - 0.5f) * 48.0f;
        return true;
    }
    if (destination == tryst::params::destination::noiseColor)
    {
        color = normalized;
        return true;
    }
    if (destination == tryst::params::destination::noiseStereo)
    {
        stereoWidth = normalized;
        return true;
    }
    if (destination == tryst::params::destination::noiseTexture)
    {
        texture = normalized;
        return true;
    }
    return false;
}

float NoiseEngine::nextWhite(ChannelState& state) noexcept
{
    auto x = state.random;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state.random = x == 0 ? 1u : x;
    const auto unit = static_cast<float>(state.random) / static_cast<float>(0xffffffffu);
    return unit * 2.0f - 1.0f;
}

float NoiseEngine::nextPink(ChannelState& state, float white) noexcept
{
    state.pink[0] = 0.99886f * state.pink[0] + white * 0.0555179f;
    state.pink[1] = 0.99332f * state.pink[1] + white * 0.0750759f;
    state.pink[2] = 0.96900f * state.pink[2] + white * 0.1538520f;
    state.pink[3] = 0.86650f * state.pink[3] + white * 0.3104856f;
    state.pink[4] = 0.55000f * state.pink[4] + white * 0.5329522f;
    state.pink[5] = -0.7616f * state.pink[5] - white * 0.0168980f;
    const auto pink = state.pink[0] + state.pink[1] + state.pink[2] + state.pink[3]
                    + state.pink[4] + state.pink[5] + state.pink[6] + white * 0.5362f;
    state.pink[6] = white * 0.115926f;
    return std::clamp(pink * 0.11f, -1.0f, 1.0f);
}

float NoiseEngine::nextBrown(ChannelState& state, float white) noexcept
{
    state.brown = std::clamp((state.brown + white * 0.025f) * 0.999f, -1.0f, 1.0f);
    return state.brown;
}

float NoiseEngine::colored(ChannelState& state) const noexcept
{
    const auto white = nextWhite(state);
    const auto pink = nextPink(state, white);
    const auto brown = nextBrown(state, white);
    if (color <= 0.5f)
    {
        const auto mix = color * 2.0f;
        return white + (pink - white) * mix;
    }
    const auto mix = (color - 0.5f) * 2.0f;
    return pink + (brown - pink) * mix;
}

std::uint32_t NoiseEngine::holdLengthSamples() const noexcept
{
    const auto pitchRatio = std::pow(2.0, static_cast<double>(tuneSemitones) / 12.0);
    const auto maxHold = std::max(1.0, sampleRate * 0.05 / pitchRatio);
    const auto shaped = static_cast<double>(texture) * static_cast<double>(texture);
    return static_cast<std::uint32_t>(std::max(1.0, 1.0 + shaped * maxHold));
}
}
