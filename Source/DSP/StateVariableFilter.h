#pragma once

#include <cstdint>

namespace tryst::dsp
{
enum class FilterMode : std::uint8_t
{
    lowpass,
    bandpass,
    highpass,
    notch
};

class StateVariableFilter
{
public:
    void prepare(double newSampleRate) noexcept;
    void reset() noexcept;

    void setEnabled(bool shouldBeEnabled) noexcept { enabled = shouldBeEnabled; }
    void setCutoffNormalized(float normalized) noexcept;
    void setResonanceNormalized(float normalized) noexcept;
    void setModeNormalized(float normalized) noexcept;

    [[nodiscard]] bool isEnabled() const noexcept { return enabled; }
    void process(float& left, float& right) noexcept;

private:
    struct State
    {
        float ic1eq = 0.0f;
        float ic2eq = 0.0f;
    };

    [[nodiscard]] float processSample(float input, State& state) noexcept;
    void updateCoefficients() noexcept;

    double sampleRate = 48000.0;
    float cutoffNormalized = 1.0f;
    float resonanceNormalized = 0.0f;
    float g = 1.0f;
    float k = 2.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float a3 = 0.0f;
    FilterMode mode = FilterMode::lowpass;
    State leftState;
    State rightState;
    bool enabled = false;
};
}
