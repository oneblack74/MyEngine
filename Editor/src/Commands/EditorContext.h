#pragma once
#include <Scene/Entity.h>
#include <Scene/Scene.h>

// Ce dont une commande a besoin de l'éditeur pour s'appliquer. Interface plutôt que
// pointeur direct vers EditorLayer : les commandes ne connaissent rien de l'UI.
//
// Toutes les commandes agissent sur la scène d'édition. L'annulation est volontairement
// indisponible pendant le Play : la scène runtime est une copie jetable, y annuler une
// action n'aurait aucun effet durable.
class EditorContext
{
public:
    virtual ~EditorContext() = default;

    virtual Engine::Scene &GetEditorScene() = 0;
    virtual void SelectEntity(Engine::Entity entity) = 0;
};
