#include "Voice.h"
#include <algorithm>

namespace tryst::dsp
{
void Voice::prepare(double newSampleRate, std::uint32_t maxBlockSize)
{
    sampleRate = newSampleRate > 1.0 ? newSampleRate : 48000.0;
    preparedBlockSize = std::max<std::uint32_t>(1, maxBlockSize);
    scratchLeft.assign(preparedBlockSize, 0.0f);
    scratchRight.assign(preparedBlockSize, 0.0f);
    ampEnvelope.prepare(sampleRate);
    fastModulation.prepare(sampleRate);
    filters.prepare(sampleRate);

    for (auto& slot : slots)
        if (auto* engine = slot.get())
            engine->prepare(sampleRate, preparedBlockSize);

    reset();
}

void Voice::reset() noexcept
{
    ampEnvelope.reset();
    fastModulation.resetPhases();
    filters.reset();
    stopEngines();
    currentNote = -1;
    currentNoteId = -1;
    voiceAge = 0;
    active = false;
    releasing = false;
}

bool Voice::setEngineKind(std::size_t slotIndex, EngineKind kind)
{
    if (slotIndex >= slots.size())
        return false;

    auto engine = createEngine(kind);
    if (!engine)
        return false;

    if (preparedBlockSize > 0)
        engine->prepare(sampleRate, preparedBlockSize);

    slots[slotIndex].setEngine(std::move(engine));
    return true;
}

void Voice::clearEngine(std::size_t slotIndex) noexcept
{
    if (slotIndex < slots.size())
        slots[slotIndex].setEngine({});
}

bool Voice::setEngineParameter(std::size_t slotIndex, tryst::params::DestinationId destination, float normalized) noexcept
{
    if (slotIndex >= slots.size())
        return false;

    const tryst::mod::TargetId target { tryst::mod::TargetDomain::generator,
                                        static_cast<std::uint8_t>(slotIndex),
                                        destination };
    if (!fastModulation.setBase(target, normalized))
        return false;

    if (auto* engine = slots[slotIndex].get())
        return engine->setParameter(destination, normalized);

    return false;
}

bool Voice::setEngineAsset(std::size_t slotIndex, std::shared_ptr<const EngineAsset> asset) noexcept
{
    if (slotIndex >= slots.size() || !asset)
        return false;

    if (auto* engine = slots[slotIndex].get())
        return engine->setAsset(std::move(asset));

    return false;
}

bool Voice::setFilterParameter(tryst::params::DestinationId destination, float normalized) noexcept
{
    const tryst::mod::TargetId target { tryst::mod::TargetDomain::filter, 0, destination };
    if (!fastModulation.setBase(target, normalized))
        return false;
    return filters.setParameter(destination, normalized);
}

void Voice::setAmpEnvelope(tryst::mod::AdsrEnvelope::Parameters parameters) noexcept
{
    ampEnvelope.setParameters(parameters);
}


bool Voice::configureFastLfo(std::size_t index, float rateHz, tryst::mod::LfoWaveform waveform) noexcept
{
    return fastModulation.configureLfo(index, rateHz, waveform);
}

bool Voice::configureFastMseg(std::size_t index,
                              std::span<const tryst::mod::MsegPoint> points,
                              float rateHz,
                              bool loop) noexcept
{
    return fastModulation.configureMseg(index, points, rateHz, loop);
}

bool Voice::configureFastStep(std::size_t index,
                              std::span<const float> values,
                              float rateHz,
                              bool loop) noexcept
{
    return fastModulation.configureStep(index, values, rateHz, loop);
}

bool Voice::addFastModulationRoute(tryst::mod::Route route) noexcept
{
    return fastModulation.addRoute(route);
}

void Voice::clearFastModulationRoutes() noexcept
{
    fastModulation.clearRoutes();
}

void Voice::start(const NoteEvent& event, std::uint64_t newAge) noexcept
{
    for (auto& slot : slots)
    {
        if (auto* engine = slot.get())
        {
            engine->reset();
            engine->noteOn(event);
        }
    }

    fastModulation.resetPhases();
    (void) fastModulation.setExpressionSource({ tryst::mod::SourceType::velocity, 0 }, std::clamp(event.velocity, 0.0f, 1.0f));
    const auto keyTrack = (std::clamp(static_cast<float>(event.note), 0.0f, 127.0f) / 127.0f) * 2.0f - 1.0f;
    (void) fastModulation.setExpressionSource({ tryst::mod::SourceType::keyTrack, 0 }, keyTrack);
    (void) fastModulation.setExpressionSource({ tryst::mod::SourceType::pressure, 0 }, std::clamp(event.pressure, 0.0f, 1.0f));
    (void) fastModulation.setExpressionSource({ tryst::mod::SourceType::timbre, 0 }, std::clamp(event.timbre, 0.0f, 1.0f));
    (void) fastModulation.setExpressionSource({ tryst::mod::SourceType::pitchBend, 0 }, std::clamp(event.pitchBendSemitones / 48.0f, -1.0f, 1.0f));
    currentNote = event.note;
    currentNoteId = event.noteId;
    voiceAge = newAge;
    active = true;
    releasing = false;
    ampEnvelope.noteOn();
}

void Voice::release(std::int32_t noteNumber, float, std::int32_t noteId) noexcept
{
    if (!matches(noteNumber, noteId))
        return;

    ampEnvelope.noteOff();
    releasing = true;
}

void Voice::updateExpression(float pressure, float timbre, float pitchBendSemitones) noexcept
{
    if (!active)
        return;

    (void) fastModulation.setExpressionSource({ tryst::mod::SourceType::pressure, 0 }, std::clamp(pressure, 0.0f, 1.0f));
    (void) fastModulation.setExpressionSource({ tryst::mod::SourceType::timbre, 0 }, std::clamp(timbre, 0.0f, 1.0f));
    const auto bend = std::clamp(pitchBendSemitones, -48.0f, 48.0f);
    (void) fastModulation.setExpressionSource({ tryst::mod::SourceType::pitchBend, 0 }, bend / 48.0f);

    for (auto& slot : slots)
        if (auto* engine = slot.get())
            (void) engine->setPitchBendSemitones(bend);
}

void Voice::render(ProcessContext& context) noexcept
{
    if (!active || context.left.empty() || context.right.empty())
        return;

    const auto samples = std::min({ context.left.size(), context.right.size(), scratchLeft.size(), scratchRight.size() });
    if (samples == 0)
        return;

    std::fill_n(scratchLeft.begin(), static_cast<std::ptrdiff_t>(samples), 0.0f);
    std::fill_n(scratchRight.begin(), static_cast<std::ptrdiff_t>(samples), 0.0f);

    for (std::size_t offset = 0; offset < samples; offset += tryst::mod::FastModulationRuntime::preferredChunkSize)
    {
        const auto chunkSamples = std::min<std::size_t>(
            tryst::mod::FastModulationRuntime::preferredChunkSize, samples - offset);

        fastModulation.applyCurrent([this](tryst::mod::TargetId target, float normalized) noexcept {
            applyFastTarget(target, normalized);
        });

        ProcessContext chunkContext {
            context.sampleRate,
            std::span<float>(scratchLeft.data() + offset, chunkSamples),
            std::span<float>(scratchRight.data() + offset, chunkSamples)
        };

        for (auto& slot : slots)
            if (auto* engine = slot.get())
                engine->render(chunkContext);

        filters.process(chunkContext.left, chunkContext.right);

        for (std::size_t i = 0; i < chunkSamples; ++i)
        {
            const auto gain = ampEnvelope.nextSample();
            context.left[offset + i] += scratchLeft[offset + i] * gain;
            context.right[offset + i] += scratchRight[offset + i] * gain;
        }

        fastModulation.advance(static_cast<std::uint32_t>(chunkSamples));
    }

    if (!ampEnvelope.isActive())
    {
        stopEngines();
        active = false;
        releasing = false;
        currentNote = -1;
        currentNoteId = -1;
    }
}

void Voice::applyFastTarget(tryst::mod::TargetId target, float normalized) noexcept
{
    if (target.domain == tryst::mod::TargetDomain::generator)
    {
        if (target.slot < slots.size())
            if (auto* engine = slots[target.slot].get())
                (void) engine->setParameter(target.parameter, normalized);
        return;
    }

    if (target.domain == tryst::mod::TargetDomain::filter)
        (void) filters.setParameter(target.parameter, normalized);
}

void Voice::stopEngines() noexcept
{
    for (auto& slot : slots)
    {
        if (auto* engine = slot.get())
        {
            if (currentNote >= 0)
                engine->noteOff(currentNote, 0.0f);
            engine->reset();
        }
    }
}
}
