#pragma once

#include "StateVariableFilter.h"
#include "../Core/ParameterRegistry.h"
#include <span>

namespace tryst::dsp
{
enum class FilterRouting
{
    serial,
    parallel
};

class FilterChain
{
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    [[nodiscard]] bool setParameter(tryst::params::DestinationId destination, float normalized) noexcept;
    void process(std::span<float> left, std::span<float> right) noexcept;

private:
    StateVariableFilter filter1;
    StateVariableFilter filter2;
    FilterRouting routing = FilterRouting::serial;
    float parallelBlend = 0.5f;
};
}
