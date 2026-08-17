#pragma once
#include <Renderer/Framebuffer.h>
#include <Renderer/OrthographicCamera.h>
#include <Scene/Scene.h>
#include <glm/glm.hpp>
#include <memory>

// Affiche un rendu "propre" de la scène (sans gizmo ni sélection au clic, contrairement
// à ViewportPanel), avec sa propre caméra fixe indépendante de la navigation d'édition —
// se rapproche de la vue "Game" de Unity/Godot. N'est ouvert que pendant le Play (ouvert
// par EditorLayer::OnScenePlay, fermé par OnSceneStop ou en cliquant sa croix) et flotte
// toujours en fenêtre OS séparée, jamais docké dans le layout principal — grâce au
// multi-viewport ImGui (ImGuiConfigFlags_ViewportsEnable + ConfigViewportsNoAutoMerge).
//
// Rendu à une résolution fixe (1920x1080 par défaut, pas encore configurable), affiché
// à l'échelle réglable via le slider "Scale" façon Unity — zoome juste l'affichage, ne
// change pas la résolution réellement rendue.
//
// Limite connue : il n'existe pas encore de CameraComponent dans l'ECS, donc ce panel ne
// peut pas (encore) montrer ce que verrait une vraie caméra de jeu placée dans la scène —
// sa caméra est fixe, pas configurable depuis l'éditeur.
class GamePanel
{
public:
    GamePanel();

    // Retourne false si l'utilisateur a fermé la fenêtre via sa croix (le Play doit
    // alors s'arrêter côté EditorLayer) — ne fait rien si `visible` est déjà false.
    bool OnImGuiRender(const std::shared_ptr<Engine::Scene> &scene, bool visible);

private:
    std::shared_ptr<Engine::Framebuffer> m_Framebuffer;
    Engine::OrthographicCamera m_Camera;

    int m_TargetWidth = 1920;
    int m_TargetHeight = 1080;
    float m_Scale = 0.5f; // 0.5x par défaut : 1920x1080 en plein écran serait trop grand à l'ouverture
};
