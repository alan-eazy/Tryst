#pragma once

#include "Lfo.h"
#include "ModMatrix.h"
#include "TargetValueStore.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace tryst::mod
{
class FastModulationRuntime
{
public:
    static constexpr std::size_t lfoCount = 4;
    static constexpr std::size_t maxTargets = 64;
    static constexpr std::uint32_t preferredChunkSize = 16;

    void prepare(double sampleRate) noexcept;
    void resetPhases() noexcept;

    [[nodiscard]] bool configureLfo(std::size_t index, float rateHz, LfoWaveform waveform) noexcept;
    [[nodiscard]] bool addRoute(Route route) noexcept;
    void clearRoutes() noexcept;
    [[nodiscard]] bool setBase(TargetId target, float normalized) noexcept;

    template <typename ApplyFn>
    void applyCurrent(ApplyFn&& apply) noexcept
    {
        refreshSources();
        for (std::size_t i = 0; i < targetCount; ++i)
        {
            const auto target = targets[i];
            const auto* base = baseValues.find(target);
            if (base == nullptr)
                continue;
            apply(target, matrix.apply(target, base->baseValue, sources));
        }
    }

    void advance(std::uint32_t samples) noexcept;

    [[nodiscard]] std::size_t routeCount() const noexcept { return matrix.routeCount(); }
    [[nodiscard]] std::size_t trackedTargetCount() const noexcept { return targetCount; }

private:
    void refreshSources() noexcept;
    [[nodiscard]] bool trackTarget(TargetId target) noexcept;
    [[nodiscard]] bool isTracked(TargetId target) const noexcept;

    ModMatrix matrix;
    SourceSnapshot sources;
    TargetValueStore baseValues;
    std::array<Lfo, lfoCount> lfos;
    std::array<TargetId, maxTargets> targets {};
    std::size_t targetCount = 0;
};
}
