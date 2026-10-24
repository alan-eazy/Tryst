#pragma once

#include "../Core/ParameterRegistry.h"
#include <cstdint>
#include <span>
#include <string_view>

namespace tryst::fx
{
enum class EffectKind : std::uint8_t
{
    drive,
    delay
};

class Effect
{
public:
    virtual ~Effect() = default;

    [[nodiscard]] virtual EffectKind kind() const noexcept = 0;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    virtual void prepare(double sampleRate, std::uint32_t maxBlockSize) = 0;
    virtual void reset() noexcept = 0;
    virtual void process(std::span<float> left, std::span<float> right) noexcept = 0;
    [[nodiscard]] virtual bool setParameter(tryst::params::DestinationId destination, float normalized) noexcept = 0;
};
}
