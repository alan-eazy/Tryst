#pragma once

#include "Core/ParameterRegistry.h"
#include "Modulation/ModTarget.h"
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tryst::plugin
{
enum class HostParameterKind : std::uint8_t
{
    patchParameter,
    macro,
    morphX,
    morphY
};

struct HostParameterInfo
{
    std::uint32_t id = 0;
    HostParameterKind kind = HostParameterKind::patchParameter;
    std::string persistentId;
    std::string name;
    tryst::mod::TargetDomain domain = tryst::mod::TargetDomain::global;
    std::uint8_t slot = 0;
    tryst::params::DestinationId destination = 0;
    float defaultNormalized = 0.0f;
    bool stepped = false;
};

class HostParameterCatalog
{
public:
    HostParameterCatalog();

    [[nodiscard]] std::span<const HostParameterInfo> all() const noexcept { return parameters; }
    [[nodiscard]] const HostParameterInfo* find(std::uint32_t id) const noexcept;
    [[nodiscard]] const HostParameterInfo* find(std::string_view persistentId) const noexcept;
    [[nodiscard]] bool valid() const noexcept { return catalogValid; }

    [[nodiscard]] static std::uint32_t stableId(std::string_view persistentId) noexcept;

private:
    void add(HostParameterInfo info);
    std::vector<HostParameterInfo> parameters;
    bool catalogValid = true;
};
}
