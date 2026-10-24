#include "DSP/VoiceManager.h"
#include "Modulation/FastModulationRuntime.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
void testFastRuntimeLfoStepping()
{
    tryst::mod::FastModulationRuntime runtime;
    runtime.prepare(1000.0);
    assert(runtime.configureLfo(0, 10.0f, tryst::mod::LfoWaveform::square));

    const tryst::mod::TargetId target {
        tryst::mod::TargetDomain::filter, 0, tryst::params::destination::filter1Cutoff
    };
    assert(runtime.addRoute({ { tryst::mod::SourceType::lfo, 0 }, target, 0.25f, 0.0f, true }));
    assert(runtime.setBase(target, 0.5f));

    float first = 0.0f;
    runtime.applyCurrent([&first](tryst::mod::TargetId, float value) noexcept { first = value; });
    assert(std::abs(first - 0.75f) < 0.0001f);

    runtime.advance(50);
    float second = 0.0f;
    runtime.applyCurrent([&second](tryst::mod::TargetId, float value) noexcept { second = value; });
    assert(std::abs(second - 0.25f) < 0.0001f);
}

void testFastModulationInVoiceManager()
{
    tryst::dsp::VoiceManager manager;
    manager.prepare(1000.0, 32);
    manager.setAmpEnvelope({ 0.0f, 0.0f, 1.0f, 0.0f });
    assert(manager.setEngineParameter(0, tryst::params::destination::generatorLevel, 0.5f));
    assert(manager.configureVoiceLfo(0, 31.25f, tryst::mod::LfoWaveform::square));
    assert(manager.addFastModulationRoute({
        { tryst::mod::SourceType::lfo, 0 },
        { tryst::mod::TargetDomain::generator, 0, tryst::params::destination::generatorLevel },
        0.5f, 0.0f, true
    }));
    assert(manager.fastModulationRouteCount() == 1);

    assert(!manager.addFastModulationRoute({
        { tryst::mod::SourceType::lfo, 0 },
        { tryst::mod::TargetDomain::filter, 0, tryst::params::destination::filter1Enabled },
        1.0f, 0.0f, true
    }));

    manager.noteOn({ 60, 1.0f });
    std::array<float, 32> left {};
    std::array<float, 32> right {};
    tryst::dsp::ProcessContext context { 1000.0, left, right };
    manager.render(context);

    float firstChunkEnergy = 0.0f;
    float secondChunkEnergy = 0.0f;
    for (std::size_t i = 0; i < 16; ++i)
        firstChunkEnergy += std::abs(left[i]);
    for (std::size_t i = 16; i < 32; ++i)
        secondChunkEnergy += std::abs(left[i]);

    assert(firstChunkEnergy > 0.1f);
    assert(secondChunkEnergy < firstChunkEnergy * 0.05f);

    manager.clearFastModulationRoutes();
    assert(manager.fastModulationRouteCount() == 0);
}
}

int main()
{
    testFastRuntimeLfoStepping();
    testFastModulationInVoiceManager();
    std::cout << "Tryst fast modulation tests passed\n";
    return 0;
}
