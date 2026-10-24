#pragma once

#include "ModSource.h"
#include "ModTarget.h"
#include "SourceSnapshot.h"
#include <algorithm>
#include <array>
#include <cstddef>

namespace tryst::mod
{
struct Route
{
    SourceId source;
    TargetId target;
    float depth = 0.0f;   // bipolar modulation amount: -1..+1
    float curve = 0.0f;   // reserved for response shaping
    bool enabled = true;
};

template <std::size_t MaxRoutes>
class RouteStore
{
public:
    [[nodiscard]] bool add(Route route) noexcept
    {
        if (count >= MaxRoutes)
            return false;
        routes[count++] = route;
        return true;
    }

    void clear() noexcept { count = 0; }
    [[nodiscard]] std::size_t size() const noexcept { return count; }
    [[nodiscard]] const Route& operator[](std::size_t i) const noexcept { return routes[i]; }

private:
    std::array<Route, MaxRoutes> routes {};
    std::size_t count = 0;
};

class ModMatrix
{
public:
    static constexpr std::size_t maxRoutes = 256;

    [[nodiscard]] bool addRoute(Route route) noexcept;
    void clear() noexcept;

    // Base values are normalized 0..1. Sources may be bipolar (-1..1) or unipolar (0..1).
    [[nodiscard]] float apply(TargetId target,
                              float baseValue,
                              const SourceSnapshot& sources) const noexcept;

    [[nodiscard]] std::size_t routeCount() const noexcept { return routes.size(); }

private:
    RouteStore<maxRoutes> routes;
};
}
