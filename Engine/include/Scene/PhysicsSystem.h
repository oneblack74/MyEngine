#pragma once
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Physics/Physics2D.h"
#include <functional>
#include <memory>
#include <unordered_map>

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

        // Contrôle d'un corps depuis le gameplay. Le TransformComponent ne sert à rien
        // pour ça : c'est Box2D qui l'écrit à chaque pas, une valeur posée dedans serait
        // écrasée dans la foulée. Sans effet si l'entité n'a pas de corps (pas de
        // RigidBody, ou appel en dehors d'une partie).
        void SetLinearVelocity(Entity entity, const glm::vec2 &velocity);
        glm::vec2 GetLinearVelocity(Entity entity);
        void ApplyLinearImpulse(Entity entity, const glm::vec2 &impulse);
        // Téléporte le corps : la position est imposée, pas atteinte par la simulation.
        void SetPosition(Entity entity, const glm::vec2 &position);

        // Box2D v3 ne fait pas de callback C direct : les events de contact sont
        // bufferisés pendant le Step() et lus juste après. OnUpdate() les traduit en
        // Entity et déclenche ces deux hooks — laissés vides par défaut (pas de coût si
        // personne ne s'y abonne).
        std::function<void(Entity, Entity)> OnCollisionBegin;
        std::function<void(Entity, Entity)> OnCollisionEnd;

    private:
        // Crée les corps des entités apparues depuis le dernier pas, et détruit ceux
        // dont l'entité a disparu : un jeu fait naître et mourir des objets en pleine
        // partie, sans pour autant redémarrer le monde physique.
        void SyncBodiesWithScene(Scene &scene);

        std::unique_ptr<Physics2D> m_Physics;

        // Corps créés, par UUID d'entité. Les corps ne se retrouvent que par là une fois
        // l'entité supprimée : son RigidBodyComponent est parti avec elle.
        std::unordered_map<uint64_t, b2BodyId> m_Bodies;
    };
}
