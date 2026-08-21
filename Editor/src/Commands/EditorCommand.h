#pragma once
#include <string>

// Une action d'édition annulable. Chaque commande sait se rejouer (Redo) et se
// défaire (Undo) — c'est la brique de base de l'historique Ctrl+Z / Ctrl+Y.
//
// Les commandes désignent leurs entités par UUID et jamais par Engine::Entity :
// les handles entt sont recyclés, donc une entité supprimée puis recréée par une
// annulation n'a plus le même handle alors que son UUID, lui, ne change pas.
class EditorCommand
{
public:
    virtual ~EditorCommand() = default;

    virtual void Redo() = 0;
    virtual void Undo() = 0;

    // Affiché dans le menu Édition : "Annuler : déplacer Square".
    virtual std::string GetName() const = 0;
};
