#include "HostParameterCatalog.h"
#include <algorithm>
#include <array>
#include <limits>

namespace tryst::plugin
{
namespace
{
bool startsWith(std::string_view text, std::string_view prefix) noexcept
{
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

bool isStepped(std::string_view id) noexcept
{
    return id.ends_with(".enabled") || id.ends_with(".reverse")
        || id == "filter.routing" || id.ends_with(".mode");
}
}

HostParameterCatalog::HostParameterCatalog()
{
    parameters.reserve(128);

    for (std::size_t i = 0; i < 8; ++i)
    {
        const auto persistent = std::string("performance.macro.") + std::to_string(i + 1);
        add({ stableId(persistent), HostParameterKind::macro, persistent,
              std::string("Macro ") + std::to_string(i + 1), tryst::mod::TargetDomain::global,
              static_cast<std::uint8_t>(i), 0, 0.0f, false });
    }

    add({ stableId("performance.morph.x"), HostParameterKind::morphX, "performance.morph.x",
          "Morph X", tryst::mod::TargetDomain::global, 0, 0, 0.0f, false });
    add({ stableId("performance.morph.y"), HostParameterKind::morphY, "performance.morph.y",
          "Morph Y", tryst::mod::TargetDomain::global, 0, 0, 0.0f, false });

    for (const auto& descriptor : tryst::params::ParameterRegistry::all())
    {
        if (startsWith(descriptor.id, "generator.") || startsWith(descriptor.id, "engine."))
        {
            for (std::size_t slot = 0; slot < 4; ++slot)
            {
                const auto persistent = std::string("generator.") + std::to_string(slot + 1) + "." + std::string(descriptor.id);
                add({ stableId(persistent), HostParameterKind::patchParameter, persistent,
                      std::string("Generator ") + std::to_string(slot + 1) + " / " + std::string(descriptor.name),
                      tryst::mod::TargetDomain::generator, static_cast<std::uint8_t>(slot), descriptor.destination,
                      descriptor.defaultNormalized, isStepped(descriptor.id) });
            }
        }
        else if (startsWith(descriptor.id, "filter."))
        {
            const auto persistent = std::string(descriptor.id);
            add({ stableId(persistent), HostParameterKind::patchParameter, persistent, std::string(descriptor.name),
                  tryst::mod::TargetDomain::filter, 0, descriptor.destination,
                  descriptor.defaultNormalized, isStepped(descriptor.id) });
        }
        else if (startsWith(descriptor.id, "fx."))
        {
            for (std::size_t slot = 0; slot < 4; ++slot)
            {
                const auto persistent = std::string("effect.") + std::to_string(slot + 1) + "." + std::string(descriptor.id);
                add({ stableId(persistent), HostParameterKind::patchParameter, persistent,
                      std::string("FX ") + std::to_string(slot + 1) + " / " + std::string(descriptor.name),
                      tryst::mod::TargetDomain::effect, static_cast<std::uint8_t>(slot), descriptor.destination,
                      descriptor.defaultNormalized, isStepped(descriptor.id) });
            }
        }
    }
}

const HostParameterInfo* HostParameterCatalog::find(std::uint32_t id) const noexcept
{
    for (const auto& parameter : parameters)
        if (parameter.id == id)
            return &parameter;
    return nullptr;
}

const HostParameterInfo* HostParameterCatalog::find(std::string_view persistentId) const noexcept
{
    for (const auto& parameter : parameters)
        if (parameter.persistentId == persistentId)
            return &parameter;
    return nullptr;
}

std::uint32_t HostParameterCatalog::stableId(std::string_view persistentId) noexcept
{
    std::uint32_t hash = 2166136261u;
    for (const auto character : persistentId)
    {
        hash ^= static_cast<std::uint8_t>(character);
        hash *= 16777619u;
    }
    return hash;
}

void HostParameterCatalog::add(HostParameterInfo info)
{
    constexpr auto invalidId = std::numeric_limits<std::uint32_t>::max();
    if (info.id == invalidId
        || std::any_of(parameters.begin(), parameters.end(), [&](const auto& existing) {
            return existing.id == info.id || existing.persistentId == info.persistentId;
        }))
    {
        catalogValid = false;
        return;
    }
    parameters.push_back(std::move(info));
}
}
