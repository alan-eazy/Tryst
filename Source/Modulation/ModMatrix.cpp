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

float ModMatrix::apply(std::uint32_t destination,
                       float baseValue,
                       SourceId source,
                       float sourceValue) const noexcept
{
    float value = baseValue;
    const float src = std::clamp(sourceValue, -1.0f, 1.0f);

    for (std::size_t i = 0; i < routes.size(); ++i)
    {
        const auto& route = routes[i];
        if (!route.enabled || route.destination != destination || !(route.source == source))
            continue;

        value += src * route.depth;
    }

    return std::clamp(value, 0.0f, 1.0f);
}
}
