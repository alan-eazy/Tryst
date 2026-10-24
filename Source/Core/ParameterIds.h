#pragma once

#include <array>
#include <string_view>

namespace tryst::params
{
inline constexpr std::string_view masterGain = "master.gain";
inline constexpr std::string_view masterTune = "master.tune";
inline constexpr std::string_view morphX     = "performance.morph_x";
inline constexpr std::string_view morphY     = "performance.morph_y";

inline constexpr std::array<std::string_view, 8> macros {
    "macro.1", "macro.2", "macro.3", "macro.4",
    "macro.5", "macro.6", "macro.7", "macro.8"
};

// Parameter IDs are part of the preset compatibility contract.
// Never recycle or silently rename an ID after release.
}
