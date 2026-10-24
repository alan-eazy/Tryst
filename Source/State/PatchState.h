#pragma once

#include "DSP/Engine.h"
#include "Modulation/AdsrEnvelope.h"
#include "Modulation/Lfo.h"
#include "Modulation/Mseg.h"
#include "Modulation/StepSequencer.h"
#include "Modulation/ModMatrix.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace tryst::state
{
struct GeneratorState
{
    bool enabled = false;
    tryst::dsp::EngineKind kind = tryst::dsp::EngineKind::analog;
};

struct ParameterState
{
    tryst::mod::TargetDomain domain = tryst::mod::TargetDomain::generator;
    std::uint8_t slot = 0;
    std::string parameterId;
    float normalized = 0.0f;
};

struct LfoState
{
    float rateHz = 1.0f;
    tryst::mod::LfoWaveform waveform = tryst::mod::LfoWaveform::sine;
};

struct MsegState
{
    float rateHz = 1.0f;
    bool loop = true;
    std::vector<tryst::mod::MsegPoint> points;
};

struct StepState
{
    float rateHz = 1.0f;
    bool loop = true;
    std::vector<float> values;
};

struct RouteState
{
    bool fast = false;
    tryst::mod::SourceId source;
    tryst::mod::TargetDomain domain = tryst::mod::TargetDomain::generator;
    std::uint8_t slot = 0;
    std::string parameterId;
    float depth = 0.0f;
    float curve = 0.0f;
    bool enabled = true;
};

struct PatchState
{
    static constexpr std::uint32_t currentSchemaVersion = 2;
    static constexpr std::uint32_t minimumReadableSchemaVersion = 1;

    std::uint32_t schemaVersion = currentSchemaVersion;
    std::array<GeneratorState, 4> generators {{ { true, tryst::dsp::EngineKind::analog }, {}, {}, {} }};
    tryst::mod::AdsrEnvelope::Parameters ampEnvelope {};
    std::array<float, 8> macros {};
    float morphX = 0.0f;
    float morphY = 0.0f;
    std::array<LfoState, 4> lfos {};
    std::array<MsegState, 2> msegs {};
    std::array<StepState, 2> steps {};
    std::vector<ParameterState> parameters;
    std::vector<RouteState> routes;
};
}
