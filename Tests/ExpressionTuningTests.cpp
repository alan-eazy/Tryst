#include "DSP/AnalogEngine.h"
#include "DSP/VoiceManager.h"
#include "Tuning/TuningTable.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
float energy(const std::array<float, 128>& audio)
{
    float total = 0.0f;
    for (auto sample : audio)
        total += std::abs(sample);
    return total;
}

std::size_t zeroCrossings(const std::array<float, 2048>& audio)
{
    std::size_t count = 0;
    for (std::size_t i = 1; i < audio.size(); ++i)
        if ((audio[i - 1] < 0.0f && audio[i] >= 0.0f) || (audio[i - 1] >= 0.0f && audio[i] < 0.0f))
            ++count;
    return count;
}

void testTuningTable()
{
    tryst::tuning::TuningTable tuning;
    assert(tuning.setOffsetSemitones(60, 0.25f));
    assert(std::abs(tuning.offsetSemitones(60) - 0.25f) < 0.0001f);
    assert(!tuning.setOffsetSemitones(128, 0.0f));

    const std::array<float, 12> octave {
        0.0f, 0.01f, -0.02f, 0.03f, -0.04f, 0.05f,
        -0.06f, 0.07f, -0.08f, 0.09f, -0.10f, 0.11f
    };
    assert(tuning.setOctaveOffsets(octave));
    assert(std::abs(tuning.offsetSemitones(61) - 0.01f) < 0.0001f);
    assert(std::abs(tuning.offsetSemitones(73) - 0.01f) < 0.0001f);
}

void testPressureFastRoute()
{
    tryst::dsp::VoiceManager manager;
    manager.prepare(48000.0, 128);
    manager.setAmpEnvelope({ 0.0f, 0.0f, 1.0f, 0.0f });
    assert(manager.setEngineParameter(0, tryst::params::destination::generatorLevel, 0.0f));
    assert(manager.addFastModulationRoute({
        { tryst::mod::SourceType::pressure, 0 },
        { tryst::mod::TargetDomain::generator, 0, tryst::params::destination::generatorLevel },
        1.0f, 0.0f, true
    }));

    manager.noteOn({ 60, 1.0f, 0.0f, 0.0f, 0.0f });
    std::array<float, 128> silentLeft {};
    std::array<float, 128> silentRight {};
    tryst::dsp::ProcessContext silent { 48000.0, silentLeft, silentRight };
    manager.render(silent);
    assert(energy(silentLeft) < 0.0001f);

    manager.updateNoteExpression(60, 1.0f, 0.0f, 0.0f);
    std::array<float, 128> loudLeft {};
    std::array<float, 128> loudRight {};
    tryst::dsp::ProcessContext loud { 48000.0, loudLeft, loudRight };
    manager.render(loud);
    assert(energy(loudLeft) > 1.0f);
}

void testLivePitchBend()
{
    tryst::dsp::AnalogEngine engine;
    engine.prepare(8000.0, 2048);
    assert(engine.setParameter(tryst::params::destination::generatorLevel, 1.0f));
    engine.noteOn({ 48, 1.0f });

    std::array<float, 2048> lower {};
    std::array<float, 2048> lowerRight {};
    tryst::dsp::ProcessContext lowerContext { 8000.0, lower, lowerRight };
    engine.render(lowerContext);
    const auto lowerCrossings = zeroCrossings(lower);

    assert(engine.setPitchBendSemitones(12.0f));
    std::array<float, 2048> upper {};
    std::array<float, 2048> upperRight {};
    tryst::dsp::ProcessContext upperContext { 8000.0, upper, upperRight };
    engine.render(upperContext);
    const auto upperCrossings = zeroCrossings(upper);

    assert(upperCrossings > lowerCrossings * 1.8);
}

void testVoiceManagerMicrotuning()
{
    tryst::dsp::VoiceManager manager;
    manager.prepare(48000.0, 128);
    assert(manager.setTuningOffsetSemitones(60, 0.37f));
    assert(std::abs(manager.tuningOffsetSemitones(60) - 0.37f) < 0.0001f);
    manager.resetTuning();
    assert(std::abs(manager.tuningOffsetSemitones(60)) < 0.0001f);

    tryst::dsp::VoiceManager equal;
    equal.prepare(8000.0, 2048);
    equal.setAmpEnvelope({ 0.0f, 0.0f, 1.0f, 0.0f });
    assert(equal.setEngineParameter(0, tryst::params::destination::generatorLevel, 1.0f));
    equal.noteOn({ 48, 1.0f });
    std::array<float, 2048> equalLeft {};
    std::array<float, 2048> equalRight {};
    tryst::dsp::ProcessContext equalContext { 8000.0, equalLeft, equalRight };
    equal.render(equalContext);

    tryst::dsp::VoiceManager octaveUp;
    octaveUp.prepare(8000.0, 2048);
    octaveUp.setAmpEnvelope({ 0.0f, 0.0f, 1.0f, 0.0f });
    assert(octaveUp.setEngineParameter(0, tryst::params::destination::generatorLevel, 1.0f));
    assert(octaveUp.setTuningOffsetSemitones(48, 12.0f));
    octaveUp.noteOn({ 48, 1.0f });
    std::array<float, 2048> tunedLeft {};
    std::array<float, 2048> tunedRight {};
    tryst::dsp::ProcessContext tunedContext { 8000.0, tunedLeft, tunedRight };
    octaveUp.render(tunedContext);

    assert(zeroCrossings(tunedLeft) > zeroCrossings(equalLeft) * 1.8);
}
}

int main()
{
    testTuningTable();
    testPressureFastRoute();
    testLivePitchBend();
    testVoiceManagerMicrotuning();
    std::cout << "Tryst expression/tuning tests passed\n";
    return 0;
}
