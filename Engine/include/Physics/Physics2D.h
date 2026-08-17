#pragma once
#include <box2d/box2d.h>
#include <glm/glm.hpp>

namespace Engine
{
    // Wrapper minimal autour d'un monde Box2D v3. La v3 expose une API en C par
    // handles (b2WorldId, b2BodyId...) plutôt que des classes C++ comme la v2 —
    // pas de pointeurs à gérer, les handles sont copiables et invalidés par Box2D
    // lui-même via b2Destroy*.
    class Physics2D
    {
    public:
        explicit Physics2D(const glm::vec2 &gravity = {0.0f, -9.8f});
        ~Physics2D();

        Physics2D(const Physics2D &) = delete;
        Physics2D &operator=(const Physics2D &) = delete;

        void Step(float timestep, int subStepCount = 4);

        b2WorldId GetWorldId() const { return m_WorldId; }

    private:
        b2WorldId m_WorldId;
    };
}
