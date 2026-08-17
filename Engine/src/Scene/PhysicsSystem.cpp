#include "Scene/PhysicsSystem.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"

namespace Engine
{
    static b2BodyType ToBox2DBodyType(RigidBodyComponent::BodyType type)
    {
        switch (type)
        {
            case RigidBodyComponent::BodyType::Static:    return b2_staticBody;
            case RigidBodyComponent::BodyType::Dynamic:   return b2_dynamicBody;
            case RigidBodyComponent::BodyType::Kinematic: return b2_kinematicBody;
        }
        return b2_staticBody;
    }

    // Box2D stocke un void* de user data par shape ; on y met directement le handle
    // EnTT de l'entité (encodé dans le pointeur, jamais déréférencé comme un vrai
    // pointeur), pour retrouver l'Entity propriétaire d'une shape dans les contact events.
    static void *EntityToUserData(entt::entity entityHandle)
    {
        return (void *)(uintptr_t)(uint32_t)entityHandle;
    }

    static Entity UserDataToEntity(Scene &scene, void *userData)
    {
        return Entity{(entt::entity)(uint32_t)(uintptr_t)userData, &scene};
    }

    void PhysicsSystem::OnRuntimeStart(Scene &scene)
    {
        m_Physics = std::make_unique<Physics2D>();

        auto view = scene.GetAllEntitiesWith<RigidBodyComponent>();
        for (auto entityHandle : view)
        {
            Entity entity{entityHandle, &scene};
            auto &transform = entity.GetComponent<TransformComponent>();
            auto &rb = entity.GetComponent<RigidBodyComponent>();

            b2BodyDef bodyDef = b2DefaultBodyDef();
            bodyDef.type = ToBox2DBodyType(rb.Type);
            bodyDef.position = {transform.Position.x, transform.Position.y};
            bodyDef.rotation = b2MakeRot(glm::radians(transform.Rotation));
            bodyDef.fixedRotation = rb.FixedRotation;

            rb.RuntimeBody = b2CreateBody(m_Physics->GetWorldId(), &bodyDef);

            if (entity.HasComponent<BoxColliderComponent>())
            {
                auto &bc = entity.GetComponent<BoxColliderComponent>();

                // Le collider suit l'échelle de l'entité au moment du Play (pas de resize
                // dynamique du collider en cours de jeu pour l'instant).
                b2Vec2 halfExtents = {bc.Size.x * transform.Scale.x, bc.Size.y * transform.Scale.y};
                b2Vec2 offset = {bc.Offset.x, bc.Offset.y};
                b2Polygon box = b2MakeOffsetBox(halfExtents.x, halfExtents.y, offset, b2Rot_identity);

                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.density = bc.Density;
                shapeDef.material.friction = bc.Friction;
                shapeDef.material.restitution = bc.Restitution;
                shapeDef.enableContactEvents = true;
                shapeDef.userData = EntityToUserData(entityHandle);

                bc.RuntimeShape = b2CreatePolygonShape(rb.RuntimeBody, &shapeDef, &box);
            }

            if (entity.HasComponent<CircleColliderComponent>())
            {
                auto &cc = entity.GetComponent<CircleColliderComponent>();

                // Box2D n'a qu'un seul rayon par cercle : en cas d'échelle non-uniforme
                // (Scale.x != Scale.y), on prend la moyenne — pas de vrai support d'ellipse.
                float scale = (transform.Scale.x + transform.Scale.y) * 0.5f;
                b2Circle circle;
                circle.center = {cc.Offset.x, cc.Offset.y};
                circle.radius = cc.Radius * scale;

                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.density = cc.Density;
                shapeDef.material.friction = cc.Friction;
                shapeDef.material.restitution = cc.Restitution;
                shapeDef.enableContactEvents = true;
                shapeDef.userData = EntityToUserData(entityHandle);

                cc.RuntimeShape = b2CreateCircleShape(rb.RuntimeBody, &shapeDef, &circle);
            }
        }
    }

    void PhysicsSystem::OnRuntimeStop()
    {
        // Détruire le monde libère tous ses bodies/shapes avec — pas besoin de les
        // détruire un par un ni de remettre RuntimeBody/RuntimeShape à null explicitement,
        // la scène runtime elle-même est jetée juste après par l'appelant.
        m_Physics.reset();
    }

    void PhysicsSystem::OnUpdate(Scene &scene, float timestep)
    {
        m_Physics->Step(timestep);

        auto view = scene.GetAllEntitiesWith<RigidBodyComponent>();
        for (auto entityHandle : view)
        {
            Entity entity{entityHandle, &scene};
            auto &rb = entity.GetComponent<RigidBodyComponent>();
            auto &transform = entity.GetComponent<TransformComponent>();

            b2Vec2 position = b2Body_GetPosition(rb.RuntimeBody);
            b2Rot rotation = b2Body_GetRotation(rb.RuntimeBody);

            transform.Position.x = position.x;
            transform.Position.y = position.y;
            transform.Rotation = glm::degrees(b2Rot_GetAngle(rotation));
        }

        // Les contact events sont bufferisés par Box2D pendant le Step() et lus ici,
        // juste après — pas de callback direct dans l'API v3.
        b2ContactEvents events = b2World_GetContactEvents(m_Physics->GetWorldId());

        if (OnCollisionBegin)
        {
            for (int i = 0; i < events.beginCount; i++)
            {
                const b2ContactBeginTouchEvent &e = events.beginEvents[i];
                OnCollisionBegin(UserDataToEntity(scene, b2Shape_GetUserData(e.shapeIdA)),
                                  UserDataToEntity(scene, b2Shape_GetUserData(e.shapeIdB)));
            }
        }

        if (OnCollisionEnd)
        {
            for (int i = 0; i < events.endCount; i++)
            {
                const b2ContactEndTouchEvent &e = events.endEvents[i];
                // D'après la doc Box2D, une shape peut avoir été détruite entre temps
                // (destruction d'entité en cours de Play, pas encore possible ici mais
                // sera vrai dès que ça le sera) — vérifier avant de lire son user data.
                if (!b2Shape_IsValid(e.shapeIdA) || !b2Shape_IsValid(e.shapeIdB))
                    continue;

                OnCollisionEnd(UserDataToEntity(scene, b2Shape_GetUserData(e.shapeIdA)),
                                UserDataToEntity(scene, b2Shape_GetUserData(e.shapeIdB)));
            }
        }
    }
}
