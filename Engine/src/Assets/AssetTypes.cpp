#include "Assets/AssetTypes.h"
#include <algorithm>

namespace Engine
{
    AssetType AssetTypeFromExtension(const std::string &extension)
    {
        std::string lower = extension;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });

        if (lower == ".png" || lower == ".jpg" || lower == ".jpeg" || lower == ".bmp" || lower == ".tga")
            return AssetType::Texture;
        if (lower == ".wav" || lower == ".mp3" || lower == ".flac" || lower == ".ogg")
            return AssetType::Audio;

        return AssetType::None;
    }

    const char *AssetTypeToString(AssetType type)
    {
        switch (type)
        {
        case AssetType::Texture:
            return "Texture";
        case AssetType::Audio:
            return "Audio";
        default:
            return "None";
        }
    }

    AssetType AssetTypeFromString(const std::string &name)
    {
        if (name == "Texture")
            return AssetType::Texture;
        if (name == "Audio")
            return AssetType::Audio;
        return AssetType::None;
    }
}
