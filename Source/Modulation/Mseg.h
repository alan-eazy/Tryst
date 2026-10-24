#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace tryst::mod
{
struct MsegPoint
{
    float phase = 0.0f; // normalized 0..1 position inside one cycle
    float value = 0.0f; // bipolar -1..1 output
};

class Mseg
{
public:
    static constexpr std::size_t maxPoints = 16;

    void prepare(double newSampleRate) noexcept;
    void reset(float newPhase = 0.0f) noexcept;

    [[nodiscard]] bool configure(std::span<const MsegPoint> newPoints,
                                 float newRateHz,
                                 bool shouldLoop) noexcept;

    [[nodiscard]] float currentValue() const noexcept;
    void advanceSamples(std::uint32_t samples) noexcept;

    [[nodiscard]] std::size_t pointCount() const noexcept { return count; }
    [[nodiscard]] float rateHz() const noexcept { return rate; }
    [[nodiscard]] bool loops() const noexcept { return loop; }

private:
    [[nodiscard]] float valueAt(float normalizedPhase) const noexcept;

    std::array<MsegPoint, maxPoints> points {};
    std::size_t count = 0;
    double sampleRate = 48000.0;
    float rate = 1.0f;
    float phase = 0.0f;
    bool loop = true;
};
}
