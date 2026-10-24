#include "DSP/NoiseEngine.h"
#include "DSP/VoiceManager.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
void testStereoWidthAndDeterminism()
{
    tryst::dsp::NoiseEngine mono;
    mono.prepare(48000.0, 256);
    assert(mono.setParameter(tryst::params::destination::generatorLevel, 1.0f));
    assert(mono.setParameter(tryst::params::destination::noiseStereo, 0.0f));
    mono.noteOn({ 60, 1.0f });

    std::array<float, 256> monoLeft {};
    std::array<float, 256> monoRight {};
    tryst::dsp::ProcessContext monoContext { 48000.0, monoLeft, monoRight };
    mono.render(monoContext);
    for (std::size_t i = 0; i < monoLeft.size(); ++i)
        assert(std::abs(monoLeft[i] - monoRight[i]) < 0.000001f);

    tryst::dsp::NoiseEngine stereoA;
    tryst::dsp::NoiseEngine stereoB;
    stereoA.prepare(48000.0, 256);
    stereoB.prepare(48000.0, 256);
    assert(stereoA.setParameter(tryst::params::destination::generatorLevel, 1.0f));
    assert(stereoB.setParameter(tryst::params::destination::generatorLevel, 1.0f));
    stereoA.noteOn({ 67, 0.8f });
    stereoB.noteOn({ 67, 0.8f });

    std::array<float, 256> leftA {};
    std::array<float, 256> rightA {};
    std::array<float, 256> leftB {};
    std::array<float, 256> rightB {};
    tryst::dsp::ProcessContext contextA { 48000.0, leftA, rightA };
    tryst::dsp::ProcessContext contextB { 48000.0, leftB, rightB };
    stereoA.render(contextA);
    stereoB.render(contextB);

    float stereoDifference = 0.0f;
    for (std::size_t i = 0; i < leftA.size(); ++i)
    {
        assert(std::abs(leftA[i] - leftB[i]) < 0.000001f);
        assert(std::abs(rightA[i] - rightB[i]) < 0.000001f);
        stereoDifference += std::abs(leftA[i] - rightA[i]);
    }
    assert(stereoDifference > 1.0f);
}

void testColorAndTexture()
{
    tryst::dsp::NoiseEngine white;
    tryst::dsp::NoiseEngine brown;
    white.prepare(48000.0, 512);
    brown.prepare(48000.0, 512);
    assert(white.setParameter(tryst::params::destination::generatorLevel, 1.0f));
    assert(brown.setParameter(tryst::params::destination::generatorLevel, 1.0f));
    assert(white.setParameter(tryst::params::destination::noiseColor, 0.0f));
    assert(brown.setParameter(tryst::params::destination::noiseColor, 1.0f));
    white.noteOn({ 60, 1.0f });
    brown.noteOn({ 60, 1.0f });

    std::array<float, 512> whiteLeft {};
    std::array<float, 512> whiteRight {};
    std::array<float, 512> brownLeft {};
    std::array<float, 512> brownRight {};
    tryst::dsp::ProcessContext whiteContext { 48000.0, whiteLeft, whiteRight };
    tryst::dsp::ProcessContext brownContext { 48000.0, brownLeft, brownRight };
    white.render(whiteContext);
    brown.render(brownContext);

    float difference = 0.0f;
    for (std::size_t i = 0; i < whiteLeft.size(); ++i)
        difference += std::abs(whiteLeft[i] - brownLeft[i]);
    assert(difference > 10.0f);

    tryst::dsp::NoiseEngine textured;
    textured.prepare(48000.0, 512);
    assert(textured.setParameter(tryst::params::destination::generatorLevel, 1.0f));
    assert(textured.setParameter(tryst::params::destination::noiseTexture, 1.0f));
    textured.noteOn({ 60, 1.0f });
    std::array<float, 512> textureLeft {};
    std::array<float, 512> textureRight {};
    tryst::dsp::ProcessContext textureContext { 48000.0, textureLeft, textureRight };
    textured.render(textureContext);

    std::size_t changes = 0;
    for (std::size_t i = 1; i < textureLeft.size(); ++i)
        if (std::abs(textureLeft[i] - textureLeft[i - 1]) > 0.000001f)
            ++changes;
    assert(changes < 20);
}

void testNoiseInVoiceManager()
{
    tryst::dsp::VoiceManager manager;
    manager.prepare(48000.0, 256);
    manager.setAmpEnvelope({ 0.0f, 0.0f, 1.0f, 0.0f });
    assert(manager.setEngineKind(3, tryst::dsp::EngineKind::noise));
    assert(manager.setEngineParameter(3, tryst::params::destination::generatorLevel, 0.4f));
    assert(manager.setEngineParameter(3, tryst::params::destination::noiseColor, 0.5f));
    assert(manager.setEngineParameter(3, tryst::params::destination::noiseStereo, 0.7f));
    manager.noteOn({ 72, 1.0f });

    std::array<float, 256> left {};
    std::array<float, 256> right {};
    tryst::dsp::ProcessContext context { 48000.0, left, right };
    manager.render(context);
    assert(std::any_of(left.begin(), left.end(), [](float value) { return std::abs(value) > 0.001f; }));
}
}

int main()
{
    testStereoWidthAndDeterminism();
    testColorAndTexture();
    testNoiseInVoiceManager();
    std::cout << "Tryst noise engine tests passed\n";
    return 0;
}
