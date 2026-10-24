#include "FX/DriveEffect.h"
#include "FX/DelayEffect.h"
#include "FX/FxRack.h"
#include "DSP/VoiceManager.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
void testDriveChangesSignal()
{
    tryst::fx::DriveEffect drive;
    drive.prepare(48000.0, 64);
    assert(drive.setParameter(tryst::params::destination::fxMix, 1.0f));
    assert(drive.setParameter(tryst::params::destination::fxDriveAmount, 1.0f));

    std::array<float, 64> left {};
    std::array<float, 64> right {};
    left.fill(0.25f);
    right.fill(-0.25f);
    drive.process(left, right);

    assert(std::abs(left[0] - 0.25f) > 0.05f);
    assert(std::abs(right[0] + 0.25f) > 0.05f);
    for (auto sample : left) assert(std::isfinite(sample));
}

void testDelayImpulse()
{
    tryst::fx::DelayEffect delay;
    delay.prepare(1000.0, 16);
    assert(delay.setParameter(tryst::params::destination::fxMix, 1.0f));
    assert(delay.setParameter(tryst::params::destination::fxDelayTime, 0.0f));
    assert(delay.setParameter(tryst::params::destination::fxDelayFeedback, 0.0f));

    std::array<float, 16> left {};
    std::array<float, 16> right {};
    left[0] = 1.0f;
    tryst::fx::Effect& effect = delay;
    effect.process(left, right);

    assert(std::abs(left[0]) < 0.0001f);
    assert(left[1] > 0.9f);
}

void testRackSwap()
{
    tryst::fx::FxRack rack;
    rack.prepare(48000.0, 64);
    assert(rack.setEffectKind(0, tryst::fx::EffectKind::drive));
    assert(rack.setEffectKind(1, tryst::fx::EffectKind::delay));
    assert(rack.effect(0) && rack.effect(0)->kind() == tryst::fx::EffectKind::drive);
    assert(rack.effect(1) && rack.effect(1)->kind() == tryst::fx::EffectKind::delay);
    assert(rack.swapSlots(0, 1));
    // Slot identity stays stable for automation; only processing order changes.
    assert(rack.effect(0)->kind() == tryst::fx::EffectKind::drive);
    assert(rack.effect(1)->kind() == tryst::fx::EffectKind::delay);
    assert(rack.processingSlot(0) == 1);
    assert(rack.processingSlot(1) == 0);
}

void testVoiceManagerEffectModulation()
{
    tryst::dsp::VoiceManager manager;
    manager.prepare(48000.0, 128);
    manager.setAmpEnvelope({ 0.0f, 0.0f, 1.0f, 0.0f });
    assert(manager.setEffectKind(0, tryst::fx::EffectKind::drive));
    assert(manager.setEffectParameter(0, tryst::params::destination::fxDriveAmount, 1.0f));
    assert(manager.setEffectParameter(0, tryst::params::destination::fxMix, 0.0f));
    assert(manager.addModulationRoute({
        { tryst::mod::SourceType::macro, 0 },
        { tryst::mod::TargetDomain::effect, 0, tryst::params::destination::fxMix },
        1.0f, 0.0f, true
    }));
    assert(!manager.addFastModulationRoute({
        { tryst::mod::SourceType::lfo, 0 },
        { tryst::mod::TargetDomain::effect, 0, tryst::params::destination::fxMix },
        1.0f, 0.0f, true
    }));
    assert(manager.setMacro(0, 1.0f));

    manager.noteOn({ 60, 1.0f });
    std::array<float, 128> left {};
    std::array<float, 128> right {};
    tryst::dsp::ProcessContext context { 48000.0, left, right };
    manager.render(context);
    bool audible = false;
    for (auto sample : left)
    {
        assert(std::isfinite(sample));
        audible = audible || std::abs(sample) > 0.001f;
    }
    assert(audible);
}
}

int main()
{
    testDriveChangesSignal();
    testDelayImpulse();
    testRackSwap();
    testVoiceManagerEffectModulation();
    std::cout << "Tryst FX rack tests passed\n";
    return 0;
}
