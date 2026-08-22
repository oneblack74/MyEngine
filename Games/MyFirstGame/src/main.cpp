#include "GameLayer.h"
#include "GameOptions.h"
#include <Core/Application.h>
#include <Core/Log.h>
#include <Utils/Paths.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>

namespace
{
    void PrintUsage()
    {
        std::cout << "Usage : MyFirstGame [options]\n"
                     "  (sans option)          lance le jeu\n"
                     "  --assets <dossier>     racine des assets du jeu\n"
                     "  --headless             fenêtre cachée : joue et rend hors écran\n"
                     "  --frames <n>           quitte après n frames\n"
                     "  --screenshot <chemin>  capture la dernière frame dans ce PNG\n"
                     "  --demo                 le panier se joue tout seul\n"
                     "  --require-catch        sort en erreur si aucune caisse n'a été attrapée\n"
                     "  --help                 affiche cette aide\n";
    }

    // Un jeu packagé emporte ses assets à côté de son binaire : ceux-là gagnent, sinon
    // c'est un build de développement et la définition CMake pointe les sources. Le test
    // porte sur le dossier de l'exécutable et pas sur le dossier de travail : en
    // développement, build/assets existe (les autres cibles y recopient les leurs) et
    // désignerait les assets de quelqu'un d'autre.
    std::filesystem::path DefaultAssetRoot(const char *argv0)
    {
        const std::filesystem::path executableDirectory = Engine::Paths::ExecutableDirectory(argv0);
        if (!executableDirectory.empty() && std::filesystem::exists(executableDirectory / "assets"))
            return executableDirectory / "assets";

#ifdef MYENGINE_GAME_ASSET_ROOT
        return MYENGINE_GAME_ASSET_ROOT;
#else
        return "assets";
#endif
    }

    // Renvoie false si les arguments sont invalides (le message a déjà été affiché).
    bool ParseArgs(int argc, char **argv, GameOptions &options, bool &shouldExit)
    {
        for (int i = 1; i < argc; ++i)
        {
            const char *arg = argv[i];

            if (std::strcmp(arg, "--help") == 0)
            {
                PrintUsage();
                shouldExit = true;
                return true;
            }
            else if (std::strcmp(arg, "--headless") == 0)
            {
                options.Headless = true;
            }
            else if (std::strcmp(arg, "--demo") == 0)
            {
                options.DemoMode = true;
            }
            else if (std::strcmp(arg, "--require-catch") == 0)
            {
                options.RequireCatch = true;
            }
            else if (std::strcmp(arg, "--assets") == 0)
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "--assets attend un dossier\n";
                    return false;
                }
                options.AssetRoot = argv[++i];
            }
            else if (std::strcmp(arg, "--screenshot") == 0)
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "--screenshot attend un chemin de fichier\n";
                    return false;
                }
                options.ScreenshotPath = argv[++i];
            }
            else if (std::strcmp(arg, "--frames") == 0)
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "--frames attend un nombre\n";
                    return false;
                }
                options.MaxFrames = std::atoi(argv[++i]);
                if (options.MaxFrames < 1)
                {
                    std::cerr << "--frames attend un nombre >= 1\n";
                    return false;
                }
            }
            else
            {
                std::cerr << "Argument inconnu : " << arg << "\n";
                PrintUsage();
                return false;
            }
        }

        if (!options.ScreenshotPath.empty() && options.MaxFrames == 0)
            options.MaxFrames = 10;

        return true;
    }
}

int main(int argc, char **argv)
{
    GameOptions options;
    bool shouldExit = false;
    if (!ParseArgs(argc, argv, options, shouldExit))
        return 2;
    if (shouldExit)
        return 0;

    Engine::Log::Init();
    LOG_INFO("MyFirstGame starting...");

    if (options.AssetRoot.empty())
        options.AssetRoot = DefaultAssetRoot(argv[0]);

    Engine::WindowProps props("MyFirstGame", 1280, 720, !options.Headless);

    Engine::Application app(props, options.AssetRoot);
    // Le layer appartient à l'Application, mais le pointeur reste valide jusqu'à sa
    // destruction — donc après l'évaluation du code de sortie ci-dessous.
    GameLayer *game = new GameLayer(options);
    app.PushLayer(game);
    app.Run();

    return game->GetExitCode();
}
