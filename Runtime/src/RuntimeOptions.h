#pragma once
#include <filesystem>
#include <string>

// Réglages de lancement du player, remplis depuis la ligne de commande.
struct RuntimeOptions
{
    // Racine des assets du jeu. Par défaut le dossier "assets" posé à côté de
    // l'exécutable dans un build packagé — pas de chemin en dur compilé, contrairement
    // à l'éditeur qui travaille sur les sources.
    std::filesystem::path AssetRoot = "assets";

    // Scène chargée au démarrage, relative à AssetRoot (un chemin absolu est accepté tel quel).
    std::string ScenePath = "scenes/main.scene";

    unsigned int Width = 1280;
    unsigned int Height = 720;

    // Fenêtre cachée : le jeu tourne et rend quand même, sans rien afficher. Sert aux
    // tests automatisés, comme pour l'éditeur.
    bool Headless = false;

    std::string ScreenshotPath;

    // 0 = tourne jusqu'à la fermeture de la fenêtre.
    int MaxFrames = 0;
};
