#include "RuntimeLayer.h"
#include "RuntimeOptions.h"
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
        std::cout << "Usage : Runtime [scène] [options]\n"
                     "  [scène]                scène jouée au démarrage, relative au dossier d'assets\n"
                     "                         (défaut : scenes/main.scene)\n"
                     "  --assets <dossier>     racine des assets (défaut : assets, à côté de l'exécutable)\n"
                     "  --width <n>            largeur de la fenêtre (défaut : 1280)\n"
                     "  --height <n>           hauteur de la fenêtre (défaut : 720)\n"
                     "  --headless             fenêtre cachée : joue et rend hors écran, sans rien afficher\n"
                     "  --frames <n>           quitte après n frames (défaut : jusqu'à la fermeture)\n"
                     "  --screenshot <chemin>  capture la dernière frame dans ce PNG (--frames vaut 10 par défaut)\n"
                     "  --help                 affiche cette aide\n";
    }

    // Un build packagé emporte ses assets à côté de son binaire : s'ils sont là, ce sont
    // les bons, quel que soit le dossier depuis lequel le jeu a été lancé. Sinon on
    // retombe sur "assets" relatif au dossier de travail (le cas en développement, où
    // l'on lance le player depuis build/).
    std::filesystem::path DefaultAssetRoot(const char *argv0)
    {
        const std::filesystem::path executableDirectory = Engine::Paths::ExecutableDirectory(argv0);
        if (!executableDirectory.empty() && std::filesystem::exists(executableDirectory / "assets"))
            return executableDirectory / "assets";

        return "assets";
    }

    // Renvoie false si les arguments sont invalides (le message a déjà été affiché).
    bool ParseArgs(int argc, char **argv, RuntimeOptions &options, bool &shouldExit)
    {
        bool sceneGiven = false;

        for (int i = 1; i < argc; ++i)
        {
            const char *arg = argv[i];

            // Le seul argument sans tiret est la scène à jouer : c'est ce qu'on tape le
            // plus souvent, il n'a pas à porter un nom d'option.
            if (arg[0] != '-')
            {
                if (sceneGiven)
                {
                    std::cerr << "Une seule scène peut être jouée : " << arg << "\n";
                    return false;
                }
                options.ScenePath = arg;
                sceneGiven = true;
            }
            else if (std::strcmp(arg, "--help") == 0)
            {
                PrintUsage();
                shouldExit = true;
                return true;
            }
            else if (std::strcmp(arg, "--headless") == 0)
            {
                options.Headless = true;
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
            else if (std::strcmp(arg, "--frames") == 0 || std::strcmp(arg, "--width") == 0 ||
                     std::strcmp(arg, "--height") == 0)
            {
                if (i + 1 >= argc)
                {
                    std::cerr << arg << " attend un nombre\n";
                    return false;
                }
                const int value = std::atoi(argv[++i]);
                if (value < 1)
                {
                    std::cerr << arg << " attend un nombre >= 1\n";
                    return false;
                }

                if (std::strcmp(arg, "--frames") == 0)
                    options.MaxFrames = value;
                else if (std::strcmp(arg, "--width") == 0)
                    options.Width = (unsigned int)value;
                else
                    options.Height = (unsigned int)value;
            }
            else
            {
                std::cerr << "Argument inconnu : " << arg << "\n";
                PrintUsage();
                return false;
            }
        }

        // Une capture sans durée demandée n'a aucune raison de laisser tourner le jeu :
        // quelques frames suffisent à ce que la scène soit à l'écran (même choix que
        // l'éditeur avec ses frames de chauffe).
        if (!options.ScreenshotPath.empty() && options.MaxFrames == 0)
            options.MaxFrames = 10;

        return true;
    }
}

int main(int argc, char **argv)
{
    RuntimeOptions options;
    options.AssetRoot = DefaultAssetRoot(argv[0]);

    bool shouldExit = false;
    if (!ParseArgs(argc, argv, options, shouldExit))
        return 2;
    if (shouldExit)
        return 0;

    Engine::Log::Init();
    LOG_INFO("Runtime starting...");

    Engine::WindowProps props(std::filesystem::path(options.ScenePath).stem().string(),
                              options.Width, options.Height, !options.Headless);

    Engine::Application app(props, options.AssetRoot);
    // Le layer appartient à l'Application, mais le pointeur reste valide jusqu'à sa
    // destruction — donc après l'évaluation du code de sortie ci-dessous.
    RuntimeLayer *runtime = new RuntimeLayer(options);
    app.PushLayer(runtime);
    app.Run();

    return runtime->GetExitCode();
}
