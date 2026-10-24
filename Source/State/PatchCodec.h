#pragma once

#include "PatchState.h"
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace tryst::state
{
struct PatchDecodeResult
{
    std::optional<PatchState> patch;
    std::size_t errorLine = 0;
    std::string error;

    [[nodiscard]] explicit operator bool() const noexcept { return patch.has_value(); }
};

class PatchCodec
{
public:
    [[nodiscard]] static std::string encode(const PatchState& patch);
    [[nodiscard]] static PatchDecodeResult decode(std::string_view text);
};
}
