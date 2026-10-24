#include "EngineFactory.h"
#include "AnalogEngine.h"
#include "WavetableEngine.h"
#include "SamplerEngine.h"
#include "GranularEngine.h"

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
            return std::make_unique<SamplerEngine>();
        case EngineKind::granular:
            return std::make_unique<GranularEngine>();
        case EngineKind::spectral:
        case EngineKind::additive:
        case EngineKind::physicalModel:
        case EngineKind::noise:
            return {};
    }

    return {};
}
