#pragma once
#include <Renderer/Framebuffer.h>
#include <Scene/Scene.h>
#include <glm/glm.hpp>
#include <memory>

// Affiche un rendu "propre" de la scène (sans gizmo ni sélection au clic, contrairement
// à ViewportPanel) à travers la CameraComponent marquée Primary dans la scène — se
// rapproche de la vue "Game" de Unity/Godot. Si aucune caméra principale n'existe, affiche
// un avertissement au lieu de fabriquer une image (comme le ferait Unity).
//
// N'est ouvert que pendant le Play (ouvert par EditorLayer::OnScenePlay, fermé par
// OnSceneStop ou en cliquant sa croix) et flotte toujours en fenêtre OS séparée, jamais
// docké dans le layout principal — grâce au multi-viewport ImGui
// (ImGuiConfigFlags_ViewportsEnable + ConfigViewportsNoAutoMerge).
//
// Rendu à une résolution fixe (1920x1080 par défaut, pas encore configurable), affiché
// à l'échelle réglable via le slider "Scale" façon Unity — zoome juste l'affichage, ne
// change pas la résolution réellement rendue.
class GamePanel
{
public:
    GamePanel();

    // Retourne false si l'utilisateur a fermé la fenêtre via sa croix (le Play doit
    // alors s'arrêter côté EditorLayer) — ne fait rien si `visible` est déjà false.
    bool OnImGuiRender(const std::shared_ptr<Engine::Scene> &scene, bool visible);

private:
    std::shared_ptr<Engine::Framebuffer> m_Framebuffer;

    int m_TargetWidth = 1920;
    int m_TargetHeight = 1080;
    float m_Scale = 0.5f; // 0.5x par défaut : 1920x1080 en plein écran serait trop grand à l'ouverture
};
