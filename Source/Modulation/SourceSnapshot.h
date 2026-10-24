#pragma once

#include "ModSource.h"
#include <algorithm>
#include <array>
#include <cstddef>

namespace tryst::mod
{
class SourceSnapshot
{
public:
    static constexpr std::size_t maxIndicesPerType = 16;
    static constexpr std::size_t typeCount = static_cast<std::size_t>(SourceType::count);

    void clear() noexcept
    {
        for (auto& group : values)
            group.fill(0.0f);
    }

    [[nodiscard]] bool set(SourceId source, float value) noexcept
    {
        const auto typeIndex = static_cast<std::size_t>(source.type);
        if (typeIndex >= typeCount || source.index >= maxIndicesPerType)
            return false;

        values[typeIndex][source.index] = std::clamp(value, -1.0f, 1.0f);
        return true;
    }

    [[nodiscard]] float get(SourceId source) const noexcept
    {
        const auto typeIndex = static_cast<std::size_t>(source.type);
        if (typeIndex >= typeCount || source.index >= maxIndicesPerType)
            return 0.0f;

        return values[typeIndex][source.index];
    }

private:
    std::array<std::array<float, maxIndicesPerType>, typeCount> values {};
};
}
