#pragma once

class EditorLayer;
struct ImGuiTestEngine;

// Enregistre la suite de tests automatisés de l'éditeur. Les tests pilotent l'UI
// (clics, saisies) puis vérifient l'état réel derrière — scène, sélection, Play/Stop.
void RegisterEditorTests(ImGuiTestEngine *engine, EditorLayer &editor);
