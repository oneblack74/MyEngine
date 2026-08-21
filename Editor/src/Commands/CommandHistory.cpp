#include "Commands/CommandHistory.h"

void CommandHistory::Execute(std::unique_ptr<EditorCommand> command)
{
    command->Redo();
    Push(std::move(command));
}

void CommandHistory::PushAlreadyApplied(std::unique_ptr<EditorCommand> command)
{
    Push(std::move(command));
}

void CommandHistory::Push(std::unique_ptr<EditorCommand> command)
{
    // Une nouvelle action après des annulations abandonne la branche rétablissable.
    m_Commands.erase(m_Commands.begin() + m_NextUndoIndex, m_Commands.end());

    m_Commands.push_back(std::move(command));

    if ((int)m_Commands.size() > k_MaxDepth)
        m_Commands.erase(m_Commands.begin());

    m_NextUndoIndex = (int)m_Commands.size();
}

void CommandHistory::Undo()
{
    if (!CanUndo())
        return;

    --m_NextUndoIndex;
    m_Commands[m_NextUndoIndex]->Undo();
}

void CommandHistory::Redo()
{
    if (!CanRedo())
        return;

    m_Commands[m_NextUndoIndex]->Redo();
    ++m_NextUndoIndex;
}

void CommandHistory::Clear()
{
    m_Commands.clear();
    m_NextUndoIndex = 0;
}

std::string CommandHistory::PeekUndoName() const
{
    return CanUndo() ? m_Commands[m_NextUndoIndex - 1]->GetName() : std::string();
}

std::string CommandHistory::PeekRedoName() const
{
    return CanRedo() ? m_Commands[m_NextUndoIndex]->GetName() : std::string();
}
