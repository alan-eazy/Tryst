#include "TuningTable.h"
#include <algorithm>
#include <cmath>

namespace tryst::tuning
{
void TuningTable::resetEqualTemperament() noexcept
{
    noteOffsets.fill(0.0f);
}

bool TuningTable::setOffsetSemitones(std::int32_t midiNote, float semitones) noexcept
{
    if (midiNote < 0 || midiNote >= static_cast<std::int32_t>(noteOffsets.size())
        || !std::isfinite(semitones) || semitones < -48.0f || semitones > 48.0f)
        return false;

    noteOffsets[static_cast<std::size_t>(midiNote)] = semitones;
    return true;
}

float TuningTable::offsetSemitones(std::int32_t midiNote) const noexcept
{
    if (midiNote < 0 || midiNote >= static_cast<std::int32_t>(noteOffsets.size()))
        return 0.0f;
    return noteOffsets[static_cast<std::size_t>(midiNote)];
}

bool TuningTable::setOctaveOffsets(std::span<const float> offsets) noexcept
{
    if (offsets.size() != 12)
        return false;

    for (const auto value : offsets)
        if (!std::isfinite(value) || value < -1.0f || value > 1.0f)
            return false;

    for (std::size_t note = 0; note < noteOffsets.size(); ++note)
        noteOffsets[note] = offsets[note % 12];
    return true;
}
}
