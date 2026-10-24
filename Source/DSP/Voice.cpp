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
    filters.prepare(sampleRate);

    for (auto& slot : slots)
        if (auto* engine = slot.get())
            engine->prepare(sampleRate, preparedBlockSize);

    reset();
}

void Voice::reset() noexcept
{
    ampEnvelope.reset();
    filters.reset();
    stopEngines();
    currentNote = -1;
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

    if (auto* engine = slots[slotIndex].get())
        return engine->setParameter(destination, normalized);

    return false;
}

bool Voice::setFilterParameter(tryst::params::DestinationId destination, float normalized) noexcept
{
    return filters.setParameter(destination, normalized);
}

void Voice::setAmpEnvelope(tryst::mod::AdsrEnvelope::Parameters parameters) noexcept
{
    ampEnvelope.setParameters(parameters);
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

    currentNote = event.note;
    voiceAge = newAge;
    active = true;
    releasing = false;
    ampEnvelope.noteOn();
}

void Voice::release(std::int32_t noteNumber, float) noexcept
{
    if (!matches(noteNumber))
        return;

    ampEnvelope.noteOff();
    releasing = true;
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

    ProcessContext voiceContext {
        context.sampleRate,
        std::span<float>(scratchLeft.data(), samples),
        std::span<float>(scratchRight.data(), samples)
    };

    for (auto& slot : slots)
        if (auto* engine = slot.get())
            engine->render(voiceContext);

    filters.process(voiceContext.left, voiceContext.right);

    for (std::size_t i = 0; i < samples; ++i)
    {
        const auto gain = ampEnvelope.nextSample();
        context.left[i] += scratchLeft[i] * gain;
        context.right[i] += scratchRight[i] * gain;
    }

    if (!ampEnvelope.isActive())
    {
        stopEngines();
        active = false;
        releasing = false;
        currentNote = -1;
    }
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
