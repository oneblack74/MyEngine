#pragma once
#include "Testing/EditorTestOptions.h"

class EditorLayer;
struct ImGuiTestEngine;

// Enveloppe Dear ImGui Test Engine : cycle de vie, capture d'écran, et pilotage du
// mode headless (lancer toute la suite puis quitter avec un code de sortie).
//
// Compilée à vide quand l'option CMake MYENGINE_EDITOR_TESTS est désactivée : l'éditeur
// se construit alors sans la dépendance et tous les appels ci-dessous ne font rien.
class EditorTestEngine
{
public:
    void Start(EditorLayer &editor, const EditorTestOptions &options);
    void Stop();

    // À appeler au tout début de la frame, avant ImGui::NewFrame() : le Test Engine
    // veut son PostSwap une fois le framebuffer présenté, et le swap de la frame
    // précédente vient justement d'avoir lieu dans Window::OnUpdate.
    void OnFrameStart();

    // Fenêtres du Test Engine (liste des tests, log) — mode interactif uniquement.
    void RenderUI();

    // 0 tant que rien n'a échoué ; renseigné à la fin d'une exécution headless.
    int ExitCode() const { return m_ExitCode; }

private:
    ImGuiTestEngine *m_Engine = nullptr;
    EditorTestOptions m_Options;
    int m_FrameCount = 0;
    bool m_TestsQueued = false;
    int m_ExitCode = 0;
};
