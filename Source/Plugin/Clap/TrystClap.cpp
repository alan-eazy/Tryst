#include <clap/clap.h>
#include <clapwrapper/vst3.h>
#include "Plugin/PluginEngine.h"
#include "TrystClapEntry.h"
#if defined(TRYST_HAS_COCOA_GUI)
#include "TrystCocoaEditor.h"
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <span>
#include <string>
#include <string_view>

#ifndef TRYST_PLUGIN_VERSION
#define TRYST_PLUGIN_VERSION "0.0.0-dev"
#endif

namespace
{
using tryst::plugin::HostEvent;
using tryst::plugin::HostEventType;
using tryst::plugin::PluginEngine;

constexpr std::size_t maxTrackedNotes = 16;
constexpr std::size_t maxTranslatedEvents = 2048;
constexpr std::size_t maxStateBytes = 8u * 1024u * 1024u;

#if defined(TRYST_HAS_COCOA_GUI)
constexpr std::uint32_t guiEventQueueCapacity = 1024;

enum class GuiParamEventKind : std::uint8_t
{
    gestureBegin,
    value,
    gestureEnd
};

struct GuiParamEvent
{
    GuiParamEventKind kind = GuiParamEventKind::value;
    std::uint32_t parameterId = 0;
    float normalized = 0.0f;
};
#endif

const char* const pluginFeatures[] = {
    CLAP_PLUGIN_FEATURE_INSTRUMENT,
    CLAP_PLUGIN_FEATURE_SYNTHESIZER,
    CLAP_PLUGIN_FEATURE_SAMPLER,
    CLAP_PLUGIN_FEATURE_GRANULAR,
    CLAP_PLUGIN_FEATURE_STEREO,
    nullptr
};

const clap_plugin_descriptor_t pluginDescriptor {
    CLAP_VERSION_INIT,
    "com.alan-eazy.tryst",
    "Tryst",
    "alan-eazy",
    "https://github.com/alan-eazy/Tryst",
    "",
    "https://github.com/alan-eazy/Tryst/issues",
    TRYST_PLUGIN_VERSION,
    "Polyphonic multi-engine synthesizer",
    pluginFeatures
};

struct ActiveNote
{
    bool active = false;
    std::int32_t hostNoteId = -1;
    std::int32_t internalNoteId = -1;
    std::int16_t port = -1;
    std::int16_t channel = -1;
    std::int16_t key = -1;
    float pressure = 0.0f;
    float timbre = 0.0f;
    float pitchBendSemitones = 0.0f;
    std::uint64_t age = 0;
};

struct TrystClapPlugin
{
    clap_plugin_t plugin {};
    const clap_host_t* host = nullptr;
    const clap_host_params_t* hostParams = nullptr;
    PluginEngine engine;
    std::array<ActiveNote, maxTrackedNotes> notes {};
    std::array<HostEvent, maxTranslatedEvents> translatedEvents {};
    std::size_t translatedEventCount = 0;
    std::int32_t nextInternalNoteId = 1;
    std::uint64_t noteAge = 0;
    std::uint32_t maxFrames = 0;
    bool active = false;
    bool processing = false;
    bool hasPendingState = false;
    std::string pendingState;
#if defined(TRYST_HAS_COCOA_GUI)
    tryst::plugin::cocoa::Editor* editor = nullptr;
    std::array<GuiParamEvent, guiEventQueueCapacity> guiEvents {};
    std::atomic<std::uint32_t> guiEventRead { 0 };
    std::atomic<std::uint32_t> guiEventWrite { 0 };
#endif
};

TrystClapPlugin* self(const clap_plugin_t* plugin) noexcept
{
    return static_cast<TrystClapPlugin*>(plugin->plugin_data);
}

float clamp01(double value) noexcept
{
    if (!std::isfinite(value))
        return 0.0f;
    return static_cast<float>(std::clamp(value, 0.0, 1.0));
}

bool addressMatches(const ActiveNote& note,
                    std::int16_t port,
                    std::int16_t channel,
                    std::int16_t key,
                    std::int32_t hostNoteId) noexcept
{
    if (!note.active)
        return false;
    if (port >= 0 && note.port != port)
        return false;
    if (channel >= 0 && note.channel != channel)
        return false;
    if (key >= 0 && note.key != key)
        return false;
    if (hostNoteId >= 0 && note.hostNoteId != hostNoteId)
        return false;
    return true;
}

std::int32_t allocateInternalNoteId(TrystClapPlugin& plug) noexcept
{
    for (std::size_t attempt = 0; attempt < maxTrackedNotes + 1; ++attempt)
    {
        if (plug.nextInternalNoteId <= 0 || plug.nextInternalNoteId == std::numeric_limits<std::int32_t>::max())
            plug.nextInternalNoteId = 1;
        const auto candidate = plug.nextInternalNoteId++;
        const auto used = std::any_of(plug.notes.begin(), plug.notes.end(), [candidate](const ActiveNote& note) {
            return note.active && note.internalNoteId == candidate;
        });
        if (!used)
            return candidate;
    }
    return 1;
}

ActiveNote& allocateNoteSlot(TrystClapPlugin& plug) noexcept
{
    if (auto it = std::find_if(plug.notes.begin(), plug.notes.end(), [](const ActiveNote& note) {
            return !note.active;
        }); it != plug.notes.end())
        return *it;

    return *std::min_element(plug.notes.begin(), plug.notes.end(), [](const ActiveNote& a, const ActiveNote& b) {
        return a.age < b.age;
    });
}

bool appendHostEvent(TrystClapPlugin& plug, HostEvent event) noexcept
{
    if (plug.translatedEventCount >= plug.translatedEvents.size())
        return false;
    plug.translatedEvents[plug.translatedEventCount++] = event;
    return true;
}

#if defined(TRYST_HAS_COCOA_GUI)
bool enqueueGuiParameterChange(TrystClapPlugin& plug,
                               std::uint32_t parameterId,
                               float normalized) noexcept
{
    const auto read = plug.guiEventRead.load(std::memory_order_acquire);
    auto write = plug.guiEventWrite.load(std::memory_order_relaxed);
    const auto used = write >= read
        ? write - read
        : guiEventQueueCapacity - (read - write);
    const auto freeSlots = guiEventQueueCapacity - 1u - used;
    if (freeSlots < 3u)
        return false;

    const std::array events {
        GuiParamEvent { GuiParamEventKind::gestureBegin, parameterId, normalized },
        GuiParamEvent { GuiParamEventKind::value, parameterId, normalized },
        GuiParamEvent { GuiParamEventKind::gestureEnd, parameterId, normalized }
    };

    for (const auto& event : events)
    {
        plug.guiEvents[write] = event;
        write = (write + 1u) % guiEventQueueCapacity;
    }
    plug.guiEventWrite.store(write, std::memory_order_release);
    return true;
}

bool popGuiEvent(TrystClapPlugin& plug, GuiParamEvent& event) noexcept
{
    auto read = plug.guiEventRead.load(std::memory_order_relaxed);
    const auto write = plug.guiEventWrite.load(std::memory_order_acquire);
    if (read == write)
        return false;

    event = plug.guiEvents[read];
    read = (read + 1u) % guiEventQueueCapacity;
    plug.guiEventRead.store(read, std::memory_order_release);
    return true;
}

void requestGuiParameterFlush(TrystClapPlugin& plug) noexcept
{
    if (plug.hostParams != nullptr && plug.hostParams->request_flush != nullptr)
        plug.hostParams->request_flush(plug.host);
    else if (plug.host != nullptr && plug.host->request_process != nullptr)
        plug.host->request_process(plug.host);
}

void guiParameterChanged(void* context,
                         std::uint32_t parameterId,
                         float normalized) noexcept
{
    auto* plug = static_cast<TrystClapPlugin*>(context);
    if (plug == nullptr || plug->engine.parameterCatalog().find(parameterId) == nullptr)
        return;

    if (enqueueGuiParameterChange(*plug, parameterId, clamp01(normalized)))
        requestGuiParameterFlush(*plug);
}

void emitGuiEventToHost(const clap_output_events_t* output,
                        const GuiParamEvent& event,
                        std::uint32_t frame) noexcept
{
    if (output == nullptr || output->try_push == nullptr)
        return;

    if (event.kind == GuiParamEventKind::value)
    {
        clap_event_param_value_t parameter {};
        parameter.header.size = sizeof(parameter);
        parameter.header.time = frame;
        parameter.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        parameter.header.type = CLAP_EVENT_PARAM_VALUE;
        parameter.header.flags = 0;
        parameter.param_id = event.parameterId;
        parameter.cookie = nullptr;
        parameter.note_id = -1;
        parameter.port_index = -1;
        parameter.channel = -1;
        parameter.key = -1;
        parameter.value = event.normalized;
        (void) output->try_push(output, &parameter.header);
        return;
    }

    clap_event_param_gesture_t gesture {};
    gesture.header.size = sizeof(gesture);
    gesture.header.time = frame;
    gesture.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    gesture.header.type = event.kind == GuiParamEventKind::gestureBegin
        ? CLAP_EVENT_PARAM_GESTURE_BEGIN
        : CLAP_EVENT_PARAM_GESTURE_END;
    gesture.header.flags = 0;
    gesture.param_id = event.parameterId;
    (void) output->try_push(output, &gesture.header);
}

bool drainGuiEventsForProcess(TrystClapPlugin& plug,
                              const clap_output_events_t* output) noexcept
{
    GuiParamEvent event;
    while (popGuiEvent(plug, event))
    {
        emitGuiEventToHost(output, event, 0);
        if (event.kind != GuiParamEventKind::value)
            continue;

        HostEvent translated;
        translated.frame = 0;
        translated.type = HostEventType::parameter;
        translated.parameterId = event.parameterId;
        translated.normalizedValue = event.normalized;
        if (!appendHostEvent(plug, translated))
            return false;
    }
    return true;
}

void drainGuiEventsForFlush(TrystClapPlugin& plug,
                            const clap_output_events_t* output) noexcept
{
    GuiParamEvent event;
    while (popGuiEvent(plug, event))
    {
        emitGuiEventToHost(output, event, 0);
        if (event.kind == GuiParamEventKind::value)
            (void) plug.engine.setHostParameter(event.parameterId, event.normalized);
    }
}
#endif

bool appendExpressionEvent(TrystClapPlugin& plug, const ActiveNote& note, std::uint32_t frame) noexcept
{
    HostEvent translated;
    translated.frame = frame;
    translated.type = HostEventType::noteExpression;
    translated.key = note.key;
    translated.noteId = note.internalNoteId;
    translated.pressure = note.pressure;
    translated.timbre = note.timbre;
    translated.pitchBendSemitones = note.pitchBendSemitones;
    return appendHostEvent(plug, translated);
}

bool translateNoteOn(TrystClapPlugin& plug, const clap_event_note_t& event) noexcept
{
    if (event.port_index < 0 || event.channel < 0 || event.key < 0 || event.key > 127)
        return true;

    // A host-provided note id identifies a unique CLAP voice. If the host does
    // not provide ids, repeated same-key note-ons remain distinct internally.
    if (event.note_id >= 0)
    {
        for (auto& existing : plug.notes)
        {
            if (addressMatches(existing, event.port_index, event.channel, event.key, event.note_id))
                existing.active = false;
        }
    }

    auto& note = allocateNoteSlot(plug);
    note = {};
    note.active = true;
    note.hostNoteId = event.note_id;
    note.internalNoteId = allocateInternalNoteId(plug);
    note.port = event.port_index;
    note.channel = event.channel;
    note.key = event.key;
    note.age = ++plug.noteAge;

    HostEvent translated;
    translated.frame = event.header.time;
    translated.type = HostEventType::noteOn;
    translated.key = event.key;
    translated.noteId = note.internalNoteId;
    translated.velocity = clamp01(event.velocity);
    return appendHostEvent(plug, translated);
}

bool translateNoteOff(TrystClapPlugin& plug, const clap_event_note_t& event) noexcept
{
    bool success = true;
    for (auto& note : plug.notes)
    {
        if (!addressMatches(note, event.port_index, event.channel, event.key, event.note_id))
            continue;

        HostEvent translated;
        translated.frame = event.header.time;
        translated.type = HostEventType::noteOff;
        translated.key = note.key;
        translated.noteId = note.internalNoteId;
        translated.velocity = clamp01(event.velocity);
        success = appendHostEvent(plug, translated) && success;
        note.active = false;
    }
    return success;
}

bool translateExpression(TrystClapPlugin& plug, const clap_event_note_expression_t& event) noexcept
{
    bool success = true;
    for (auto& note : plug.notes)
    {
        if (!addressMatches(note, event.port_index, event.channel, event.key, event.note_id))
            continue;

        switch (event.expression_id)
        {
            case CLAP_NOTE_EXPRESSION_TUNING:
                note.pitchBendSemitones = static_cast<float>(std::clamp(event.value, -48.0, 48.0));
                break;
            case CLAP_NOTE_EXPRESSION_BRIGHTNESS:
                note.timbre = clamp01(event.value);
                break;
            case CLAP_NOTE_EXPRESSION_PRESSURE:
                note.pressure = clamp01(event.value);
                break;
            default:
                continue;
        }
        success = appendExpressionEvent(plug, note, event.header.time) && success;
    }
    return success;
}

bool translateParameter(TrystClapPlugin& plug, const clap_event_param_value_t& event) noexcept
{
    if (plug.engine.parameterCatalog().find(event.param_id) == nullptr)
        return true;

    HostEvent translated;
    translated.frame = event.header.time;
    translated.type = HostEventType::parameter;
    translated.parameterId = event.param_id;
    translated.normalizedValue = clamp01(event.value);
    return appendHostEvent(plug, translated);
}

bool translateEvents(TrystClapPlugin& plug, const clap_input_events_t* input, std::uint32_t frames) noexcept
{
    if (input == nullptr)
        return true;

    const auto count = input->size(input);
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const auto* header = input->get(input, index);
        if (header == nullptr || header->space_id != CLAP_CORE_EVENT_SPACE_ID)
            continue;
        if (header->time > frames)
            return false;

        bool success = true;
        switch (header->type)
        {
            case CLAP_EVENT_NOTE_ON:
                success = translateNoteOn(plug, *reinterpret_cast<const clap_event_note_t*>(header));
                break;
            case CLAP_EVENT_NOTE_OFF:
            case CLAP_EVENT_NOTE_CHOKE:
                // Until the core exposes an immediate choke event, choke follows the normal release path.
                success = translateNoteOff(plug, *reinterpret_cast<const clap_event_note_t*>(header));
                break;
            case CLAP_EVENT_NOTE_EXPRESSION:
                success = translateExpression(plug, *reinterpret_cast<const clap_event_note_expression_t*>(header));
                break;
            case CLAP_EVENT_PARAM_VALUE:
                success = translateParameter(plug, *reinterpret_cast<const clap_event_param_value_t*>(header));
                break;
            default:
                break;
        }
        if (!success)
            return false;
    }
    return true;
}

bool writeState(const clap_ostream_t* stream, std::string_view text) noexcept
{
    if (stream == nullptr || stream->write == nullptr)
        return false;
    std::size_t offset = 0;
    while (offset < text.size())
    {
        const auto written = stream->write(stream, text.data() + offset, text.size() - offset);
        if (written <= 0)
            return false;
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

bool readState(const clap_istream_t* stream, std::string& text)
{
    if (stream == nullptr || stream->read == nullptr)
        return false;

    text.clear();
    std::array<char, 4096> buffer {};
    for (;;)
    {
        const auto read = stream->read(stream, buffer.data(), buffer.size());
        if (read < 0)
            return false;
        if (read == 0)
            break;
        if (text.size() + static_cast<std::size_t>(read) > maxStateBytes)
            return false;
        text.append(buffer.data(), static_cast<std::size_t>(read));
    }
    return !text.empty();
}

void notifyParameterValuesChanged(TrystClapPlugin& plug) noexcept
{
    if (plug.hostParams != nullptr)
        plug.hostParams->rescan(plug.host, CLAP_PARAM_RESCAN_VALUES | CLAP_PARAM_RESCAN_TEXT);
}

bool applyState(TrystClapPlugin& plug, std::string_view text)
{
    if (!plug.engine.loadState(text))
        return false;
    notifyParameterValuesChanged(plug);
#if defined(TRYST_HAS_COCOA_GUI)
    tryst::plugin::cocoa::refresh(plug.editor);
#endif
    return true;
}

bool pluginInit(const clap_plugin_t* plugin)
{
    auto& plug = *self(plugin);
    if (plug.host != nullptr && plug.host->get_extension != nullptr)
        plug.hostParams = static_cast<const clap_host_params_t*>(plug.host->get_extension(plug.host, CLAP_EXT_PARAMS));
    return plug.engine.parameterCatalog().valid();
}

void pluginDestroy(const clap_plugin_t* plugin)
{
    auto* plug = self(plugin);
#if defined(TRYST_HAS_COCOA_GUI)
    tryst::plugin::cocoa::destroyEditor(plug->editor);
    plug->editor = nullptr;
#endif
    delete plug;
}

bool pluginActivate(const clap_plugin_t* plugin,
                    double sampleRate,
                    std::uint32_t,
                    std::uint32_t maxFrames)
{
    auto& plug = *self(plugin);
    if (maxFrames == 0)
        return false;

    if (plug.hasPendingState)
    {
        if (!plug.engine.loadState(plug.pendingState))
            return false;
        plug.pendingState.clear();
        plug.hasPendingState = false;
    }

    if (!plug.engine.prepare(sampleRate, maxFrames))
        return false;

    plug.maxFrames = maxFrames;
    plug.active = true;
    for (auto& note : plug.notes)
        note = {};
    return true;
}

void pluginDeactivate(const clap_plugin_t* plugin)
{
    auto& plug = *self(plugin);
    plug.processing = false;
    plug.active = false;
    plug.engine.reset();
    for (auto& note : plug.notes)
        note = {};

    if (plug.hasPendingState)
    {
        if (plug.engine.loadState(plug.pendingState))
            notifyParameterValuesChanged(plug);
        plug.pendingState.clear();
        plug.hasPendingState = false;
#if defined(TRYST_HAS_COCOA_GUI)
        tryst::plugin::cocoa::refresh(plug.editor);
#endif
    }
}

bool pluginStartProcessing(const clap_plugin_t* plugin)
{
    auto& plug = *self(plugin);
    plug.processing = plug.active;
    return plug.processing;
}

void pluginStopProcessing(const clap_plugin_t* plugin)
{
    self(plugin)->processing = false;
}

void pluginReset(const clap_plugin_t* plugin)
{
    auto& plug = *self(plugin);
    plug.engine.reset();
    for (auto& note : plug.notes)
        note = {};
}

clap_process_status pluginProcess(const clap_plugin_t* plugin, const clap_process_t* process)
{
    auto& plug = *self(plugin);
    if (!plug.active || !plug.processing || process == nullptr || process->frames_count > plug.maxFrames)
        return CLAP_PROCESS_ERROR;
    if (process->audio_outputs_count < 1 || process->audio_outputs == nullptr)
        return CLAP_PROCESS_ERROR;

    auto& output = process->audio_outputs[0];
    if (output.channel_count < 2 || output.data32 == nullptr || output.data32[0] == nullptr || output.data32[1] == nullptr)
        return CLAP_PROCESS_ERROR;

    plug.translatedEventCount = 0;
#if defined(TRYST_HAS_COCOA_GUI)
    if (!drainGuiEventsForProcess(plug, process->out_events))
        return CLAP_PROCESS_ERROR;
#endif
    if (!translateEvents(plug, process->in_events, process->frames_count))
        return CLAP_PROCESS_ERROR;

    const std::span<float> left(output.data32[0], process->frames_count);
    const std::span<float> right(output.data32[1], process->frames_count);
    const std::span<const HostEvent> events(plug.translatedEvents.data(), plug.translatedEventCount);
    return plug.engine.process(left, right, events) ? CLAP_PROCESS_CONTINUE : CLAP_PROCESS_ERROR;
}

std::uint32_t audioPortsCount(const clap_plugin_t*, bool isInput)
{
    return isInput ? 0u : 1u;
}

bool audioPortsGet(const clap_plugin_t*, std::uint32_t index, bool isInput, clap_audio_port_info_t* info)
{
    if (isInput || index != 0 || info == nullptr)
        return false;
    info->id = 0;
    std::snprintf(info->name, sizeof(info->name), "%s", "Main Output");
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = 2;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    return true;
}

const clap_plugin_audio_ports_t audioPortsExtension {
    audioPortsCount,
    audioPortsGet
};

std::uint32_t notePortsCount(const clap_plugin_t*, bool isInput)
{
    return isInput ? 1u : 0u;
}

bool notePortsGet(const clap_plugin_t*, std::uint32_t index, bool isInput, clap_note_port_info_t* info)
{
    if (!isInput || index != 0 || info == nullptr)
        return false;
    info->id = 0;
    info->supported_dialects = CLAP_NOTE_DIALECT_CLAP;
    info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
    std::snprintf(info->name, sizeof(info->name), "%s", "Note Input");
    return true;
}

const clap_plugin_note_ports_t notePortsExtension {
    notePortsCount,
    notePortsGet
};

std::uint32_t paramsCount(const clap_plugin_t* plugin)
{
    return static_cast<std::uint32_t>(self(plugin)->engine.parameterCatalog().all().size());
}

bool paramsGetInfo(const clap_plugin_t* plugin, std::uint32_t index, clap_param_info_t* info)
{
    if (info == nullptr)
        return false;
    const auto parameters = self(plugin)->engine.parameterCatalog().all();
    if (index >= parameters.size())
        return false;

    const auto& parameter = parameters[index];
    *info = {};
    info->id = parameter.id;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_REQUIRES_PROCESS;
    if (parameter.stepped)
        info->flags |= CLAP_PARAM_IS_STEPPED;
    std::snprintf(info->name, sizeof(info->name), "%s", parameter.name.c_str());
    std::snprintf(info->module, sizeof(info->module), "%s", "Tryst");
    info->min_value = 0.0;
    info->max_value = 1.0;
    info->default_value = parameter.defaultNormalized;
    return true;
}

bool paramsGetValue(const clap_plugin_t* plugin, clap_id paramId, double* value)
{
    if (value == nullptr || self(plugin)->engine.parameterCatalog().find(paramId) == nullptr)
        return false;
    *value = self(plugin)->engine.hostParameterValue(paramId);
    return true;
}

bool paramsValueToText(const clap_plugin_t* plugin,
                       clap_id paramId,
                       double value,
                       char* output,
                       std::uint32_t capacity)
{
    if (output == nullptr || capacity == 0)
        return false;
    const auto* parameter = self(plugin)->engine.parameterCatalog().find(paramId);
    if (parameter == nullptr)
        return false;
    if (parameter->stepped)
        std::snprintf(output, capacity, "%s", value >= 0.5 ? "On" : "Off");
    else
        std::snprintf(output, capacity, "%.3f", std::clamp(value, 0.0, 1.0));
    return true;
}

bool paramsTextToValue(const clap_plugin_t* plugin, clap_id paramId, const char* text, double* value)
{
    if (text == nullptr || value == nullptr)
        return false;
    const auto* parameter = self(plugin)->engine.parameterCatalog().find(paramId);
    if (parameter == nullptr)
        return false;

    if (parameter->stepped)
    {
        if (std::strcmp(text, "On") == 0 || std::strcmp(text, "on") == 0 || std::strcmp(text, "1") == 0)
        {
            *value = 1.0;
            return true;
        }
        if (std::strcmp(text, "Off") == 0 || std::strcmp(text, "off") == 0 || std::strcmp(text, "0") == 0)
        {
            *value = 0.0;
            return true;
        }
    }

    errno = 0;
    char* parseEnd = nullptr;
    const auto parsed = std::strtod(text, &parseEnd);
    if (parseEnd == text || parseEnd == nullptr || *parseEnd != '\0' || errno == ERANGE || !std::isfinite(parsed))
        return false;
    *value = std::clamp(parsed, 0.0, 1.0);
    return true;
}

void paramsFlush(const clap_plugin_t* plugin,
                 const clap_input_events_t* input,
                 const clap_output_events_t* output)
{
    auto& plug = *self(plugin);
#if defined(TRYST_HAS_COCOA_GUI)
    drainGuiEventsForFlush(plug, output);
#else
    (void)output;
#endif
    if (input == nullptr)
        return;
    const auto count = input->size(input);
    for (std::uint32_t index = 0; index < count; ++index)
    {
        const auto* header = input->get(input, index);
        if (header == nullptr || header->space_id != CLAP_CORE_EVENT_SPACE_ID || header->type != CLAP_EVENT_PARAM_VALUE)
            continue;
        const auto& event = *reinterpret_cast<const clap_event_param_value_t*>(header);
        (void) plug.engine.setHostParameter(event.param_id, clamp01(event.value));
    }
}

const clap_plugin_params_t paramsExtension {
    paramsCount,
    paramsGetInfo,
    paramsGetValue,
    paramsValueToText,
    paramsTextToValue,
    paramsFlush
};

bool stateSave(const clap_plugin_t* plugin, const clap_ostream_t* stream)
{
    const auto state = self(plugin)->engine.saveState();
    return writeState(stream, state);
}

bool stateLoad(const clap_plugin_t* plugin, const clap_istream_t* stream)
{
    auto& plug = *self(plugin);
    std::string state;
    if (!readState(stream, state))
        return false;

    if (plug.active)
    {
        plug.pendingState = std::move(state);
        plug.hasPendingState = true;
        if (plug.host != nullptr && plug.host->request_restart != nullptr)
            plug.host->request_restart(plug.host);
        return true;
    }

    return applyState(plug, state);
}

const clap_plugin_state_t stateExtension {
    stateSave,
    stateLoad
};

#if defined(TRYST_HAS_COCOA_GUI)
bool guiIsApiSupported(const clap_plugin_t*, const char* api, bool isFloating)
{
    return !isFloating && api != nullptr && std::strcmp(api, CLAP_WINDOW_API_COCOA) == 0;
}

bool guiGetPreferredApi(const clap_plugin_t*, const char** api, bool* isFloating)
{
    if (api == nullptr || isFloating == nullptr)
        return false;
    *api = CLAP_WINDOW_API_COCOA;
    *isFloating = false;
    return true;
}

bool guiCreate(const clap_plugin_t* plugin, const char* api, bool isFloating)
{
    auto& plug = *self(plugin);
    if (plug.editor != nullptr || !guiIsApiSupported(plugin, api, isFloating))
        return false;
    plug.editor = tryst::plugin::cocoa::createEditor(plug.engine, &plug, guiParameterChanged);
    return plug.editor != nullptr;
}

void guiDestroy(const clap_plugin_t* plugin)
{
    auto& plug = *self(plugin);
    tryst::plugin::cocoa::destroyEditor(plug.editor);
    plug.editor = nullptr;
}

bool guiSetScale(const clap_plugin_t*, double)
{
    // Cocoa uses logical points and the OS owns backing-scale conversion.
    return false;
}

bool guiGetSize(const clap_plugin_t* plugin, std::uint32_t* width, std::uint32_t* height)
{
    if (self(plugin)->editor == nullptr || width == nullptr || height == nullptr)
        return false;
    *width = tryst::plugin::cocoa::editorWidth;
    *height = tryst::plugin::cocoa::editorHeight;
    return true;
}

bool guiCanResize(const clap_plugin_t*)
{
    return false;
}

bool guiGetResizeHints(const clap_plugin_t*, clap_gui_resize_hints_t*)
{
    return false;
}

bool guiAdjustSize(const clap_plugin_t*, std::uint32_t* width, std::uint32_t* height)
{
    if (width == nullptr || height == nullptr)
        return false;
    *width = tryst::plugin::cocoa::editorWidth;
    *height = tryst::plugin::cocoa::editorHeight;
    return true;
}

bool guiSetSize(const clap_plugin_t*, std::uint32_t width, std::uint32_t height)
{
    return width == tryst::plugin::cocoa::editorWidth
        && height == tryst::plugin::cocoa::editorHeight;
}

bool guiSetParent(const clap_plugin_t* plugin, const clap_window_t* window)
{
    if (window == nullptr || window->api == nullptr || window->cocoa == nullptr
        || std::strcmp(window->api, CLAP_WINDOW_API_COCOA) != 0)
        return false;
    return tryst::plugin::cocoa::setParent(self(plugin)->editor, window->cocoa);
}

bool guiSetTransient(const clap_plugin_t*, const clap_window_t*)
{
    return false;
}

void guiSuggestTitle(const clap_plugin_t*, const char*) {}

bool guiShow(const clap_plugin_t* plugin)
{
    return tryst::plugin::cocoa::show(self(plugin)->editor);
}

bool guiHide(const clap_plugin_t* plugin)
{
    return tryst::plugin::cocoa::hide(self(plugin)->editor);
}

const clap_plugin_gui_t guiExtension {
    guiIsApiSupported,
    guiGetPreferredApi,
    guiCreate,
    guiDestroy,
    guiSetScale,
    guiGetSize,
    guiCanResize,
    guiGetResizeHints,
    guiAdjustSize,
    guiSetSize,
    guiSetParent,
    guiSetTransient,
    guiSuggestTitle,
    guiShow,
    guiHide
};
#endif

std::uint32_t vst3MidiChannels(const clap_plugin_t*, std::uint32_t)
{
    return 16;
}

std::uint32_t vst3SupportedNoteExpressions(const clap_plugin_t*)
{
    return AS_VST3_NOTE_EXPRESSION_TUNING
         | AS_VST3_NOTE_EXPRESSION_BRIGHTNESS
         | AS_VST3_NOTE_EXPRESSION_PRESSURE;
}

const clap_plugin_as_vst3_t vst3PluginExtension {
    vst3MidiChannels,
    vst3SupportedNoteExpressions
};

const void* pluginGetExtension(const clap_plugin_t*, const char* id)
{
    if (id == nullptr)
        return nullptr;
    if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
        return &audioPortsExtension;
    if (std::strcmp(id, CLAP_EXT_NOTE_PORTS) == 0)
        return &notePortsExtension;
    if (std::strcmp(id, CLAP_EXT_PARAMS) == 0)
        return &paramsExtension;
    if (std::strcmp(id, CLAP_EXT_STATE) == 0)
        return &stateExtension;
#if defined(TRYST_HAS_COCOA_GUI)
    if (std::strcmp(id, CLAP_EXT_GUI) == 0)
        return &guiExtension;
#endif
    if (std::strcmp(id, CLAP_PLUGIN_AS_VST3) == 0)
        return &vst3PluginExtension;
    return nullptr;
}

void pluginOnMainThread(const clap_plugin_t*) {}

const clap_plugin_t* createPlugin(const clap_plugin_factory_t*, const clap_host_t* host, const char* pluginId)
{
    if (host == nullptr || pluginId == nullptr || !clap_version_is_compatible(host->clap_version)
        || std::strcmp(pluginId, pluginDescriptor.id) != 0)
        return nullptr;

    auto* plug = new (std::nothrow) TrystClapPlugin;
    if (plug == nullptr)
        return nullptr;

    plug->host = host;
    plug->plugin.desc = &pluginDescriptor;
    plug->plugin.plugin_data = plug;
    plug->plugin.init = pluginInit;
    plug->plugin.destroy = pluginDestroy;
    plug->plugin.activate = pluginActivate;
    plug->plugin.deactivate = pluginDeactivate;
    plug->plugin.start_processing = pluginStartProcessing;
    plug->plugin.stop_processing = pluginStopProcessing;
    plug->plugin.reset = pluginReset;
    plug->plugin.process = pluginProcess;
    plug->plugin.get_extension = pluginGetExtension;
    plug->plugin.on_main_thread = pluginOnMainThread;
    return &plug->plugin;
}

std::uint32_t factoryPluginCount(const clap_plugin_factory_t*)
{
    return 1;
}

const clap_plugin_descriptor_t* factoryPluginDescriptor(const clap_plugin_factory_t*, std::uint32_t index)
{
    return index == 0 ? &pluginDescriptor : nullptr;
}

const clap_plugin_factory_t pluginFactory {
    factoryPluginCount,
    factoryPluginDescriptor,
    createPlugin
};

const clap_plugin_factory_as_vst3_t vst3FactoryInfo {
    "alan-eazy",
    "https://github.com/alan-eazy/Tryst",
    nullptr,
    nullptr,
    nullptr
};
}

extern "C" bool tryst_clap_entry_init(const char*)
{
    return true;
}

extern "C" void tryst_clap_entry_deinit() {}

extern "C" const void* tryst_clap_entry_get_factory(const char* factoryId)
{
    if (factoryId == nullptr)
        return nullptr;
    if (std::strcmp(factoryId, CLAP_PLUGIN_FACTORY_ID) == 0)
        return &pluginFactory;
    if (std::strcmp(factoryId, CLAP_PLUGIN_FACTORY_INFO_VST3) == 0
        || std::strcmp(factoryId, CLAP_PLUGIN_FACTORY_INFO_VST3_V1) == 0)
        return &vst3FactoryInfo;
    return nullptr;
}
