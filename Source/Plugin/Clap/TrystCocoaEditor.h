#pragma once

#include <cstdint>

namespace tryst::plugin
{
class PluginEngine;
}

namespace tryst::plugin::cocoa
{
constexpr std::uint32_t editorWidth = 900;
constexpr std::uint32_t editorHeight = 580;

struct Editor;
using ParameterChangedCallback = void (*)(void* context, std::uint32_t parameterId, float normalized) noexcept;

[[nodiscard]] Editor* createEditor(PluginEngine& engine,
                                   void* context,
                                   ParameterChangedCallback callback) noexcept;
void destroyEditor(Editor* editor) noexcept;
[[nodiscard]] bool setParent(Editor* editor, void* nsView) noexcept;
[[nodiscard]] bool show(Editor* editor) noexcept;
[[nodiscard]] bool hide(Editor* editor) noexcept;
void refresh(Editor* editor) noexcept;
}
