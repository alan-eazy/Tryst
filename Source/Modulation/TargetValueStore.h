#pragma once

#include "ModTarget.h"
#include <algorithm>
#include <array>
#include <cstddef>

namespace tryst::mod
{
class TargetValueStore
{
public:
    struct Entry
    {
        TargetId target;
        float baseValue = 0.0f;
    };

    static constexpr std::size_t capacity = 64;

    [[nodiscard]] bool set(TargetId target, float baseValue) noexcept
    {
        baseValue = std::clamp(baseValue, 0.0f, 1.0f);
        for (std::size_t i = 0; i < count; ++i)
        {
            if (values[i].target == target)
            {
                values[i].baseValue = baseValue;
                return true;
            }
        }

        if (count >= capacity)
            return false;

        values[count++] = { target, baseValue };
        return true;
    }

    [[nodiscard]] const Entry* find(TargetId target) const noexcept
    {
        for (std::size_t i = 0; i < count; ++i)
            if (values[i].target == target)
                return &values[i];
        return nullptr;
    }

    void clear() noexcept { count = 0; }
    [[nodiscard]] std::size_t size() const noexcept { return count; }
    [[nodiscard]] const Entry& operator[](std::size_t index) const noexcept { return values[index]; }

private:
    std::array<Entry, capacity> values {};
    std::size_t count = 0;
};
}
