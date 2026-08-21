#pragma once
#include <Assets/AssetTypes.h>
#include <Scene/Scene.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>

// Répercute une scène source modifiée sur ses instances, en préservant ce que
// l'utilisateur a changé sur chacune.
//
// Le principe : une fusion à trois côtés, comme un merge de gestionnaire de versions.
// Pour chaque propriété, on compare ce qu'a l'instance à ce qu'avait la source la
// dernière fois qu'on l'a vue (la « base ») :
//   - la valeur de l'instance diffère de la base  -> c'est une surcharge, on la garde
//   - elle est identique                          -> on prend la nouvelle valeur
//
// La comparaison se fait sur la forme JSON des entités, celle du SceneSerializer : le
// JSON nomme déjà chaque propriété, ce qui évite d'écrire une réflexion sur les
// components rien que pour ça.
class SceneInstanceSync
{
public:
    // La base d'une source, relevée à l'instanciation puis après chaque fusion.
    void RememberSource(Engine::AssetHandle source, const nlohmann::json &sourceJson);
    bool KnowsSource(Engine::AssetHandle source) const;

    // Fusionne dans `scene` la nouvelle version d'une source. Renvoie false si cette
    // source n'a aucune instance ici, auquel cas la scène n'est pas touchée.
    //
    // La scène est relue en place depuis le JSON fusionné — le même objet Scene, vidé
    // puis rempli : rien de l'état précédent ne subsiste, et tout ce qui référence la
    // scène reste valide. Les UUID sont préservés, donc la sélection et l'historique
    // d'annulation continuent de désigner les mêmes entités.
    bool Refresh(const std::shared_ptr<Engine::Scene> &scene, Engine::AssetHandle source,
                 const nlohmann::json &newSourceJson);

private:
    // Entités ajoutées ou retirées dans la source depuis la base : traitées sur l'arbre
    // plutôt que sur le JSON, la hiérarchie y étant directement interrogeable.
    void ApplyStructuralChanges(const std::shared_ptr<Engine::Scene> &scene,
                                Engine::AssetHandle source, const nlohmann::json &newSourceJson);

    std::unordered_map<uint64_t, nlohmann::json> m_Bases;
};
