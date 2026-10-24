#include "ModulationRuntime.h"
#include <algorithm>

namespace tryst::mod
{
void ModulationRuntime::reset() noexcept
{
    matrix.clear();
    sources.clear();
}

bool ModulationRuntime::addRoute(Route route) noexcept
{
    return matrix.addRoute(route);
}

void ModulationRuntime::clearRoutes() noexcept
{
    matrix.clear();
}

bool ModulationRuntime::setMacro(std::size_t index, float normalized) noexcept
{
    if (index >= macroCount)
        return false;

    return sources.set({ SourceType::macro, static_cast<std::uint16_t>(index) },
                       std::clamp(normalized, 0.0f, 1.0f));
}

void ModulationRuntime::setMorphX(float normalized) noexcept
{
    (void) sources.set({ SourceType::morphX, 0 }, std::clamp(normalized, 0.0f, 1.0f));
}

void ModulationRuntime::setMorphY(float normalized) noexcept
{
    (void) sources.set({ SourceType::morphY, 0 }, std::clamp(normalized, 0.0f, 1.0f));
}

bool ModulationRuntime::setSource(SourceId source, float value) noexcept
{
    return sources.set(source, value);
}

float ModulationRuntime::evaluate(TargetId target, float baseValue) const noexcept
{
    return matrix.apply(target, baseValue, sources);
}
}
