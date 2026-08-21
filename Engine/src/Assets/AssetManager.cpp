#include "Assets/AssetManager.h"
#include "Core/Log.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace Engine
{
    namespace
    {
        struct CachedAsset
        {
            std::shared_ptr<Texture2D> Texture;

            // Date du fichier au moment du chargement, pour le rechargement à chaud.
            std::filesystem::file_time_type LastWriteTime{};
        };

        std::filesystem::path s_AssetRoot;
        std::unordered_map<uint64_t, AssetMetadata> s_Registry;
        std::unordered_map<uint64_t, CachedAsset> s_Cache;
        uint64_t s_ReloadCount = 0;


        // Dates des fichiers de scène, suivies à part : une scène n'a pas d'entrée de
        // cache, puisqu'on n'en garde jamais d'exemplaire chargé.
        std::unordered_map<uint64_t, std::filesystem::file_time_type> s_SceneWriteTimes;
        std::vector<AssetHandle> s_ModifiedScenes;

        std::filesystem::path RegistryFile()
        {
            return s_AssetRoot / "AssetRegistry.json";
        }

        std::filesystem::path FullPath(const AssetMetadata &metadata)
        {
            return s_AssetRoot / metadata.Path;
        }

        // Relève la date d'un fichier de scène, sans la signaler comme un changement.
        // Appelé dès l'enregistrement de l'asset : sans ça, une scène modifiée avant le
        // premier passage du hot-reload verrait sa modification avalée, la date relevée
        // à ce moment-là étant déjà la nouvelle.
        void RememberSceneWriteTime(uint64_t handle, const AssetMetadata &metadata)
        {
            if (metadata.Type != AssetType::Scene)
                return;

            std::error_code ec;
            const auto writeTime = std::filesystem::last_write_time(FullPath(metadata), ec);
            if (!ec)
                s_SceneWriteTimes[handle] = writeTime;
        }

        // Les chemins sont stockés relativement à la racine des assets et avec des
        // séparateurs '/', pour qu'un registre écrit sur une machine soit lisible sur
        // une autre.
        std::string NormalizePath(const std::string &path)
        {
            std::filesystem::path candidate(path);

            // Déjà exprimé par rapport à la racine : rien à faire.
            std::error_code ec;
            if (std::filesystem::exists(s_AssetRoot / candidate, ec))
                return candidate.generic_string();

            // Sinon (chemin absolu, ou relatif au dossier de travail), on tente de le
            // ramener à la racine ; s'il tombe en dehors, on le garde tel quel.
            std::filesystem::path relative = std::filesystem::relative(candidate, s_AssetRoot, ec);
            if (!ec && !relative.empty() && relative.native().rfind("..", 0) != 0)
                candidate = relative;

            return candidate.generic_string();
        }

        const AssetMetadata *FindMetadata(AssetHandle handle)
        {
            auto it = s_Registry.find((uint64_t)handle);
            return it == s_Registry.end() ? nullptr : &it->second;
        }
    }

    void AssetManager::Init(const std::filesystem::path &assetRoot)
    {
        s_AssetRoot = assetRoot;
        LoadRegistry();
    }

    const std::filesystem::path &AssetManager::GetAssetRoot()
    {
        return s_AssetRoot;
    }

    void AssetManager::Shutdown()
    {
        s_Cache.clear();
        s_Registry.clear();
        s_AssetRoot.clear();
    }

    AssetHandle AssetManager::Import(const std::string &path)
    {
        const std::string normalized = NormalizePath(path);

        for (const auto &[handle, metadata] : s_Registry)
        {
            if (metadata.Path == normalized)
                return AssetHandle(handle);
        }

        const AssetType type = AssetTypeFromExtension(std::filesystem::path(normalized).extension().string());
        if (type == AssetType::None)
        {
            ENGINE_LOG_WARN(LogCategories::Assets, "Unknown asset type, file ignored: {0}", normalized);
            return AssetHandle(k_InvalidAssetHandle);
        }

        AssetHandle handle;
        s_Registry[(uint64_t)handle] = {type, normalized};
        RememberSceneWriteTime((uint64_t)handle, s_Registry[(uint64_t)handle]);
        SaveRegistry();

        ENGINE_LOG_INFO(LogCategories::Assets, "Asset imported: {0} ({1})", normalized, AssetTypeToString(type));
        return handle;
    }

    bool AssetManager::IsValid(AssetHandle handle)
    {
        return FindMetadata(handle) != nullptr;
    }

    AssetType AssetManager::GetType(AssetHandle handle)
    {
        const AssetMetadata *metadata = FindMetadata(handle);
        return metadata ? metadata->Type : AssetType::None;
    }

    std::string AssetManager::GetPath(AssetHandle handle)
    {
        const AssetMetadata *metadata = FindMetadata(handle);
        return metadata ? metadata->Path : std::string();
    }

    const std::unordered_map<uint64_t, AssetMetadata> &AssetManager::GetRegistry()
    {
        return s_Registry;
    }

    std::shared_ptr<Texture2D> AssetManager::GetTexture(AssetHandle handle)
    {
        const AssetMetadata *metadata = FindMetadata(handle);
        if (metadata == nullptr || metadata->Type != AssetType::Texture)
            return nullptr;

        auto cached = s_Cache.find((uint64_t)handle);
        if (cached != s_Cache.end())
            return cached->second.Texture;

        const std::filesystem::path file = FullPath(*metadata);
        if (!std::filesystem::exists(file))
        {
            ENGINE_LOG_ERROR(LogCategories::Assets, "Texture not found: {0}", file.string());
            return nullptr;
        }

        CachedAsset entry;
        entry.Texture = std::make_shared<Texture2D>(file.string());
        entry.LastWriteTime = std::filesystem::last_write_time(file);
        s_Cache[(uint64_t)handle] = entry;

        return entry.Texture;
    }

    std::shared_ptr<AudioSource> AssetManager::LoadSound(AssetHandle handle)
    {
        const AssetMetadata *metadata = FindMetadata(handle);
        if (metadata == nullptr || metadata->Type != AssetType::Audio)
            return nullptr;

        // Pas de cache ici : chaque appelant reçoit sa propre lecture (voir l'en-tête).
        return AudioSource::LoadFromFile(FullPath(*metadata).string());
    }

    void AssetManager::ReloadModifiedAssets()
    {
        // Les scènes d'abord : elles ne passent pas par le cache, leur date est
        // comparée directement à celle relevée au dernier passage.
        for (const auto &[handle, metadata] : s_Registry)
        {
            if (metadata.Type != AssetType::Scene)
                continue;

            std::error_code ec;
            const auto writeTime = std::filesystem::last_write_time(FullPath(metadata), ec);
            if (ec)
                continue;

            auto known = s_SceneWriteTimes.find(handle);
            if (known == s_SceneWriteTimes.end())
            {
                // Première observation : on note la date sans crier au changement.
                s_SceneWriteTimes[handle] = writeTime;
                continue;
            }

            if (known->second == writeTime)
                continue;

            known->second = writeTime;
            s_ModifiedScenes.push_back(AssetHandle(handle));
            ++s_ReloadCount;
            ENGINE_LOG_INFO(LogCategories::Assets, "Scene asset changed: {0}", metadata.Path);
        }

        for (auto &[handle, entry] : s_Cache)
        {
            const AssetMetadata *metadata = FindMetadata(AssetHandle(handle));
            if (metadata == nullptr || !entry.Texture)
                continue;

            const std::filesystem::path file = FullPath(*metadata);
            std::error_code ec;
            const auto writeTime = std::filesystem::last_write_time(file, ec);
            if (ec || writeTime == entry.LastWriteTime)
                continue;

            // L'entrée du cache reçoit une nouvelle Texture2D ; quiconque garde encore
            // l'ancienne continue de la voir. C'est sans conséquence parce que les
            // consommateurs résolvent leur handle à chaque frame (voir RenderSystem)
            // plutôt que de conserver la texture dans leur component.
            entry.Texture = std::make_shared<Texture2D>(file.string());
            entry.LastWriteTime = writeTime;
            ++s_ReloadCount;
            ENGINE_LOG_INFO(LogCategories::Assets, "Asset reloaded: {0}", metadata->Path);
        }
    }

    std::vector<AssetHandle> AssetManager::TakeModifiedScenes()
    {
        std::vector<AssetHandle> modified;
        modified.swap(s_ModifiedScenes);
        return modified;
    }

    uint64_t AssetManager::GetReloadCount()
    {
        return s_ReloadCount;
    }

    void AssetManager::SaveRegistry()
    {
        if (s_AssetRoot.empty())
            return;

        nlohmann::json json;
        for (const auto &[handle, metadata] : s_Registry)
        {
            json[std::to_string(handle)] = {
                {"Type", AssetTypeToString(metadata.Type)},
                {"Path", metadata.Path},
            };
        }

        std::error_code ec;
        std::filesystem::create_directories(s_AssetRoot, ec);

        std::ofstream out(RegistryFile());
        if (!out)
        {
            ENGINE_LOG_ERROR(LogCategories::Assets, "Could not write {0}", RegistryFile().string());
            return;
        }
        out << json.dump(4);
    }

    void AssetManager::LoadRegistry()
    {
        s_Registry.clear();

        std::ifstream in(RegistryFile());
        if (!in)
            return; // premier lancement : le registre se créera au premier import

        nlohmann::json json;
        in >> json;

        for (auto it = json.begin(); it != json.end(); ++it)
        {
            AssetMetadata metadata;
            metadata.Type = AssetTypeFromString(it.value()["Type"].get<std::string>());
            metadata.Path = it.value()["Path"].get<std::string>();
            s_Registry[std::stoull(it.key())] = metadata;
        }

        ENGINE_LOG_INFO(LogCategories::Assets, "{0} asset(s) in registry", s_Registry.size());
    }
}
