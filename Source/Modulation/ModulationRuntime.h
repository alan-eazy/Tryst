#pragma once

#include "ModMatrix.h"
#include <cstddef>

namespace tryst::mod
{
class ModulationRuntime
{
public:
    static constexpr std::size_t macroCount = 8;

    void reset() noexcept;

    [[nodiscard]] bool addRoute(Route route) noexcept;
    void clearRoutes() noexcept;

    [[nodiscard]] bool setMacro(std::size_t index, float normalized) noexcept;
    void setMorphX(float normalized) noexcept;
    void setMorphY(float normalized) noexcept;
    [[nodiscard]] bool setSource(SourceId source, float value) noexcept;

    [[nodiscard]] float evaluate(TargetId target, float baseValue) const noexcept;
    [[nodiscard]] std::size_t routeCount() const noexcept { return matrix.routeCount(); }

private:
    ModMatrix matrix;
    SourceSnapshot sources;
};
}
