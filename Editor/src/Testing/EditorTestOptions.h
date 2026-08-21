#pragma once
#include <string>

// Options du mode test de l'éditeur, remplies depuis la ligne de commande par main().
// En usage normal (aucun argument) tout est à sa valeur par défaut et l'éditeur se
// comporte exactement comme avant.
struct EditorTestOptions
{
    // Fenêtre GLFW cachée : l'éditeur rend normalement mais rien n'apparaît à l'écran.
    bool Headless = false;

    // Si non vide : capture le rendu dans ce fichier PNG puis quitte.
    std::string ScreenshotPath;

    // Nombre de frames rendues avant la capture. Il en faut plus d'une : la mise en
    // place du dock layout est volontairement différée d'une frame (voir
    // EditorLayer::RenderImGui), et ImGui a besoin d'une passe pour connaître la
    // taille réelle des panels.
    int WarmupFrames = 10;
};
