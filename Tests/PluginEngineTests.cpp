#include "Plugin/PluginEngine.h"
#include "State/PatchCodec.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
float energy(std::span<const float> audio)
{
    float result = 0.0f;
    for (const auto sample : audio)
        result += std::abs(sample);
    return result;
}

void testParameterCatalog()
{
    tryst::plugin::PluginEngine engine;
    const auto& catalog = engine.parameterCatalog();
    assert(catalog.valid());
    assert(catalog.all().size() > 80);

    const auto* macro = catalog.find("performance.macro.1");
    assert(macro != nullptr);
    assert(macro->kind == tryst::plugin::HostParameterKind::macro);

    const auto* shape = catalog.find("generator.1.engine.analog.shape");
    assert(shape != nullptr);
    assert(shape->domain == tryst::mod::TargetDomain::generator);
    assert(shape->slot == 0);

    for (std::size_t i = 0; i < catalog.all().size(); ++i)
        for (std::size_t j = i + 1; j < catalog.all().size(); ++j)
            assert(catalog.all()[i].id != catalog.all()[j].id);
}

void testSampleAccurateParameterEvent()
{
    tryst::plugin::PluginEngine engine;
    assert(engine.prepare(8000.0, 64));

    const auto* level = engine.parameterCatalog().find("generator.1.generator.level");
    assert(level != nullptr);
    assert(engine.setHostParameter(level->id, 1.0f));

    const std::array events {
        tryst::plugin::HostEvent { 0, tryst::plugin::HostEventType::noteOn, 60, 7, 1.0f },
        tryst::plugin::HostEvent { 32, tryst::plugin::HostEventType::parameter, 60, -1, 1.0f, 0.0f, 0.0f, 0.0f, level->id, 0.0f }
    };

    std::array<float, 64> left {};
    std::array<float, 64> right {};
    assert(engine.process(left, right, events));
    assert(energy(std::span<const float>(left.data(), 32)) > 0.5f);
    assert(energy(std::span<const float>(left.data() + 32, 32)) < 0.0001f);
}

void testSameKeyNoteIds()
{
    tryst::plugin::PluginEngine engine;
    auto patch = engine.patchState();
    patch.ampEnvelope = { 0.0f, 0.0f, 1.0f, 0.0f };
    assert(engine.loadState(tryst::state::PatchCodec::encode(patch)));
    assert(engine.prepare(48000.0, 128));

    const std::array starts {
        tryst::plugin::HostEvent { 0, tryst::plugin::HostEventType::noteOn, 60, 101, 1.0f },
        tryst::plugin::HostEvent { 0, tryst::plugin::HostEventType::noteOn, 60, 202, 1.0f }
    };
    std::array<float, 128> left {};
    std::array<float, 128> right {};
    assert(engine.process(left, right, starts));
    assert(engine.activeVoiceCount() == 2);

    const std::array stopOne {
        tryst::plugin::HostEvent { 0, tryst::plugin::HostEventType::noteOff, 60, 101, 0.0f }
    };
    assert(engine.process(left, right, stopOne));
    assert(engine.activeVoiceCount() == 1);
}

void testStateAndTopologyRoundTrip()
{
    tryst::plugin::PluginEngine source;
    assert(source.setGeneratorKind(1, tryst::dsp::EngineKind::wavetable, true));
    assert(source.setEffectKind(0, tryst::fx::EffectKind::drive, true));
    const std::array<std::uint8_t, 4> order { 1, 0, 2, 3 };
    assert(source.setEffectOrder(order));
    assert(source.setTuningOffsetSemitones(60, 0.25f));

    const auto* wtPosition = source.parameterCatalog().find("generator.2.engine.wavetable.position");
    const auto* driveAmount = source.parameterCatalog().find("effect.1.fx.drive.amount");
    const auto* macro = source.parameterCatalog().find("performance.macro.1");
    assert(wtPosition && driveAmount && macro);
    assert(source.setHostParameter(wtPosition->id, 0.72f));
    assert(source.setHostParameter(driveAmount->id, 0.81f));
    assert(source.setHostParameter(macro->id, 0.33f));

    const auto text = source.saveState();
    auto decoded = tryst::state::PatchCodec::decode(text);
    assert(decoded);
    assert(decoded.patch->generators[1].kind == tryst::dsp::EngineKind::wavetable);
    assert(decoded.patch->effects[0].kind == tryst::fx::EffectKind::drive);
    assert(decoded.patch->effectOrder[0] == 1);

    tryst::plugin::PluginEngine restored;
    assert(restored.loadState(text));
    assert(std::abs(restored.hostParameterValue(wtPosition->id) - 0.72f) < 0.0001f);
    assert(std::abs(restored.hostParameterValue(driveAmount->id) - 0.81f) < 0.0001f);
    assert(std::abs(restored.hostParameterValue(macro->id) - 0.33f) < 0.0001f);
    assert(restored.prepare(48000.0, 128));
}

void testEventOrderingValidation()
{
    tryst::plugin::PluginEngine engine;
    assert(engine.prepare(48000.0, 32));
    const std::array events {
        tryst::plugin::HostEvent { 10, tryst::plugin::HostEventType::noteOn, 60, 1, 1.0f },
        tryst::plugin::HostEvent { 5, tryst::plugin::HostEventType::noteOff, 60, 1, 0.0f }
    };
    std::array<float, 32> left {};
    std::array<float, 32> right {};
    assert(!engine.process(left, right, events));
}
}

int main()
{
    testParameterCatalog();
    testSampleAccurateParameterEvent();
    testSameKeyNoteIds();
    testStateAndTopologyRoundTrip();
    testEventOrderingValidation();
    std::cout << "Tryst plugin engine tests passed\n";
    return 0;
}
