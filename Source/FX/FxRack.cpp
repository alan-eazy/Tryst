#include "FxRack.h"
#include <algorithm>

namespace tryst::fx
{
FxRack::FxRack() noexcept
{
    for (std::size_t i = 0; i < order.size(); ++i)
        order[i] = static_cast<std::uint8_t>(i);
}

void FxRack::prepare(double newSampleRate, std::uint32_t newMaxBlockSize)
{
    sampleRate = newSampleRate > 1.0 ? newSampleRate : 48000.0;
    maxBlockSize = std::max<std::uint32_t>(1, newMaxBlockSize);
    prepared = true;
    for (auto& slot : slots)
        if (slot)
            slot->prepare(sampleRate, maxBlockSize);
}

void FxRack::reset() noexcept
{
    for (auto& slot : slots)
        if (slot)
            slot->reset();
}

bool FxRack::setEffectKind(std::size_t slotIndex, EffectKind kind)
{
    if (slotIndex >= slots.size())
        return false;
    auto next = createEffect(kind);
    if (!next)
        return false;
    if (prepared)
        next->prepare(sampleRate, maxBlockSize);
    slots[slotIndex] = std::move(next);
    return true;
}

void FxRack::clearEffect(std::size_t slotIndex) noexcept
{
    if (slotIndex < slots.size())
        slots[slotIndex].reset();
}

bool FxRack::swapSlots(std::size_t positionA, std::size_t positionB) noexcept
{
    if (positionA >= order.size() || positionB >= order.size())
        return false;
    std::swap(order[positionA], order[positionB]);
    return true;
}

bool FxRack::setOrder(std::span<const std::uint8_t> slotOrder) noexcept
{
    if (slotOrder.size() != order.size())
        return false;
    std::array<bool, maxSlots> seen {};
    for (const auto slot : slotOrder)
    {
        if (slot >= maxSlots || seen[slot])
            return false;
        seen[slot] = true;
    }
    std::copy(slotOrder.begin(), slotOrder.end(), order.begin());
    return true;
}

bool FxRack::setParameter(std::size_t slotIndex, tryst::params::DestinationId destination, float normalized) noexcept
{
    if (slotIndex >= slots.size() || !slots[slotIndex])
        return false;
    return slots[slotIndex]->setParameter(destination, normalized);
}

void FxRack::process(std::span<float> left, std::span<float> right) noexcept
{
    for (const auto slotIndex : order)
        if (slots[slotIndex])
            slots[slotIndex]->process(left, right);
}

const Effect* FxRack::effect(std::size_t slotIndex) const noexcept
{
    return slotIndex < slots.size() ? slots[slotIndex].get() : nullptr;
}

std::size_t FxRack::processingSlot(std::size_t position) const noexcept
{
    return position < order.size() ? order[position] : order.size();
}
}
