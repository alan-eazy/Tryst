#pragma once
#include "EffectFactory.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace tryst::fx
{
class FxRack
{
public:
    static constexpr std::size_t maxSlots = 4;

    FxRack() noexcept;
    void prepare(double sampleRate, std::uint32_t maxBlockSize);
    void reset() noexcept;
    [[nodiscard]] bool setEffectKind(std::size_t slot, EffectKind kind);
    void clearEffect(std::size_t slot) noexcept;
    // Reorders processing positions while preserving slot identity/automation.
    [[nodiscard]] bool swapSlots(std::size_t positionA, std::size_t positionB) noexcept;
    [[nodiscard]] bool setOrder(std::span<const std::uint8_t> slotOrder) noexcept;
    [[nodiscard]] bool setParameter(std::size_t slot, tryst::params::DestinationId destination, float normalized) noexcept;
    void process(std::span<float> left, std::span<float> right) noexcept;
    [[nodiscard]] const Effect* effect(std::size_t slot) const noexcept;
    [[nodiscard]] std::size_t processingSlot(std::size_t position) const noexcept;

private:
    std::array<std::unique_ptr<Effect>, maxSlots> slots;
    std::array<std::uint8_t, maxSlots> order {};
    double sampleRate = 48000.0;
    std::uint32_t maxBlockSize = 0;
    bool prepared = false;
};
}
