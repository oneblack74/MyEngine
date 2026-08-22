#pragma once
#include <filesystem>

namespace Engine::Paths
{
    // Dossier de l'exécutable en cours, déduit de argv[0]. Sert à retrouver ce qu'un
    // build packagé emporte à côté de son binaire (assets...), indépendamment du dossier
    // depuis lequel il a été lancé.
    //
    // Renvoie un chemin vide si argv[0] ne dit rien d'exploitable — c'est le cas quand le
    // programme est trouvé via le PATH, où argv[0] est un simple nom. À l'appelant de
    // prévoir un repli.
    std::filesystem::path ExecutableDirectory(const char *argv0);
}
