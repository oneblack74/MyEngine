#include "Utils/Paths.h"

namespace Engine::Paths
{
    std::filesystem::path ExecutableDirectory(const char *argv0)
    {
        if (!argv0 || *argv0 == '\0')
            return {};

        const std::filesystem::path invoked(argv0);
        // Un nom nu ("MyFirstGame") a été résolu par le PATH : on ne sait pas d'où il
        // vient, et le prendre pour un chemin relatif désignerait le dossier de travail.
        if (!invoked.has_parent_path())
            return {};

        std::error_code ec;
        const std::filesystem::path resolved = std::filesystem::weakly_canonical(invoked, ec);
        return (ec ? invoked : resolved).parent_path();
    }
}
