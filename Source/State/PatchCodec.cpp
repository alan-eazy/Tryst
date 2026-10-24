#include "PatchCodec.h"
#include "Core/ParameterRegistry.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace tryst::state
{
namespace
{
std::string_view engineName(tryst::dsp::EngineKind kind)
{
    using K = tryst::dsp::EngineKind;
    switch (kind)
    {
        case K::analog: return "analog";
        case K::wavetable: return "wavetable";
        case K::sampler: return "sampler";
        case K::granular: return "granular";
        case K::spectral: return "spectral";
        case K::additive: return "additive";
        case K::physicalModel: return "physical_model";
        case K::noise: return "noise";
    }
    return "analog";
}

std::optional<tryst::dsp::EngineKind> parseEngine(std::string_view text)
{
    using K = tryst::dsp::EngineKind;
    if (text == "analog") return K::analog;
    if (text == "wavetable") return K::wavetable;
    if (text == "sampler") return K::sampler;
    if (text == "granular") return K::granular;
    if (text == "spectral") return K::spectral;
    if (text == "additive") return K::additive;
    if (text == "physical_model") return K::physicalModel;
    if (text == "noise") return K::noise;
    return std::nullopt;
}

std::string_view domainName(tryst::mod::TargetDomain domain)
{
    using D = tryst::mod::TargetDomain;
    switch (domain)
    {
        case D::generator: return "generator";
        case D::filter: return "filter";
        case D::global: return "global";
    }
    return "generator";
}

std::optional<tryst::mod::TargetDomain> parseDomain(std::string_view text)
{
    using D = tryst::mod::TargetDomain;
    if (text == "generator") return D::generator;
    if (text == "filter") return D::filter;
    if (text == "global") return D::global;
    return std::nullopt;
}

std::string_view sourceName(tryst::mod::SourceType type)
{
    using S = tryst::mod::SourceType;
    switch (type)
    {
        case S::lfo: return "lfo";
        case S::envelope: return "envelope";
        case S::mseg: return "mseg";
        case S::step: return "step";
        case S::random: return "random";
        case S::velocity: return "velocity";
        case S::keyTrack: return "key_track";
        case S::pressure: return "pressure";
        case S::timbre: return "timbre";
        case S::pitchBend: return "pitch_bend";
        case S::macro: return "macro";
        case S::morphX: return "morph_x";
        case S::morphY: return "morph_y";
        case S::envelopeFollower: return "envelope_follower";
        case S::count: break;
    }
    return "lfo";
}

std::optional<tryst::mod::SourceType> parseSource(std::string_view text)
{
    using S = tryst::mod::SourceType;
    if (text == "lfo") return S::lfo;
    if (text == "envelope") return S::envelope;
    if (text == "mseg") return S::mseg;
    if (text == "step") return S::step;
    if (text == "random") return S::random;
    if (text == "velocity") return S::velocity;
    if (text == "key_track") return S::keyTrack;
    if (text == "pressure") return S::pressure;
    if (text == "timbre") return S::timbre;
    if (text == "pitch_bend") return S::pitchBend;
    if (text == "macro") return S::macro;
    if (text == "morph_x") return S::morphX;
    if (text == "morph_y") return S::morphY;
    if (text == "envelope_follower") return S::envelopeFollower;
    return std::nullopt;
}

std::string_view waveformName(tryst::mod::LfoWaveform waveform)
{
    using W = tryst::mod::LfoWaveform;
    switch (waveform)
    {
        case W::sine: return "sine";
        case W::triangle: return "triangle";
        case W::saw: return "saw";
        case W::square: return "square";
    }
    return "sine";
}

std::optional<tryst::mod::LfoWaveform> parseWaveform(std::string_view text)
{
    using W = tryst::mod::LfoWaveform;
    if (text == "sine") return W::sine;
    if (text == "triangle") return W::triangle;
    if (text == "saw") return W::saw;
    if (text == "square") return W::square;
    return std::nullopt;
}

PatchDecodeResult failure(std::size_t line, std::string message)
{
    PatchDecodeResult result;
    result.errorLine = line;
    result.error = std::move(message);
    return result;
}

bool validNormalized(float value)
{
    return value >= 0.0f && value <= 1.0f;
}
}

std::string PatchCodec::encode(const PatchState& patch)
{
    std::ostringstream out;
    out << std::setprecision(9);
    out << "TRYST_PATCH " << patch.schemaVersion << '\n';

    for (std::size_t i = 0; i < patch.generators.size(); ++i)
        out << "engine " << i << ' ' << (patch.generators[i].enabled ? 1 : 0) << ' '
            << engineName(patch.generators[i].kind) << '\n';

    out << "amp " << patch.ampEnvelope.attackSeconds << ' ' << patch.ampEnvelope.decaySeconds << ' '
        << patch.ampEnvelope.sustainLevel << ' ' << patch.ampEnvelope.releaseSeconds << '\n';

    for (std::size_t i = 0; i < patch.macros.size(); ++i)
        out << "macro " << i << ' ' << patch.macros[i] << '\n';
    out << "morph " << patch.morphX << ' ' << patch.morphY << '\n';

    for (std::size_t i = 0; i < patch.lfos.size(); ++i)
        out << "lfo " << i << ' ' << patch.lfos[i].rateHz << ' ' << waveformName(patch.lfos[i].waveform) << '\n';

    for (std::size_t i = 0; i < patch.msegs.size(); ++i)
    {
        const auto& mseg = patch.msegs[i];
        if (mseg.points.empty())
            continue;
        out << "mseg " << i << ' ' << mseg.rateHz << ' ' << (mseg.loop ? 1 : 0) << ' ' << mseg.points.size();
        for (const auto& point : mseg.points)
            out << ' ' << point.phase << ' ' << point.value;
        out << '\n';
    }

    for (std::size_t i = 0; i < patch.steps.size(); ++i)
    {
        const auto& step = patch.steps[i];
        if (step.values.empty())
            continue;
        out << "step " << i << ' ' << step.rateHz << ' ' << (step.loop ? 1 : 0) << ' ' << step.values.size();
        for (const auto value : step.values)
            out << ' ' << value;
        out << '\n';
    }

    for (std::size_t note = 0; note < patch.tuningOffsetsSemitones.size(); ++note)
        if (std::abs(patch.tuningOffsetsSemitones[note]) > 0.0000001f)
            out << "tuning " << note << ' ' << patch.tuningOffsetsSemitones[note] << '\n';

    for (const auto& parameter : patch.parameters)
        out << "param " << domainName(parameter.domain) << ' ' << static_cast<unsigned>(parameter.slot) << ' '
            << parameter.parameterId << ' ' << parameter.normalized << '\n';

    for (const auto& route : patch.routes)
        out << "route " << (route.fast ? "fast" : "control") << ' '
            << sourceName(route.source.type) << ' ' << route.source.index << ' '
            << domainName(route.domain) << ' ' << static_cast<unsigned>(route.slot) << ' '
            << route.parameterId << ' ' << route.depth << ' ' << route.curve << ' '
            << (route.enabled ? 1 : 0) << '\n';

    out << "end\n";
    return out.str();
}

PatchDecodeResult PatchCodec::decode(std::string_view text)
{
    std::istringstream input { std::string(text) };
    PatchState patch;
    std::string line;
    std::size_t lineNumber = 0;
    bool sawHeader = false;
    bool sawEnd = false;
    std::uint32_t sourceVersion = PatchState::currentSchemaVersion;

    while (std::getline(input, line))
    {
        ++lineNumber;
        std::istringstream row(line);
        std::string command;
        if (!(row >> command))
            continue;
        if (!command.empty() && command.front() == '#')
            continue;

        if (!sawHeader)
        {
            if (command != "TRYST_PATCH")
                return failure(lineNumber, "missing TRYST_PATCH header");
            std::uint32_t version = 0;
            if (!(row >> version))
                return failure(lineNumber, "missing schema version");
            if (version < PatchState::minimumReadableSchemaVersion || version > PatchState::currentSchemaVersion)
                return failure(lineNumber, "unsupported schema version");
            sourceVersion = version;
            patch.schemaVersion = PatchState::currentSchemaVersion;
            sawHeader = true;
            continue;
        }

        if (command == "end")
        {
            sawEnd = true;
            break;
        }
        if (command == "engine")
        {
            std::size_t slot = 0;
            int enabled = 0;
            std::string kindText;
            if (!(row >> slot >> enabled >> kindText) || slot >= patch.generators.size())
                return failure(lineNumber, "invalid engine record");
            const auto kind = parseEngine(kindText);
            if (!kind)
                return failure(lineNumber, "unknown engine kind");
            patch.generators[slot] = { enabled != 0, *kind };
            continue;
        }
        if (command == "amp")
        {
            if (!(row >> patch.ampEnvelope.attackSeconds >> patch.ampEnvelope.decaySeconds
                      >> patch.ampEnvelope.sustainLevel >> patch.ampEnvelope.releaseSeconds))
                return failure(lineNumber, "invalid amp envelope");
            if (patch.ampEnvelope.attackSeconds < 0.0f || patch.ampEnvelope.decaySeconds < 0.0f
                || !validNormalized(patch.ampEnvelope.sustainLevel) || patch.ampEnvelope.releaseSeconds < 0.0f)
                return failure(lineNumber, "amp envelope out of range");
            continue;
        }
        if (command == "macro")
        {
            std::size_t index = 0;
            float value = 0.0f;
            if (!(row >> index >> value) || index >= patch.macros.size() || !validNormalized(value))
                return failure(lineNumber, "invalid macro record");
            patch.macros[index] = value;
            continue;
        }
        if (command == "morph")
        {
            if (!(row >> patch.morphX >> patch.morphY) || !validNormalized(patch.morphX) || !validNormalized(patch.morphY))
                return failure(lineNumber, "invalid morph record");
            continue;
        }
        if (command == "lfo")
        {
            std::size_t index = 0;
            float rate = 0.0f;
            std::string waveformText;
            if (!(row >> index >> rate >> waveformText) || index >= patch.lfos.size() || rate < 0.0f || rate > 100.0f)
                return failure(lineNumber, "invalid LFO record");
            const auto waveform = parseWaveform(waveformText);
            if (!waveform)
                return failure(lineNumber, "unknown LFO waveform");
            patch.lfos[index] = { rate, *waveform };
            continue;
        }
        if (command == "mseg")
        {
            if (sourceVersion < 2)
                return failure(lineNumber, "MSEG record not valid in schema 1");
            std::size_t index = 0;
            float rate = 0.0f;
            int loop = 0;
            std::size_t count = 0;
            if (!(row >> index >> rate >> loop >> count) || index >= patch.msegs.size()
                || rate < 0.0f || rate > 100.0f || count < 2 || count > tryst::mod::Mseg::maxPoints)
                return failure(lineNumber, "invalid MSEG record");

            MsegState state;
            state.rateHz = rate;
            state.loop = loop != 0;
            state.points.reserve(count);
            float previousPhase = -1.0f;
            for (std::size_t i = 0; i < count; ++i)
            {
                tryst::mod::MsegPoint point;
                if (!(row >> point.phase >> point.value) || point.phase < 0.0f || point.phase > 1.0f
                    || point.value < -1.0f || point.value > 1.0f || point.phase <= previousPhase)
                    return failure(lineNumber, "invalid MSEG point");
                previousPhase = point.phase;
                state.points.push_back(point);
            }
            if (std::abs(state.points.front().phase) > 0.00001f
                || std::abs(state.points.back().phase - 1.0f) > 0.00001f)
                return failure(lineNumber, "MSEG must start at 0 and end at 1");
            patch.msegs[index] = std::move(state);
            continue;
        }
        if (command == "step")
        {
            if (sourceVersion < 2)
                return failure(lineNumber, "step record not valid in schema 1");
            std::size_t index = 0;
            float rate = 0.0f;
            int loop = 0;
            std::size_t count = 0;
            if (!(row >> index >> rate >> loop >> count) || index >= patch.steps.size()
                || rate < 0.0f || rate > 100.0f || count == 0 || count > tryst::mod::StepSequencer::maxSteps)
                return failure(lineNumber, "invalid step record");

            StepState state;
            state.rateHz = rate;
            state.loop = loop != 0;
            state.values.reserve(count);
            for (std::size_t i = 0; i < count; ++i)
            {
                float value = 0.0f;
                if (!(row >> value) || value < -1.0f || value > 1.0f)
                    return failure(lineNumber, "invalid step value");
                state.values.push_back(value);
            }
            patch.steps[index] = std::move(state);
            continue;
        }
        if (command == "tuning")
        {
            if (sourceVersion < 3)
                return failure(lineNumber, "tuning record not valid before schema 3");
            std::size_t note = 0;
            float offset = 0.0f;
            if (!(row >> note >> offset) || note >= patch.tuningOffsetsSemitones.size()
                || !std::isfinite(offset) || offset < -48.0f || offset > 48.0f)
                return failure(lineNumber, "invalid tuning record");
            patch.tuningOffsetsSemitones[note] = offset;
            continue;
        }
        if (command == "param")
        {
            std::string domainText;
            unsigned slot = 0;
            ParameterState parameter;
            if (!(row >> domainText >> slot >> parameter.parameterId >> parameter.normalized) || slot > 255U
                || !validNormalized(parameter.normalized))
                return failure(lineNumber, "invalid parameter record");
            const auto domain = parseDomain(domainText);
            if (!domain)
                return failure(lineNumber, "unknown parameter domain");
            if (tryst::params::ParameterRegistry::find(parameter.parameterId) == nullptr)
                return failure(lineNumber, "unknown parameter id");
            parameter.domain = *domain;
            parameter.slot = static_cast<std::uint8_t>(slot);
            patch.parameters.push_back(std::move(parameter));
            continue;
        }
        if (command == "route")
        {
            std::string rateText;
            std::string sourceText;
            unsigned sourceIndex = 0;
            std::string domainText;
            unsigned slot = 0;
            RouteState route;
            int enabled = 0;
            if (!(row >> rateText >> sourceText >> sourceIndex >> domainText >> slot >> route.parameterId
                      >> route.depth >> route.curve >> enabled)
                || sourceIndex > 65535U || slot > 255U || route.depth < -1.0f || route.depth > 1.0f)
                return failure(lineNumber, "invalid route record");
            if (rateText != "control" && rateText != "fast")
                return failure(lineNumber, "unknown route rate");
            const auto source = parseSource(sourceText);
            const auto domain = parseDomain(domainText);
            if (!source || !domain)
                return failure(lineNumber, "unknown route source/domain");
            if (tryst::params::ParameterRegistry::find(route.parameterId) == nullptr)
                return failure(lineNumber, "unknown route parameter id");
            route.fast = rateText == "fast";
            route.source = { *source, static_cast<std::uint16_t>(sourceIndex) };
            route.domain = *domain;
            route.slot = static_cast<std::uint8_t>(slot);
            route.enabled = enabled != 0;
            patch.routes.push_back(std::move(route));
            continue;
        }

        return failure(lineNumber, "unknown patch record");
    }

    if (!sawHeader)
        return failure(0, "empty patch");
    if (!sawEnd)
        return failure(lineNumber, "missing end marker");

    PatchDecodeResult result;
    result.patch = std::move(patch);
    return result;
}
}
