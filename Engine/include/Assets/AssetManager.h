#pragma once
#include "Assets/AssetTypes.h"
#include "Audio/AudioSource.h"
#include "Renderer/Texture.h"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine
{
    // Registre des assets du projet et cache de ce qui est chargé.
    //
    // Les components ne gardent pas l'objet chargé mais son AssetHandle, et le résolvent
    // au moment de s'en servir : c'est ce qui permet de sérialiser une référence d'asset,
    // et de remplacer un asset rechargé à chaud sans que personne ne garde l'ancien.
    class AssetManager
    {
    public:
        // assetRoot est la racine des assets du projet ("assets"). Le registre y est lu
        // et écrit sous le nom AssetRegistry.json.
        static void Init(const std::filesystem::path &assetRoot);
        static void Shutdown();

        static const std::filesystem::path &GetAssetRoot();

        // Le chemin est de préférence relatif à la racine des assets ("textures/x.png") ;
        // un chemin absolu ou relatif au dossier de travail est ramené à la racine.
        //
        // Enregistre le fichier s'il est inconnu, et renvoie son handle. Le handle d'un
        // chemin déjà connu ne change jamais : c'est ce que garantit le registre.
        static AssetHandle Import(const std::string &path);

        // Retire un asset du registre et du cache. Le fichier lui-même n'est pas touché.
        static void Remove(AssetHandle handle);

        static bool IsValid(AssetHandle handle);
        static AssetType GetType(AssetHandle handle);
        static std::string GetPath(AssetHandle handle);
        static const std::unordered_map<uint64_t, AssetMetadata> &GetRegistry();

        // Chargent à la demande et mettent en cache. Renvoient nullptr si le handle est
        // inconnu, du mauvais type, ou si le fichier ne se charge pas.
        static std::shared_ptr<Texture2D> GetTexture(AssetHandle handle);

        // Chaque appel renvoie un son distinct : deux entités qui jouent le même fichier
        // doivent avoir leur propre lecture, un son partagé se couperait lui-même.
        static std::shared_ptr<AudioSource> LoadSound(AssetHandle handle);

        // Handles des scènes dont le fichier a changé depuis le dernier appel, puis
        // oubliés. Les scènes ne sont pas mises en cache — chaque instance en est une
        // copie indépendante — donc AssetManager se contente de signaler le changement
        // et laisse l'éditeur décider quoi en faire.
        static std::vector<AssetHandle> TakeModifiedScenes();

        // Recharge les assets dont le fichier a changé sur le disque depuis leur
        // chargement. Appelé périodiquement par l'éditeur.
        static void ReloadModifiedAssets();

        // Nombre d'assets effectivement rechargés depuis le lancement. Sert à observer
        // le rechargement à chaud sans avoir à manipuler la ressource elle-même.
        static uint64_t GetReloadCount();

        static void SaveRegistry();

    private:
        static void LoadRegistry();
    };
}
