#pragma once
#include "Commands/CommandHistory.h"
#include "Commands/EditorContext.h"
#include "Commands/SceneCommands.h"
#include "SceneInstanceSync.h"
#include <Scene/Entity.h>
#include <functional>
#include <imgui.h>
#include <set>
#include <string>
#include <memory>

// Affiche et permet d'éditer les components de l'entité sélectionnée dans SceneHierarchyPanel.
class InspectorPanel
{
public:
    InspectorPanel() = default;

    // Le contexte sert aux commandes d'annulation créées par les éditions du panel.
    void SetEditorContext(EditorContext *context) { m_Context = context; }

    // history nul = éditions non annulables (pendant le Play, où la scène est une
    // copie jetée au Stop).
    // Sert à repérer et révoquer les surcharges d'une instance de scène. Nul : le
    // panel se comporte comme avant, sans marquage.
    void SetInstanceSync(SceneInstanceSync *sync) { m_InstanceSync = sync; }

    void OnImGuiRender(Engine::Entity selectedEntity, CommandHistory *history);

private:
    void DrawComponents(Engine::Entity entity);

    // En-tête d'une section de component : le TreeNode habituel, le bouton "Reset"
    // aligné à droite, et un menu contextuel (clic droit) pour retirer le component ou
    // révoquer ses surcharges. Renvoie true si la section est ouverte ;
    // resetRequested passe à true la frame où le bouton est cliqué.
    // componentName est le nom de sérialisation ("SpriteRendererComponent").
    bool BeginComponentSection(Engine::Entity entity, const char *label, const char *componentName,
                               bool &resetRequested);

    // Marque une propriété qui s'écarte de la scène source, d'un liseré à gauche de sa
    // ligne — façon Unity. À appeler juste après le widget concerné.
    void MarkOverride(const char *componentName, const char *field);

    void DrawComponentContextMenu(Engine::Entity entity, const char *componentName);

    // Ce qu'un menu contextuel a demandé. L'action n'est pas exécutée sur place :
    // retirer ou réécrire un component au milieu du dessin de sa propre section
    // laisserait la suite du code travailler sur une référence morte.
    struct PendingAction
    {
        enum class Kind
        {
            RemoveComponent,
            RevertComponent,
            RevertProperty
        };

        Kind Type = Kind::RemoveComponent;
        Engine::Entity Target;
        std::string Component;
        std::string Field;
    };

    void ApplyPendingAction();

    bool m_HasPendingAction = false;
    PendingAction m_PendingAction;

    SceneInstanceSync *m_InstanceSync = nullptr;

    // Propriétés surchargées de l'entité affichée, recalculées à chaque frame :
    // "NomDuComponent/NomDuChamp".
    std::set<std::string> m_Overrides;

    // À appeler juste après un widget continu (drag, sélecteur de couleur) : la valeur
    // d'avant est capturée quand on saisit le widget et la commande n'est empilée qu'au
    // relâchement, pour qu'un drag entier ne compte que pour une seule annulation.
    template <typename T>
    void TrackEdit(Engine::Entity entity, const char *actionName);

    // Pour les modifications instantanées (case à cocher, liste déroulante, bouton de
    // réinitialisation), où la valeur d'avant est connue d'avance.
    template <typename T>
    void RecordEdit(Engine::Entity entity, const T &before, const char *actionName);

    EditorContext *m_Context = nullptr;
    CommandHistory *m_History = nullptr;

    // Édition en cours : construite à la saisie du widget, appelée au relâchement pour
    // lire la valeur d'arrivée.
    std::function<std::unique_ptr<EditorCommand>()> m_PendingEdit;

    // Nom de l'entité avant la saisie en cours dans le champ Tag.
    std::string m_NameBeforeEdit;
};

template <typename T>
void InspectorPanel::TrackEdit(Engine::Entity entity, const char *actionName)
{
    if (m_History == nullptr || m_Context == nullptr)
        return;

    if (ImGui::IsItemActivated())
    {
        m_PendingEdit = [this, entity, before = entity.GetComponent<T>(), actionName]() mutable
        {
            Engine::Entity target = entity;
            return std::unique_ptr<EditorCommand>(new ComponentEditCommand<T>(
                *m_Context, target.GetUUID(), before, target.GetComponent<T>(),
                std::string(actionName) + " " + target.GetName()));
        };
    }

    if (ImGui::IsItemDeactivatedAfterEdit() && m_PendingEdit)
    {
        // Le widget a déjà écrit la nouvelle valeur : la commande ne doit pas la
        // réappliquer, seulement rendre le retour en arrière possible.
        m_History->PushAlreadyApplied(m_PendingEdit());
        m_PendingEdit = nullptr;
    }
}

template <typename T>
void InspectorPanel::RecordEdit(Engine::Entity entity, const T &before, const char *actionName)
{
    if (m_History == nullptr || m_Context == nullptr)
        return;

    m_History->PushAlreadyApplied(std::make_unique<ComponentEditCommand<T>>(
        *m_Context, entity.GetUUID(), before, entity.GetComponent<T>(),
        std::string(actionName) + " " + entity.GetName()));
}
