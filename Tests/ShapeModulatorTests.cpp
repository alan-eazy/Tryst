#include "DSP/VoiceManager.h"
#include "Modulation/Mseg.h"
#include "Modulation/StepSequencer.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
void testMsegInterpolationAndLooping()
{
    tryst::mod::Mseg mseg;
    mseg.prepare(1000.0);
    const std::array points {
        tryst::mod::MsegPoint { 0.0f, -1.0f },
        tryst::mod::MsegPoint { 0.5f, 1.0f },
        tryst::mod::MsegPoint { 1.0f, -1.0f }
    };
    assert(mseg.configure(points, 1.0f, true));
    assert(std::abs(mseg.currentValue() + 1.0f) < 0.0001f);

    mseg.advanceSamples(250);
    assert(std::abs(mseg.currentValue()) < 0.01f);
    mseg.advanceSamples(250);
    assert(std::abs(mseg.currentValue() - 1.0f) < 0.01f);
    mseg.advanceSamples(500);
    assert(std::abs(mseg.currentValue() + 1.0f) < 0.01f);

    assert(!mseg.configure({ points.data(), 1 }, 1.0f, true));
}

void testMsegOneShot()
{
    tryst::mod::Mseg mseg;
    mseg.prepare(1000.0);
    const std::array points {
        tryst::mod::MsegPoint { 0.0f, 0.0f },
        tryst::mod::MsegPoint { 1.0f, 0.75f }
    };
    assert(mseg.configure(points, 1.0f, false));
    mseg.advanceSamples(2000);
    assert(std::abs(mseg.currentValue() - 0.75f) < 0.0001f);
}

void testStepSequencer()
{
    tryst::mod::StepSequencer step;
    step.prepare(1000.0);
    const std::array values { -1.0f, -0.5f, 0.5f, 1.0f };
    assert(step.configure(values, 1.0f, true));
    assert(std::abs(step.currentValue() + 1.0f) < 0.0001f);

    step.advanceSamples(250);
    assert(std::abs(step.currentValue() + 0.5f) < 0.0001f);
    step.advanceSamples(250);
    assert(std::abs(step.currentValue() - 0.5f) < 0.0001f);
    step.advanceSamples(250);
    assert(std::abs(step.currentValue() - 1.0f) < 0.0001f);
    step.advanceSamples(250);
    assert(std::abs(step.currentValue() + 1.0f) < 0.0001f);
}

float chunkEnergy(const std::array<float, 64>& audio, std::size_t offset)
{
    float energy = 0.0f;
    for (std::size_t i = offset; i < offset + 16; ++i)
        energy += std::abs(audio[i]);
    return energy;
}

void testStepFastRouteInVoiceManager()
{
    tryst::dsp::VoiceManager manager;
    manager.prepare(1000.0, 64);
    manager.setAmpEnvelope({ 0.0f, 0.0f, 1.0f, 0.0f });
    assert(manager.setEngineParameter(0, tryst::params::destination::generatorLevel, 0.5f));

    const std::array values { -1.0f, 1.0f };
    assert(manager.configureVoiceStep(0, values, 31.25f, true));
    assert(manager.addFastModulationRoute({
        { tryst::mod::SourceType::step, 0 },
        { tryst::mod::TargetDomain::generator, 0, tryst::params::destination::generatorLevel },
        1.0f, 0.0f, true
    }));

    manager.noteOn({ 60, 1.0f });
    std::array<float, 64> left {};
    std::array<float, 64> right {};
    tryst::dsp::ProcessContext context { 1000.0, left, right };
    manager.render(context);

    const auto e0 = chunkEnergy(left, 0);
    const auto e1 = chunkEnergy(left, 16);
    const auto e2 = chunkEnergy(left, 32);
    const auto e3 = chunkEnergy(left, 48);
    assert(e0 < 0.0001f);
    assert(e1 > 0.1f);
    assert(e2 < 0.0001f);
    assert(e3 > 0.1f);
}
}

int main()
{
    testMsegInterpolationAndLooping();
    testMsegOneShot();
    testStepSequencer();
    testStepFastRouteInVoiceManager();
    std::cout << "Tryst MSEG/step tests passed\n";
    return 0;
}
