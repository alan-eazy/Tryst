#include "EngineFactory.h"
#include "AnalogEngine.h"
#include "WavetableEngine.h"

namespace tryst::dsp
{
std::unique_ptr<Engine> createEngine(EngineKind kind)
{
    switch (kind)
    {
        case EngineKind::analog:
            return std::make_unique<AnalogEngine>();
        case EngineKind::wavetable:
            return std::make_unique<WavetableEngine>();
        case EngineKind::sampler:
        case EngineKind::granular:
        case EngineKind::spectral:
        case EngineKind::additive:
        case EngineKind::physicalModel:
        case EngineKind::noise:
            return {};
    }

    return {};
}
}
