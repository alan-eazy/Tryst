#pragma once

#include "PatchState.h"
#include "DSP/VoiceManager.h"

namespace tryst::state
{
class PatchApplier
{
public:
    [[nodiscard]] static bool apply(const PatchState& patch, tryst::dsp::VoiceManager& manager);
};
}
