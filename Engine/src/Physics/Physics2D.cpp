#include "Physics/Physics2D.h"

namespace Engine
{
    Physics2D::Physics2D(const glm::vec2 &gravity)
    {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {gravity.x, gravity.y};
        m_WorldId = b2CreateWorld(&worldDef);
    }

    Physics2D::~Physics2D()
    {
        b2DestroyWorld(m_WorldId);
    }

    void Physics2D::Step(float timestep, int subStepCount)
    {
        b2World_Step(m_WorldId, timestep, subStepCount);
    }
}
