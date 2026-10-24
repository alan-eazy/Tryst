#include "Core/ParameterRegistry.h"
#include "DSP/FilterChain.h"
#include "DSP/VoiceManager.h"
#include "DSP/WavetableEngine.h"
#include "Modulation/AdsrEnvelope.h"
#include "Modulation/Lfo.h"
#include "Modulation/ModMatrix.h"
#include "Modulation/ModulationRuntime.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
void testModMatrix()
{
    tryst::mod::ModMatrix matrix;
    tryst::mod::SourceSnapshot sources;
    const tryst::mod::SourceId lfo { tryst::mod::SourceType::lfo, 0 };
    const tryst::mod::TargetId target { tryst::mod::TargetDomain::filter, 0, tryst::params::destination::filter1Cutoff };
    assert(sources.set(lfo, 1.0f));
    assert(matrix.addRoute({ lfo, target, 0.25f, 0.0f, true }));
    const auto value = matrix.apply(target, 0.5f, sources);
    assert(std::abs(value - 0.75f) < 0.0001f);
}

void testModulationRuntime()
{
    tryst::mod::ModulationRuntime runtime;
    const tryst::mod::TargetId shape { tryst::mod::TargetDomain::generator, 0, tryst::params::destination::analogShape };
    const tryst::mod::TargetId cutoff { tryst::mod::TargetDomain::filter, 0, tryst::params::destination::filter1Cutoff };

    assert(runtime.addRoute({ { tryst::mod::SourceType::macro, 0 }, shape, 0.5f, 0.0f, true }));
    assert(runtime.addRoute({ { tryst::mod::SourceType::morphX, 0 }, cutoff, -0.5f, 0.0f, true }));
    assert(runtime.setMacro(0, 1.0f));
    runtime.setMorphX(1.0f);

    assert(std::abs(runtime.evaluate(shape, 0.25f) - 0.75f) < 0.0001f);
    assert(std::abs(runtime.evaluate(cutoff, 0.8f) - 0.3f) < 0.0001f);
}

void testAdsr()
{
    tryst::mod::AdsrEnvelope env;
    env.prepare(1000.0);
    env.setParameters({ 0.01f, 0.01f, 0.5f, 0.01f });
    env.noteOn();

    for (int i = 0; i < 30; ++i)
        (void) env.nextSample();

    assert(env.currentStage() == tryst::mod::AdsrEnvelope::Stage::sustain);
    assert(std::abs(env.currentValue() - 0.5f) < 0.01f);

    env.noteOff();
    for (int i = 0; i < 20; ++i)
        (void) env.nextSample();

    assert(!env.isActive());
    assert(env.currentValue() == 0.0f);
}

void testLfo()
{
    tryst::mod::Lfo lfo;
    lfo.prepare(1000.0);
    lfo.setRateHz(10.0f);
    lfo.setWaveform(tryst::mod::LfoWaveform::sine);

    float minimum = 1.0f;
    float maximum = -1.0f;
    for (int i = 0; i < 100; ++i)
    {
        const auto value = lfo.nextSample();
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
        assert(value >= -1.0001f && value <= 1.0001f);
    }

    assert(minimum < -0.9f);
    assert(maximum > 0.9f);
}

void testParameterRegistry()
{
    const auto all = tryst::params::ParameterRegistry::all();
    assert(all.size() >= 14);

    const auto* position = tryst::params::ParameterRegistry::find("engine.wavetable.position");
    assert(position != nullptr);
    assert(position->destination == tryst::params::destination::wavetablePosition);

    const auto* level = tryst::params::ParameterRegistry::find(tryst::params::destination::generatorLevel);
    assert(level != nullptr);
    assert(level->id == "generator.level");

    const auto* cutoff = tryst::params::ParameterRegistry::find("filter.1.cutoff");
    assert(cutoff != nullptr);
    assert(cutoff->destination == tryst::params::destination::filter1Cutoff);
}

void testWavetableEngine()
{
    tryst::dsp::WavetableEngine engine;
    engine.prepare(48000.0, 256);
    assert(engine.setParameter(tryst::params::destination::generatorLevel, 1.0f));
    assert(engine.setParameter(tryst::params::destination::wavetablePosition, 0.0f));
    engine.noteOn({ 60, 1.0f });

    std::array<float, 256> sineLeft {};
    std::array<float, 256> sineRight {};
    tryst::dsp::ProcessContext sineContext { 48000.0, sineLeft, sineRight };
    engine.render(sineContext);

    engine.reset();
    assert(engine.setParameter(tryst::params::destination::wavetablePosition, 1.0f));
    engine.noteOn({ 60, 1.0f });

    std::array<float, 256> squareLeft {};
    std::array<float, 256> squareRight {};
    tryst::dsp::ProcessContext squareContext { 48000.0, squareLeft, squareRight };
    engine.render(squareContext);

    float difference = 0.0f;
    for (std::size_t i = 0; i < sineLeft.size(); ++i)
        difference += std::abs(sineLeft[i] - squareLeft[i]);

    assert(difference > 1.0f);
}

void testWavetableInVoiceManager()
{
    tryst::dsp::VoiceManager manager;
    manager.prepare(48000.0, 128);
    assert(manager.setEngineKind(1, tryst::dsp::EngineKind::wavetable));
    assert(manager.setEngineParameter(1, tryst::params::destination::wavetablePosition, 0.65f));
    assert(manager.setEngineParameter(1, tryst::params::destination::generatorLevel, 0.25f));

    manager.noteOn({ 67, 0.9f });
    std::array<float, 128> left {};
    std::array<float, 128> right {};
    tryst::dsp::ProcessContext context { 48000.0, left, right };
    manager.render(context);

    const auto nonZero = std::any_of(left.begin(), left.end(), [](float sample) { return std::abs(sample) > 0.0001f; });
    assert(nonZero);
}

void testDualFilterChain()
{
    tryst::dsp::FilterChain filters;
    filters.prepare(48000.0);
    assert(filters.setParameter(tryst::params::destination::filter1Enabled, 1.0f));
    assert(filters.setParameter(tryst::params::destination::filter1Cutoff, 0.0f));
    assert(filters.setParameter(tryst::params::destination::filter1Resonance, 0.0f));
    assert(filters.setParameter(tryst::params::destination::filter1Mode, 0.0f));

    std::array<float, 512> left {};
    std::array<float, 512> right {};
    for (std::size_t i = 0; i < left.size(); ++i)
    {
        const auto value = (i & 1U) == 0U ? 1.0f : -1.0f;
        left[i] = value;
        right[i] = value;
    }

    filters.process(left, right);

    float tailEnergy = 0.0f;
    for (std::size_t i = 256; i < left.size(); ++i)
        tailEnergy += std::abs(left[i]);

    assert(tailEnergy < 5.0f);

    filters.reset();
    assert(filters.setParameter(tryst::params::destination::filter2Enabled, 1.0f));
    assert(filters.setParameter(tryst::params::destination::filter2Cutoff, 0.75f));
    assert(filters.setParameter(tryst::params::destination::filter2Mode, 0.65f));
    assert(filters.setParameter(tryst::params::destination::filterRouting, 1.0f));
    assert(filters.setParameter(tryst::params::destination::filterParallelBlend, 0.5f));

    left.fill(0.25f);
    right.fill(0.25f);
    filters.process(left, right);
    assert(std::all_of(left.begin(), left.end(), [](float sample) { return std::isfinite(sample); }));
}

void testFiltersInVoiceManager()
{
    tryst::dsp::VoiceManager dry;
    dry.prepare(48000.0, 256);
    dry.setAmpEnvelope({ 0.0f, 0.0f, 1.0f, 0.0f });
    dry.noteOn({ 84, 1.0f });

    tryst::dsp::VoiceManager filtered;
    filtered.prepare(48000.0, 256);
    filtered.setAmpEnvelope({ 0.0f, 0.0f, 1.0f, 0.0f });
    assert(filtered.setFilterParameter(tryst::params::destination::filter1Enabled, 1.0f));
    assert(filtered.setFilterParameter(tryst::params::destination::filter1Cutoff, 0.15f));
    assert(filtered.setFilterParameter(tryst::params::destination::filter1Mode, 0.0f));
    filtered.noteOn({ 84, 1.0f });

    std::array<float, 256> dryLeft {};
    std::array<float, 256> dryRight {};
    std::array<float, 256> filteredLeft {};
    std::array<float, 256> filteredRight {};
    tryst::dsp::ProcessContext dryContext { 48000.0, dryLeft, dryRight };
    tryst::dsp::ProcessContext filteredContext { 48000.0, filteredLeft, filteredRight };
    dry.render(dryContext);
    filtered.render(filteredContext);

    float dryEnergy = 0.0f;
    float filteredEnergy = 0.0f;
    for (std::size_t i = 0; i < dryLeft.size(); ++i)
    {
        dryEnergy += std::abs(dryLeft[i]);
        filteredEnergy += std::abs(filteredLeft[i]);
    }

    assert(filteredEnergy < dryEnergy * 0.5f);
}

void testMacroAndXYRoutingInVoiceManager()
{
    tryst::dsp::VoiceManager manager;
    manager.prepare(48000.0, 256);
    manager.setAmpEnvelope({ 0.0f, 0.0f, 1.0f, 0.0f });

    assert(manager.setEngineParameter(0, tryst::params::destination::analogShape, 0.0f));
    assert(manager.setFilterParameter(tryst::params::destination::filter1Enabled, 1.0f));
    assert(manager.setFilterParameter(tryst::params::destination::filter1Cutoff, 0.8f));

    assert(manager.addModulationRoute({
        { tryst::mod::SourceType::macro, 0 },
        { tryst::mod::TargetDomain::generator, 0, tryst::params::destination::analogShape },
        1.0f, 0.0f, true
    }));
    assert(manager.addModulationRoute({
        { tryst::mod::SourceType::morphX, 0 },
        { tryst::mod::TargetDomain::filter, 0, tryst::params::destination::filter1Cutoff },
        -0.7f, 0.0f, true
    }));
    assert(manager.modulationRouteCount() == 2);

    assert(manager.setMacro(0, 1.0f));
    manager.setMorphX(1.0f);
    manager.noteOn({ 72, 1.0f });

    std::array<float, 256> left {};
    std::array<float, 256> right {};
    tryst::dsp::ProcessContext context { 48000.0, left, right };
    manager.render(context);

    const auto nonZero = std::any_of(left.begin(), left.end(), [](float sample) { return std::abs(sample) > 0.0001f; });
    assert(nonZero);

    manager.clearModulationRoutes();
    assert(manager.modulationRouteCount() == 0);
}

void testPolyphonicVoiceManager()
{
    tryst::dsp::VoiceManager manager;
    manager.prepare(48000.0, 256);
    manager.setAmpEnvelope({ 0.0f, 0.0f, 1.0f, 0.005f });

    manager.noteOn({ 60, 1.0f });
    manager.noteOn({ 64, 0.8f });
    assert(manager.activeVoiceCount() == 2);

    std::array<float, 256> left {};
    std::array<float, 256> right {};
    tryst::dsp::ProcessContext context { 48000.0, left, right };
    manager.render(context);

    const auto peak = *std::max_element(left.begin(), left.end(), [](float a, float b) {
        return std::abs(a) < std::abs(b);
    });
    assert(std::abs(peak) > 0.001f);

    manager.noteOff(60);
    manager.noteOff(64);
    for (int block = 0; block < 4; ++block)
    {
        left.fill(0.0f);
        right.fill(0.0f);
        manager.render(context);
    }

    assert(manager.activeVoiceCount() == 0);
}
}

int main()
{
    testParameterRegistry();
    testModMatrix();
    testModulationRuntime();
    testAdsr();
    testLfo();
    testWavetableEngine();
    testWavetableInVoiceManager();
    testDualFilterChain();
    testFiltersInVoiceManager();
    testMacroAndXYRoutingInVoiceManager();
    testPolyphonicVoiceManager();
    std::cout << "Tryst core tests passed\n";
    return 0;
}
