#include "DSP/VoiceManager.h"
#include "Modulation/AdsrEnvelope.h"
#include "Modulation/Lfo.h"
#include "Modulation/ModMatrix.h"
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
    const tryst::mod::SourceId lfo { tryst::mod::SourceType::lfo, 0 };
    assert(matrix.addRoute({ lfo, 7, 0.25f, 0.0f, true }));
    const auto value = matrix.apply(7, 0.5f, lfo, 1.0f);
    assert(std::abs(value - 0.75f) < 0.0001f);
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
    testModMatrix();
    testAdsr();
    testLfo();
    testPolyphonicVoiceManager();
    std::cout << "Tryst core tests passed\n";
    return 0;
}
