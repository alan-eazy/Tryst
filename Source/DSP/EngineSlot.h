#pragma once

#include "Engine.h"
#include <memory>
#include <utility>

namespace tryst::dsp
{
class EngineSlot
{
public:
    void setEngine(std::unique_ptr<Engine> next) noexcept
    {
        engine = std::move(next);
    }

    [[nodiscard]] Engine* get() noexcept { return engine.get(); }
    [[nodiscard]] const Engine* get() const noexcept { return engine.get(); }
    [[nodiscard]] bool empty() const noexcept { return engine == nullptr; }

private:
    std::unique_ptr<Engine> engine;
};
}
