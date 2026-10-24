#pragma once

#include "DSP/VoiceManager.h"
#include "HostParameterCatalog.h"
#include "State/PatchState.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace tryst::plugin
{
enum class HostEventType : std::uint8_t
{
    noteOn,
    noteOff,
    noteExpression,
    parameter
};

struct HostEvent
{
    std::uint32_t frame = 0;
    HostEventType type = HostEventType::noteOn;
    std::int32_t key = 60;
    std::int32_t noteId = -1;
    float velocity = 1.0f;
    float pressure = 0.0f;
    float timbre = 0.0f;
    float pitchBendSemitones = 0.0f;
    std::uint32_t parameterId = 0;
    float normalizedValue = 0.0f;
};

class AtomicFloatStore
{
public:
    class Reference
    {
    public:
        explicit Reference(std::atomic<float>* value) noexcept : storage(value) {}
        Reference& operator=(float value) noexcept
        {
            storage->store(value, std::memory_order_relaxed);
            return *this;
        }
        operator float() const noexcept
        {
            return storage->load(std::memory_order_relaxed);
        }

    private:
        std::atomic<float>* storage = nullptr;
    };

    void reserve(std::size_t capacity)
    {
        storage = std::make_unique<std::atomic<float>[]>(capacity);
        capacityCount = capacity;
        valueCount = 0;
    }

    void push_back(float value) noexcept
    {
        if (valueCount >= capacityCount)
            return;
        storage[valueCount++].store(value, std::memory_order_relaxed);
    }

    [[nodiscard]] std::size_t size() const noexcept { return valueCount; }
    [[nodiscard]] Reference operator[](std::size_t index) noexcept { return Reference(&storage[index]); }
    [[nodiscard]] float operator[](std::size_t index) const noexcept
    {
        return storage[index].load(std::memory_order_relaxed);
    }

private:
    std::unique_ptr<std::atomic<float>[]> storage;
    std::size_t capacityCount = 0;
    std::size_t valueCount = 0;
};

class PluginEngine
{
public:
    PluginEngine();

    [[nodiscard]] bool prepare(double sampleRate, std::uint32_t maxBlockSize);
    void reset() noexcept;

    // Events must be sorted by frame. Processing is sample-accurate at event boundaries.
    [[nodiscard]] bool process(std::span<float> left,
                               std::span<float> right,
                               std::span<const HostEvent> events) noexcept;

    // Real-time safe after construction: updates fixed preallocated runtime storage only.
    [[nodiscard]] bool setHostParameter(std::uint32_t parameterId, float normalized) noexcept;
    [[nodiscard]] float hostParameterValue(std::uint32_t parameterId) const noexcept;

    // Topology/state operations are control-thread operations and may rebuild DSP objects.
    [[nodiscard]] bool setGeneratorKind(std::size_t slot, tryst::dsp::EngineKind kind, bool enabled = true);
    [[nodiscard]] bool setEffectKind(std::size_t slot, tryst::fx::EffectKind kind, bool enabled = true);
    [[nodiscard]] bool setEffectOrder(std::span<const std::uint8_t> slotOrder);
    [[nodiscard]] bool setTuningOffsetSemitones(std::int32_t note, float semitones);

    [[nodiscard]] std::string saveState() const;
    [[nodiscard]] bool loadState(std::string_view text);

    [[nodiscard]] const HostParameterCatalog& parameterCatalog() const noexcept { return catalog; }
    [[nodiscard]] const tryst::state::PatchState& patchState() const noexcept { return patch; }
    [[nodiscard]] bool isPrepared() const noexcept { return prepared; }
    [[nodiscard]] std::size_t activeVoiceCount() const noexcept { return manager.activeVoiceCount(); }

private:
    [[nodiscard]] bool rebuildManager();
    [[nodiscard]] bool applyHostParameter(const HostParameterInfo& info, float normalized) noexcept;
    [[nodiscard]] std::size_t catalogIndex(const HostParameterInfo& info) const noexcept;
    [[nodiscard]] tryst::state::PatchState makeSerializablePatch() const;
    void syncHostValuesFromPatch(const tryst::state::PatchState& source) noexcept;
    [[nodiscard]] bool parameterAppliesToPatch(const HostParameterInfo& info) const noexcept;
    void dispatchEvent(const HostEvent& event) noexcept;
    void renderRange(std::span<float> left, std::span<float> right,
                     std::size_t offset, std::size_t count) noexcept;

    HostParameterCatalog catalog;
    AtomicFloatStore hostValues;
    tryst::state::PatchState patch;
    tryst::dsp::VoiceManager manager;
    double preparedSampleRate = 48000.0;
    std::uint32_t preparedBlockSize = 0;
    bool prepared = false;
};
}
