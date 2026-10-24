#pragma once

#include "Engine.h"
#include <memory>

namespace tryst::dsp
{
[[nodiscard]] std::unique_ptr<Engine> createEngine(EngineKind kind);
}
