#include "StateVariableFilter.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace tryst::dsp
{
void StateVariableFilter::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate > 1.0 ? newSampleRate : 48000.0;
    updateCoefficients();
    reset();
}

void StateVariableFilter::reset() noexcept
{
    leftState = {};
    rightState = {};
}

void StateVariableFilter::setCutoffNormalized(float normalized) noexcept
{
    cutoffNormalized = std::clamp(normalized, 0.0f, 1.0f);
    updateCoefficients();
}

void StateVariableFilter::setResonanceNormalized(float normalized) noexcept
{
    resonanceNormalized = std::clamp(normalized, 0.0f, 1.0f);
    updateCoefficients();
}

void StateVariableFilter::setModeNormalized(float normalized) noexcept
{
    const auto clamped = std::clamp(normalized, 0.0f, 1.0f);
    if (clamped < 0.25f)
        mode = FilterMode::lowpass;
    else if (clamped < 0.50f)
        mode = FilterMode::bandpass;
    else if (clamped < 0.75f)
        mode = FilterMode::highpass;
    else
        mode = FilterMode::notch;
}

void StateVariableFilter::process(float& left, float& right) noexcept
{
    if (!enabled)
        return;

    left = processSample(left, leftState);
    right = processSample(right, rightState);
}

float StateVariableFilter::processSample(float input, State& state) noexcept
{
    const auto v3 = input - state.ic2eq;
    const auto v1 = (a1 * state.ic1eq) + (a2 * v3);
    const auto v2 = state.ic2eq + (a2 * state.ic1eq) + (a3 * v3);

    state.ic1eq = (2.0f * v1) - state.ic1eq;
    state.ic2eq = (2.0f * v2) - state.ic2eq;

    const auto low = v2;
    const auto band = v1;
    const auto high = input - (k * v1) - v2;

    switch (mode)
    {
        case FilterMode::lowpass: return low;
        case FilterMode::bandpass: return band;
        case FilterMode::highpass: return high;
        case FilterMode::notch: return low + high;
    }

    return input;
}

void StateVariableFilter::updateCoefficients() noexcept
{
    const auto minCutoff = 20.0f;
    const auto maxCutoff = std::min(20000.0f, static_cast<float>(sampleRate * 0.45));
    const auto ratio = std::max(1.0f, maxCutoff / minCutoff);
    const auto cutoffHz = minCutoff * std::pow(ratio, cutoffNormalized);

    const auto q = 0.5f + (resonanceNormalized * resonanceNormalized * 19.5f);
    k = 1.0f / q;
    g = std::tan(std::numbers::pi_v<float> * cutoffHz / static_cast<float>(sampleRate));
    a1 = 1.0f / (1.0f + (g * (g + k)));
    a2 = g * a1;
    a3 = g * a2;
}
}
