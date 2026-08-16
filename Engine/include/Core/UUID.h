#pragma once
#include <cstdint>
#include <functional>

namespace Engine
{
    // Identifiant unique et stable pour une entité (indépendant de l'entt::entity interne,
    // qui lui est recyclé et ne survit pas à une sauvegarde/rechargement de scène).
    class UUID
    {
    public:
        UUID();
        UUID(uint64_t uuid);
        UUID(const UUID &) = default;

        operator uint64_t() const { return m_UUID; }

    private:
        uint64_t m_UUID;
    };
}

namespace std
{
    template <>
    struct hash<Engine::UUID>
    {
        std::size_t operator()(const Engine::UUID &uuid) const
        {
            return (uint64_t)uuid;
        }
    };
}
