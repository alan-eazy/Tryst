#pragma once
#include "Effect.h"

namespace tryst::fx
{
class DriveEffect final : public Effect
{
public:
    [[nodiscard]] EffectKind kind() const noexcept override { return EffectKind::drive; }
    [[nodiscard]] std::string_view name() const noexcept override { return "Drive"; }
    void prepare(double, std::uint32_t) override {}
    void reset() noexcept override {}
    void process(std::span<float> left, std::span<float> right) noexcept override;
    [[nodiscard]] bool setParameter(tryst::params::DestinationId destination, float normalized) noexcept override;

private:
    float mix = 1.0f;
    float amount = 0.25f;
};
}
