#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace tryst::tuning
{
class TuningTable
{
public:
    static constexpr std::size_t midiNotes = 128;

    void resetEqualTemperament() noexcept;
    [[nodiscard]] bool setOffsetSemitones(std::int32_t midiNote, float semitones) noexcept;
    [[nodiscard]] float offsetSemitones(std::int32_t midiNote) const noexcept;

    // Repeats one octave of offsets across the MIDI range. Values are offsets
    // from 12-TET in semitones, indexed from C through B.
    [[nodiscard]] bool setOctaveOffsets(std::span<const float> offsets) noexcept;

    [[nodiscard]] const std::array<float, midiNotes>& offsets() const noexcept { return noteOffsets; }

private:
    std::array<float, midiNotes> noteOffsets {};
};
}
