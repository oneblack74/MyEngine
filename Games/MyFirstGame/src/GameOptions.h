#pragma once
#include <filesystem>
#include <string>

// Réglages de lancement, remplis depuis la ligne de commande. Les options d'exécution
// sans écran sont les mêmes que celles du Runtime : c'est ce qui rend le jeu testable.
struct GameOptions
{
    // Vide = la racine compilée dans le binaire (les sources en développement, le
    // dossier assets/ posé à côté de l'exécutable dans un jeu packagé).
    std::filesystem::path AssetRoot;

    bool Headless = false;
    std::string ScreenshotPath;
    int MaxFrames = 0; // 0 = jusqu'à ce que le joueur quitte

    // Le panier se joue tout seul : il suit la caisse la plus basse. Sert de mode
    // démonstration, et permet à un test de vérifier qu'on attrape vraiment quelque
    // chose sans avoir à simuler des appuis clavier.
    bool DemoMode = false;

    // Sort en erreur si aucune caisse n'a été attrapée à la fin de la partie testée.
    bool RequireCatch = false;
};
