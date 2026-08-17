#pragma once
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Physics/Physics2D.h"
#include <functional>
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

        // Box2D v3 ne fait pas de callback C direct : les events de contact sont
        // bufferisés pendant le Step() et lus juste après. OnUpdate() les traduit en
        // Entity et déclenche ces deux hooks — laissés vides par défaut (pas de coût si
        // personne ne s'y abonne).
        std::function<void(Entity, Entity)> OnCollisionBegin;
        std::function<void(Entity, Entity)> OnCollisionEnd;

    private:
        std::unique_ptr<Physics2D> m_Physics;
    };
}
