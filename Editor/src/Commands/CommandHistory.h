#pragma once
#include "Commands/EditorCommand.h"
#include <memory>
#include <vector>

// Pile d'annulation de l'éditeur : les commandes exécutées s'empilent, Undo remonte
// la pile, Redo la redescend. Toute nouvelle commande exécutée après une annulation
// efface la branche de rétablissement (comportement standard d'un éditeur).
class CommandHistory
{
public:
    // Exécute la commande puis l'empile.
    void Execute(std::unique_ptr<EditorCommand> command);

    // Empile une commande dont l'effet est déjà appliqué — cas des widgets ImGui, qui
    // écrivent directement dans le component pendant qu'on les manipule : la rejouer
    // ici ne ferait que réappliquer ce qui est déjà là.
    void PushAlreadyApplied(std::unique_ptr<EditorCommand> command);

    void Undo();
    void Redo();
    void Clear();

    bool CanUndo() const { return m_NextUndoIndex > 0; }
    bool CanRedo() const { return m_NextUndoIndex < (int)m_Commands.size(); }

    // Chaîne vide s'il n'y a rien à annuler/rétablir.
    std::string PeekUndoName() const;
    std::string PeekRedoName() const;

private:
    void Push(std::unique_ptr<EditorCommand> command);

    // Les commandes au-delà de m_NextUndoIndex sont celles qui ont été annulées et
    // restent disponibles pour un Redo.
    std::vector<std::unique_ptr<EditorCommand>> m_Commands;
    int m_NextUndoIndex = 0;

    // Au-delà, les commandes les plus anciennes sont oubliées.
    static constexpr int k_MaxDepth = 200;
};
