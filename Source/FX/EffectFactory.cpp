#include "EffectFactory.h"
#include "DriveEffect.h"
#include "DelayEffect.h"

namespace tryst::fx
{
std::unique_ptr<Effect> createEffect(EffectKind kind)
{
    switch (kind)
    {
        case EffectKind::drive: return std::make_unique<DriveEffect>();
        case EffectKind::delay: return std::make_unique<DelayEffect>();
    }
    return {};
}
}
