## Légende

- ✅ Terminé
- 🔄 En cours
- ⬜ À faire

---

## Phase 0 — Setup & Architecture

- ✅ Structure des dossiers du projet
- ✅ CMakeLists.txt racine + sous-projets (Engine, Editor, Sandbox)
- ✅ Compilation de base (static lib Engine + executable Sandbox)
- ✅ Système de Layers (`Layer`, `LayerStack`)
- ✅ Système d'Events (`Event`, `KeyEvent`, `WindowEvent`)
- ✅ Abstraction Window (GLFW)
- ✅ Logger (`Log.h` avec spdlog)
- ✅ Clear couleur + swap buffers fonctionnel
- ✅ Boucle principale (`Application::Run()`)

---

## Phase 1 — Renderer de base

- ✅ Premier triangle (VAO / VBO / Shader OpenGL)
- ✅ Abstraction `Buffer` (VertexBuffer, IndexBuffer)
- ✅ Abstraction `VertexArray`
- ✅ Abstraction `Shader`
- ✅ `RenderCommand` (SetClearColor, Clear, DrawIndexed)
- ✅ `Renderer` (Submit, BeginScene, EndScene)
- ✅ Caméra orthographique (`OrthographicCamera`)
- ✅ Dessiner un quad coloré
- ✅ Chargement et affichage d'une texture (stb_image)
- ✅ `Renderer2D` — batch rendering (quads + textures)
- ✅ `SubTexture2D` — support sprite sheets

---

## Phase 2 — ECS & Scene

- ✅ Intégration **EnTT**
- ✅ `Entity` (wrapper autour d'un entt::entity)
- ✅ `Scene` (contient le registry EnTT)
- ✅ `Components.h` (`TransformComponent`, `SpriteRendererComponent`, `TagComponent`)
- ✅ `RenderSystem` (itère les entités et les dessine)
- ✅ `UUID` (identifiants uniques pour les entités)
- ✅ Sérialisation de scène en JSON ou YAML
- ✅ `SceneManager` (load / save / switch de scènes)
- ✅ Hiérarchie parent/enfant (`ParentComponent`) — le `TransformComponent` devient local au parent, `Scene::GetWorldTransform()`/`SetWorldTransform()` font le passage avec le repère du monde (rendu, physique, gizmo, picking, caméra) ; rattacher conserve la position dans le monde, les cycles sont refusés, supprimer un parent supprime ses enfants

---

## Phase 3 — Input System

- ✅ `Input` — polling clavier et souris (Input::IsKeyPressed, etc.)
- ✅ `KeyCodes.h` et `MouseCodes.h`
- ✅ Intégration dans l'éditeur (raccourcis, navigation caméra)

---

## Phase 4 — Editor

- ✅ Intégration **ImGui** (docking branch)
- ✅ `EditorLayer` — layer principal de l'éditeur
- ✅ `ViewportPanel` — rendu dans un Framebuffer affiché comme texture ImGui
- ✅ `Framebuffer` — abstraction OpenGL
- ✅ `SceneHierarchyPanel` — arbre des entités de la scène, coiffé par le nom de la scène ; glisser-déposer pour réordonner (bande du haut d'un nœud) ou rattacher (reste du nœud), retour à la racine en déposant sur la scène, le tout annulable
- ✅ `InspectorPanel` — édition des components d'une entité
- ✅ `ContentBrowserPanel` — navigation dans les assets
- ✅ `ConsolePanel` — affichage des logs spdlog dans l'UI
- ✅ Gizmos de transformation (translate, rotate, scale)
- ✅ Play / Pause / Stop depuis l'éditeur
- ✅ Raccourcis clavier standards de l'éditeur (Undo/Redo `Ctrl+Z`/`Ctrl+Y`, Copier/Coller `Ctrl+C`/`Ctrl+V`, Dupliquer `Ctrl+D`, Supprimer) — historique de commandes annulables (`Editor/src/Commands/`), doublé d'un menu "Édition"
- ✅ Tests automatisés de l'éditeur — mode sans fenêtre, captures PNG et suite de tests qui pilotent l'UI (Dear ImGui Test Engine), voir [tests.md](tests.md)
- ✅ Saisie des valeurs de l'Inspecteur directement au clavier (taper un chiffre plutôt que de devoir drag-slider)
- ✅ Bouton "Réinitialiser aux valeurs par défaut" dans l'Inspecteur (un par section de component)
- ✅ Sauvegarde de la scène depuis l'éditeur (`Ctrl+S`) — menu Fichier complet (Nouvelle scène `Ctrl+N`, Ouvrir `Ctrl+O`, Enregistrer `Ctrl+S`, Enregistrer sous `Ctrl+Maj+S`, Quitter) ; l'éditeur retient le chemin de la scène ouverte et l'affiche dans le titre de la fenêtre. Sélecteur de fichier dessiné en ImGui (pas de dialogue natif : aucune dépendance système, et pilotable par les tests), scènes rangées dans `assets/scenes` avec l'extension `.scene`
- ✅ `GamePanel` — vue de jeu séparée du Viewport d'édition, ouvrable dans une fenêtre à part (façon Godot) ; l'Inspecteur reste éditable en direct pendant le Play (façon Unity)
- ✅ Console — regroupement des messages identiques ("Collapse" façon Unity), avec un compteur sur la ligne
- ✅ Console — trois boutons de filtre Log / Warning / Error, affichant le nombre de messages de chaque type (+ coloration par niveau)
- ✅ Catégories de log dans le moteur (`Collision`, `Input`, `Renderer`, `Physique`…, plus des catégories côté jeu) — macros `ENGINE_LOG_*` / `GAME_LOG_*`, un logger spdlog par catégorie
- ✅ Console — menu déroulant "Moteur" à cases à cocher, une par catégorie moteur
- ✅ Console — menu déroulant "Jeu" à cases à cocher, une par catégorie définie par le jeu
- ✅ `CameraComponent` — caméra ECS (taille orthographique + flag Primary), utilisée par `GamePanel` pour rendre ce que voit la caméra principale de la scène (position/rotation via le `TransformComponent` de l'entité)

---

## Phase 5 — Physics 2D

- ✅ Intégration **Box2D**
- ✅ `Physics2D` — wrapper Box2D (monde, step)
- ✅ `RigidBodyComponent` (`Static`, `Dynamic`, `Kinematic`)
- ✅ `BoxColliderComponent`
- ✅ `CircleColliderComponent`
- ✅ `PhysicsSystem` — synchronise Transform ↔ Box2D body
- ✅ Callbacks de collision
- ✅ Debug-draw des colliders dans le Viewport (contours Box/Circle, superposés à la scène) — contour permanent de l'entité sélectionnée, plus une case "Affichage" pour toute la scène, primitives `Renderer2D::DrawLine`/`DrawRect`/`DrawCircle` ajoutées au passage
- ✅ Bug corrigé : éditer RigidBody/BoxCollider/CircleCollider dans l'Inspecteur pendant le Play n'avait aucun effet — `PhysicsSystem` repousse maintenant les valeurs des components vers Box2D à chaque frame (`b2Body_SetType`, `b2Shape_SetPolygon`/`SetCircle`, `SetDensity`/`SetFriction`/`SetRestitution`) avant chaque `Step()`

---

## Phase 6 — Audio

- ✅ Intégration **miniaudio** (retenu plutôt qu'OpenAL : pas de dépendance système, récupéré par FetchContent)
- ✅ `AudioEngine` (init, shutdown, volume global, repli sur le backend "null" sans périphérique)
- ✅ `AudioSource` (play, stop, pause, loop, volume)
- ✅ `AudioComponent` dans l'ECS (+ `AudioSystem` : chargement au Play, lecture au démarrage, relâchement au Stop)
- ✅ Lecture depuis l'éditeur (bouton Écouter/Arrêter dans l'Inspecteur, sans passer par le Play)

---

## Phase 7 — Asset System

- ✅ `AssetHandle` (UUID → asset)
- ✅ `AssetManager` (registre JSON persisté + cache)
- ✅ Import des textures (type déduit de l'extension, chargement à la demande et mise en cache)
- ✅ Import des sons (non mis en cache : chaque entité a besoin de sa propre lecture)
- ✅ Hot-reload des assets en éditeur (dates des fichiers relues quelques fois par seconde)
- ✅ Glisser-déposer d'un asset du Content Browser vers l'Inspecteur (type vérifié au dépôt)
- ✅ Registre d'assets versionné avec les sources (l'éditeur travaille directement sur `Editor/assets/`)
- 🔄 **Instanciation de scènes (prefabs)** — pouvoir réutiliser une branche d'entités (un ennemi, une plateforme) à plusieurs endroits, et la modifier à un seul endroit
  - **Modèle retenu (2026-08-21) : celui de Godot** — tout est une scène, et n'importe quelle scène peut être instanciée dans une autre. Pas de second concept ni de second format : le `.scene` existant sert de prefab. Écarté : le modèle Unity, où le prefab est un asset distinct de la scène (distinction plus nette entre « niveau » et « objet réutilisable », mais deux concepts à écrire, deux formats à sérialiser et un mode d'édition séparé)
  - ✅ Racine unique par scène — c'est elle qui devient l'entité instanciée. Une scène neuve naît avec sa racine, un fichier multi-racines est enveloppé au chargement, et la racine n'est pas supprimable depuis l'éditeur
  - ✅ Instancier sans lien : déposer une scène du Content Browser sur une entité de la hiérarchie y recopie sa branche, en enfant de la cible. L'instance reçoit de nouveaux UUID mais garde les transforms locaux de la source ; une scène refuse de s'instancier dans elle-même ; le tout est annulable
  - ✅ Garder le lien : `SceneInstanceComponent` retient l'`AssetHandle` de la source ; modifier le fichier source rafraîchit ses instances. Le niveau garde ce qui lui appartient (placement, nom, place dans la hiérarchie), le reste est refait à l'identique de la source. `.scene` est devenu un `AssetType`, et les dates des scènes sont suivies hors du cache
  - ✅ Surcharges par propriété : une instance peut diverger de sa source sur n'importe quelle valeur, et la divergence survit aux modifications de la source. Fusion à trois côtés sur la forme JSON des entités (celle du `SceneSerializer`, qui nomme déjà chaque propriété) : si l'instance s'écarte de ce qu'avait la source la dernière fois qu'on l'a vue, c'est une surcharge, sinon la nouvelle valeur passe
  - ⬜ Marquer visuellement les propriétés surchargées dans l'Inspecteur (façon liseré bleu d'Unity), et pouvoir en révoquer une
  - ⬜ Retirer un component sur une instance : la fusion le fait revenir, faute de retenir les suppressions

---

## Phase 8 — Runtime & Build

- ⬜ `Runtime/` — exécutable standalone sans éditeur
- ⬜ Chargement d'une scène sérialisée au démarrage
- ⬜ `Games/MyFirstGame/` — premier vrai jeu utilisant le moteur
- ⬜ Packaging / export du jeu

---

## Phase 9 — Scripting *(optionnel / avancé)*

- ⬜ Choix du langage : **Lua** ou **C# (Mono)**
- ⬜ `ScriptEngine` — chargement et exécution des scripts
- ⬜ Binding des components exposés au scripting
- ⬜ Rechargement à chaud des scripts en éditeur

