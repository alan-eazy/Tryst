#include "ModMatrix.h"

namespace tryst::mod
{
bool ModMatrix::addRoute(Route route) noexcept
{
    route.depth = std::clamp(route.depth, -1.0f, 1.0f);
    return routes.add(route);
}

void ModMatrix::clear() noexcept
{
    routes.clear();
}

float ModMatrix::apply(TargetId target,
                       float baseValue,
                       const SourceSnapshot& sources) const noexcept
{
    float value = baseValue;

    for (std::size_t i = 0; i < routes.size(); ++i)
    {
        const auto& route = routes[i];
        if (!route.enabled || !(route.target == target))
            continue;

        value += sources.get(route.source) * route.depth;
    }

    return std::clamp(value, 0.0f, 1.0f);
}
}
