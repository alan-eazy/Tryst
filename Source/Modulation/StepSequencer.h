#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace tryst::mod
{
class StepSequencer
{
public:
    static constexpr std::size_t maxSteps = 16;

    void prepare(double newSampleRate) noexcept;
    void reset(float newPhase = 0.0f) noexcept;

    [[nodiscard]] bool configure(std::span<const float> newValues,
                                 float newRateHz,
                                 bool shouldLoop) noexcept;

    [[nodiscard]] float currentValue() const noexcept;
    void advanceSamples(std::uint32_t samples) noexcept;

    [[nodiscard]] std::size_t stepCount() const noexcept { return count; }
    [[nodiscard]] float rateHz() const noexcept { return rate; }
    [[nodiscard]] bool loops() const noexcept { return loop; }

private:
    std::array<float, maxSteps> values {};
    std::size_t count = 0;
    double sampleRate = 48000.0;
    float rate = 1.0f; // complete sequence cycles per second
    float phase = 0.0f;
    bool loop = true;
};
}
