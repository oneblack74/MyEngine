#pragma once
#include "Core/UUID.h"
#include <string>

namespace Engine
{
    // Référence stable vers un asset. C'est un UUID, comme l'identité des entités :
    // il survit à un déplacement de fichier (seul le chemin change dans le registre)
    // et à une sauvegarde/rechargement de scène, ce qu'un chemin brut ne fait pas.
    using AssetHandle = UUID;

    inline constexpr uint64_t k_InvalidAssetHandle = 0;

    enum class AssetType
    {
        None = 0,
        Texture,
        Audio
    };

    struct AssetMetadata
    {
        AssetType Type = AssetType::None;

        // Chemin du fichier, relatif à la racine des assets.
        std::string Path;
    };

    // Type déduit de l'extension : il n'y a pas de fichier .meta à côté de chaque asset,
    // le registre suffit à ce stade.
    AssetType AssetTypeFromExtension(const std::string &extension);
    const char *AssetTypeToString(AssetType type);
    AssetType AssetTypeFromString(const std::string &name);
}
