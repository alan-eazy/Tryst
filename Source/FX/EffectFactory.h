#pragma once
#include "Effect.h"
#include <memory>

namespace tryst::fx
{
[[nodiscard]] std::unique_ptr<Effect> createEffect(EffectKind kind);
}
