#include "EditorLayer.h"
#include "Testing/EditorTestOptions.h"
#include <Core/Application.h>
#include <Core/Log.h>
#include <cstring>
#include <iostream>

namespace
{
    void PrintUsage()
    {
        std::cout << "Usage : Editor [options]\n"
                     "  (sans option)          lance l'éditeur normalement\n"
                     "  --headless             fenêtre cachée : rend hors écran, sans rien afficher\n"
                     "  --screenshot <chemin>  capture le rendu dans ce PNG puis quitte\n"
                     "  --frames <n>           frames rendues avant la capture (défaut : 10)\n"
                     "  --tests                ouvre l'éditeur avec la fenêtre du Test Engine\n"
                     "  --tests-headless       joue toute la suite de tests sans fenêtre, puis quitte\n"
                     "                         (code de sortie 0 si tout passe, 1 sinon)\n"
                     "  --filter <motif>       restreint les tests joués (ex. \"inspector/*\")\n"
                     "  --help                 affiche cette aide\n";
    }

    // Renvoie false si les arguments sont invalides (le message a déjà été affiché).
    bool ParseArgs(int argc, char **argv, EditorTestOptions &options, bool &shouldExit)
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
            else if (std::strcmp(arg, "--tests") == 0)
            {
                options.RunTests = true;
                options.ShowTestUI = true;
            }
            else if (std::strcmp(arg, "--tests-headless") == 0)
            {
                options.RunTests = true;
                options.Headless = true;
            }
            else if (std::strcmp(arg, "--filter") == 0)
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "--filter attend un motif\n";
                    return false;
                }
                options.TestFilter = argv[++i];
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
                options.WarmupFrames = std::atoi(argv[++i]);
                if (options.WarmupFrames < 1)
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

        return true;
    }
}

int main(int argc, char **argv)
{
    EditorTestOptions options;
    bool shouldExit = false;
    if (!ParseArgs(argc, argv, options, shouldExit))
        return 2;
    if (shouldExit)
        return 0;

    Engine::Log::Init();
    LOG_INFO("Editor starting...");

    Engine::WindowProps props;
    props.Visible = !options.Headless;

    Engine::Application app(props);
    // Le layer appartient à l'Application, mais le pointeur reste valide jusqu'à sa
    // destruction — donc après l'évaluation du code de sortie ci-dessous.
    EditorLayer *editor = new EditorLayer(options);
    app.PushLayer(editor);
    app.Run();

    return editor->GetTestExitCode();
}
