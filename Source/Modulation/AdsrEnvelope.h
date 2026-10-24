#pragma once

#include <algorithm>

namespace tryst::mod
{
class AdsrEnvelope
{
public:
    struct Parameters
    {
        float attackSeconds = 0.01f;
        float decaySeconds = 0.10f;
        float sustainLevel = 0.80f;
        float releaseSeconds = 0.20f;
    };

    enum class Stage
    {
        idle,
        attack,
        decay,
        sustain,
        release
    };

    void prepare(double newSampleRate) noexcept;
    void reset() noexcept;
    void setParameters(Parameters next) noexcept;
    void noteOn() noexcept;
    void noteOff() noexcept;

    [[nodiscard]] float nextSample() noexcept;
    [[nodiscard]] bool isActive() const noexcept { return stage != Stage::idle; }
    [[nodiscard]] Stage currentStage() const noexcept { return stage; }
    [[nodiscard]] float currentValue() const noexcept { return value; }

private:
    [[nodiscard]] float secondsToSamples(float seconds) const noexcept;
    void enterAttack() noexcept;
    void enterDecay() noexcept;
    void enterRelease() noexcept;

    double sampleRate = 48000.0;
    Parameters parameters {};
    Stage stage = Stage::idle;
    float value = 0.0f;
    float delta = 0.0f;
};
}
