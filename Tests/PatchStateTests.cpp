#include "State/PatchApplier.h"
#include "State/PatchCodec.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
tryst::state::PatchState makePatch()
{
    tryst::state::PatchState patch;
    patch.generators[0] = { true, tryst::dsp::EngineKind::analog };
    patch.generators[1] = { true, tryst::dsp::EngineKind::wavetable };
    patch.generators[2].enabled = false;
    patch.generators[3].enabled = false;
    patch.ampEnvelope = { 0.002f, 0.08f, 0.72f, 0.15f };
    patch.macros[0] = 0.75f;
    patch.macros[3] = 0.25f;
    patch.morphX = 0.4f;
    patch.morphY = 0.6f;
    patch.lfos[0] = { 5.0f, tryst::mod::LfoWaveform::square };

    patch.msegs[0].rateHz = 0.5f;
    patch.msegs[0].loop = true;
    patch.msegs[0].points = {
        { 0.0f, -1.0f },
        { 0.25f, 0.5f },
        { 1.0f, -0.25f }
    };
    patch.steps[0].rateHz = 2.0f;
    patch.steps[0].loop = true;
    patch.steps[0].values = { -1.0f, 0.0f, 1.0f, 0.25f };
    patch.tuningOffsetsSemitones[60] = 0.37f;
    patch.tuningOffsetsSemitones[61] = -0.12f;

    patch.parameters.push_back({ tryst::mod::TargetDomain::generator, 0, "generator.level", 0.45f });
    patch.parameters.push_back({ tryst::mod::TargetDomain::generator, 0, "engine.analog.shape", 0.20f });
    patch.parameters.push_back({ tryst::mod::TargetDomain::generator, 1, "generator.level", 0.25f });
    patch.parameters.push_back({ tryst::mod::TargetDomain::generator, 1, "engine.wavetable.position", 0.70f });
    patch.parameters.push_back({ tryst::mod::TargetDomain::filter, 0, "filter.1.enabled", 1.0f });
    patch.parameters.push_back({ tryst::mod::TargetDomain::filter, 0, "filter.1.cutoff", 0.65f });

    patch.routes.push_back({
        false,
        { tryst::mod::SourceType::macro, 0 },
        tryst::mod::TargetDomain::generator,
        0,
        "engine.analog.shape",
        0.30f,
        0.0f,
        true
    });
    patch.routes.push_back({
        true,
        { tryst::mod::SourceType::lfo, 0 },
        tryst::mod::TargetDomain::generator,
        1,
        "generator.level",
        0.20f,
        0.0f,
        true
    });
    patch.routes.push_back({
        true,
        { tryst::mod::SourceType::mseg, 0 },
        tryst::mod::TargetDomain::filter,
        0,
        "filter.1.cutoff",
        0.25f,
        0.0f,
        true
    });
    patch.routes.push_back({
        true,
        { tryst::mod::SourceType::step, 0 },
        tryst::mod::TargetDomain::generator,
        0,
        "engine.analog.shape",
        0.30f,
        0.0f,
        true
    });
    return patch;
}

void testRoundTrip()
{
    const auto source = makePatch();
    const auto encoded = tryst::state::PatchCodec::encode(source);
    const auto decoded = tryst::state::PatchCodec::decode(encoded);
    assert(decoded);
    assert(decoded.patch->schemaVersion == tryst::state::PatchState::currentSchemaVersion);
    assert(decoded.patch->generators[1].enabled);
    assert(decoded.patch->generators[1].kind == tryst::dsp::EngineKind::wavetable);
    assert(decoded.patch->parameters.size() == source.parameters.size());
    assert(decoded.patch->routes.size() == source.routes.size());
    assert(decoded.patch->msegs[0].points.size() == 3);
    assert(decoded.patch->steps[0].values.size() == 4);
    assert(std::abs(decoded.patch->msegs[0].points[1].value - 0.5f) < 0.00001f);
    assert(std::abs(decoded.patch->steps[0].values[2] - 1.0f) < 0.00001f);
    assert(std::abs(decoded.patch->macros[0] - 0.75f) < 0.00001f);
    assert(std::abs(decoded.patch->tuningOffsetsSemitones[60] - 0.37f) < 0.00001f);
    assert(tryst::state::PatchCodec::encode(*decoded.patch) == encoded);
}

void testApplyToVoiceManager()
{
    const auto encoded = tryst::state::PatchCodec::encode(makePatch());
    const auto decoded = tryst::state::PatchCodec::decode(encoded);
    assert(decoded);

    tryst::dsp::VoiceManager manager;
    manager.prepare(48000.0, 128);
    assert(tryst::state::PatchApplier::apply(*decoded.patch, manager));
    assert(manager.modulationRouteCount() == 1);
    assert(manager.fastModulationRouteCount() == 3);
    assert(std::abs(manager.tuningOffsetSemitones(60) - 0.37f) < 0.00001f);

    manager.noteOn({ 64, 0.9f });
    std::array<float, 128> left {};
    std::array<float, 128> right {};
    tryst::dsp::ProcessContext context { 48000.0, left, right };
    manager.render(context);
    assert(std::any_of(left.begin(), left.end(), [](float value) { return std::abs(value) > 0.0001f; }));
}

void testDecodeValidation()
{
    const auto unsupported = tryst::state::PatchCodec::decode("TRYST_PATCH 99\nend\n");
    assert(!unsupported);
    assert(!unsupported.error.empty());

    const auto unknownParameter = tryst::state::PatchCodec::decode(
        "TRYST_PATCH 1\nparam generator 0 engine.unknown.parameter 0.5\nend\n");
    assert(!unknownParameter);

    const auto missingEnd = tryst::state::PatchCodec::decode("TRYST_PATCH 1\nengine 0 1 analog\n");
    assert(!missingEnd);

    const auto migrated = tryst::state::PatchCodec::decode("TRYST_PATCH 1\nend\n");
    assert(migrated);
    assert(migrated.patch->schemaVersion == tryst::state::PatchState::currentSchemaVersion);
    assert(migrated.patch->msegs[0].points.empty());
    assert(migrated.patch->steps[0].values.empty());
    const auto migratedText = tryst::state::PatchCodec::encode(*migrated.patch);
    assert(migratedText.rfind("TRYST_PATCH 3\n", 0) == 0);

    const auto migratedV2 = tryst::state::PatchCodec::decode("TRYST_PATCH 2\nend\n");
    assert(migratedV2);
    assert(migratedV2.patch->schemaVersion == tryst::state::PatchState::currentSchemaVersion);
    assert(std::abs(migratedV2.patch->tuningOffsetsSemitones[60]) < 0.00001f);

    const auto badTuning = tryst::state::PatchCodec::decode("TRYST_PATCH 3\ntuning 60 50\nend\n");
    assert(!badTuning);

    const auto badMseg = tryst::state::PatchCodec::decode(
        "TRYST_PATCH 2\nmseg 0 1 1 2 0 0 0.5 1\nend\n");
    assert(!badMseg);
}
}

int main()
{
    testRoundTrip();
    testApplyToVoiceManager();
    testDecodeValidation();
    std::cout << "Tryst patch state tests passed\n";
    return 0;
}
