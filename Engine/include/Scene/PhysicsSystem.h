#pragma once
#include "Scene/Scene.h"
#include "Physics/Physics2D.h"
#include <memory>

namespace Engine
{
    // Fait le pont entre les components RigidBody/BoxCollider de l'ECS et un monde
    // Physics2D : crée les corps/shapes Box2D au démarrage du Play (OnRuntimeStart),
    // les détruit au Stop (OnRuntimeStop), et resynchronise TransformComponent depuis
    // Box2D après chaque OnUpdate(). Un PhysicsSystem = une durée de vie de Play,
    // possédé par l'appelant (EditorLayer), pas un singleton statique comme RenderSystem.
    class PhysicsSystem
    {
    public:
        void OnRuntimeStart(Scene &scene);
        void OnRuntimeStop();
        void OnUpdate(Scene &scene, float timestep);

    private:
        std::unique_ptr<Physics2D> m_Physics;
    };
}
