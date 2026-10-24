#include "DSP/GranularEngine.h"
#include "DSP/SampleData.h"
#include "DSP/VoiceManager.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
void testGranularPositionSelection()
{
    std::vector<float> mono(4096, 0.8f);
    for (std::size_t i = mono.size() / 2; i < mono.size(); ++i)
        mono[i] = -0.8f;

    auto sample = tryst::dsp::SampleData::fromMono(std::move(mono), 48000.0, 60.0f);
    assert(sample != nullptr);

    tryst::dsp::GranularEngine engine;
    engine.prepare(48000.0, 512);
    assert(engine.setAsset(sample));
    assert(engine.setParameter(tryst::params::destination::generatorLevel, 1.0f));
    assert(engine.setParameter(tryst::params::destination::granularSize, 0.10f));
    assert(engine.setParameter(tryst::params::destination::granularDensity, 0.20f));
    assert(engine.setParameter(tryst::params::destination::granularSpray, 0.0f));
    assert(engine.setParameter(tryst::params::destination::granularPitch, 0.5f));
    assert(engine.setParameter(tryst::params::destination::granularPosition, 0.20f));
    engine.noteOn({ 60, 1.0f });

    std::array<float, 512> leftA {};
    std::array<float, 512> rightA {};
    tryst::dsp::ProcessContext contextA { 48000.0, leftA, rightA };
    engine.render(contextA);

    float sumA = 0.0f;
    for (const auto value : leftA)
        sumA += value;
    assert(sumA > 1.0f);

    engine.reset();
    assert(engine.setParameter(tryst::params::destination::granularPosition, 0.80f));
    engine.noteOn({ 60, 1.0f });

    std::array<float, 512> leftB {};
    std::array<float, 512> rightB {};
    tryst::dsp::ProcessContext contextB { 48000.0, leftB, rightB };
    engine.render(contextB);

    float sumB = 0.0f;
    for (const auto value : leftB)
        sumB += value;
    assert(sumB < -1.0f);
}

void testGranularInVoiceManager()
{
    std::vector<float> mono(4096);
    for (std::size_t i = 0; i < mono.size(); ++i)
        mono[i] = std::sin(static_cast<float>(i) * 0.025f);

    auto sample = tryst::dsp::SampleData::fromMono(std::move(mono), 48000.0, 60.0f);
    assert(sample != nullptr);

    tryst::dsp::VoiceManager manager;
    manager.prepare(48000.0, 512);
    manager.setAmpEnvelope({ 0.0f, 0.0f, 1.0f, 0.0f });
    assert(manager.setEngineKind(1, tryst::dsp::EngineKind::granular));
    assert(manager.setEngineAsset(1, sample));
    assert(manager.setEngineParameter(1, tryst::params::destination::generatorLevel, 0.7f));
    assert(manager.setEngineParameter(1, tryst::params::destination::granularPosition, 0.4f));
    assert(manager.setEngineParameter(1, tryst::params::destination::granularSize, 0.15f));
    assert(manager.setEngineParameter(1, tryst::params::destination::granularDensity, 0.65f));
    manager.noteOn({ 60, 1.0f });

    std::array<float, 512> left {};
    std::array<float, 512> right {};
    tryst::dsp::ProcessContext context { 48000.0, left, right };
    manager.render(context);

    assert(std::any_of(left.begin(), left.end(), [](float value) { return std::abs(value) > 0.001f; }));
}
}

int main()
{
    testGranularPositionSelection();
    testGranularInVoiceManager();
    std::cout << "Tryst granular tests passed\n";
    return 0;
}
