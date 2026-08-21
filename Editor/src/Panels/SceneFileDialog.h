#pragma once
#include <Scene/SceneSerializer.h>
#include <filesystem>
#include <string>

// Choix d'un fichier de scène, dessiné en ImGui plutôt qu'en boîte de dialogue native :
// pas de dépendance système à ajouter, et surtout une fenêtre que le Test Engine sait
// piloter (une dialogue GTK ou Win32 lui serait complètement opaque).
class SceneFileDialog
{
public:
    enum class Mode
    {
        Open,
        Save
    };

    // Ouvre la boîte à la frame suivante. `suggestedName` pré-remplit le champ de nom
    // (le nom de la scène courante, en général).
    void OpenDialog(Mode mode, const std::filesystem::path &directory, const std::string &suggestedName);

    Mode GetMode() const { return m_Mode; }

    // Renvoie true la seule frame où l'utilisateur valide, avec son choix dans outPath.
    bool OnImGuiRender(std::filesystem::path &outPath);

private:
    Mode m_Mode = Mode::Open;
    bool m_Open = false;
    bool m_ShouldOpenPopup = false;
    std::filesystem::path m_Directory;

    // Tampon de saisie ImGui : InputText écrit dans un char[], pas dans un std::string.
    char m_Filename[128] = {};
};
