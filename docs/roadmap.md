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
- ✅ `SceneHierarchyPanel` — liste des entités de la scène
- ✅ `InspectorPanel` — édition des components d'une entité
- ✅ `ContentBrowserPanel` — navigation dans les assets
- ✅ `ConsolePanel` — affichage des logs spdlog dans l'UI
- ✅ Gizmos de transformation (translate, rotate, scale)
- ✅ Play / Pause / Stop depuis l'éditeur
- ✅ Raccourcis clavier standards de l'éditeur (Undo/Redo `Ctrl+Z`/`Ctrl+Y`, Copier/Coller `Ctrl+C`/`Ctrl+V`, Dupliquer `Ctrl+D`, Supprimer) — historique de commandes annulables (`Editor/src/Commands/`), doublé d'un menu "Édition"
- ✅ Tests automatisés de l'éditeur — mode sans fenêtre, captures PNG et suite de tests qui pilotent l'UI (Dear ImGui Test Engine), voir [tests.md](tests.md)
- ✅ Saisie des valeurs de l'Inspecteur directement au clavier (taper un chiffre plutôt que de devoir drag-slider)
- ✅ Bouton "Réinitialiser aux valeurs par défaut" dans l'Inspecteur (un par section de component)
- ✅ `GamePanel` — vue de jeu séparée du Viewport d'édition, ouvrable dans une fenêtre à part (façon Godot) ; l'Inspecteur reste éditable en direct pendant le Play (façon Unity)
- ⬜ Console — regroupement des messages identiques ("Collapse" façon Unity), avec un compteur sur la ligne
- ⬜ Console — trois boutons de filtre Log / Warning / Error, affichant le nombre de messages de chaque type
- ⬜ Catégories de log dans le moteur (`Collision`, `Input`, `Renderer`, `Physique`…, plus des catégories côté jeu) — prérequis des deux filtres ci-dessous, le moteur ne distingue aujourd'hui que le niveau, pas la provenance
- ⬜ Console — menu déroulant "Moteur" à cases à cocher, une par catégorie moteur
- ⬜ Console — menu déroulant "Jeu" à cases à cocher, une par catégorie définie par le jeu
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
- ✅ Debug-draw des colliders dans le Viewport (contours Box/Circle, superposés à la scène) — menu "Affichage", primitives `Renderer2D::DrawLine`/`DrawRect`/`DrawCircle` ajoutées au passage
- ✅ Bug corrigé : éditer RigidBody/BoxCollider/CircleCollider dans l'Inspecteur pendant le Play n'avait aucun effet — `PhysicsSystem` repousse maintenant les valeurs des components vers Box2D à chaque frame (`b2Body_SetType`, `b2Shape_SetPolygon`/`SetCircle`, `SetDensity`/`SetFriction`/`SetRestitution`) avant chaque `Step()`

---

## Phase 6 — Audio

- ⬜ Intégration **miniaudio** ou **OpenAL**
- ⬜ `AudioEngine` (init, shutdown)
- ⬜ `AudioSource` (play, stop, loop, volume)
- ⬜ `AudioComponent` dans l'ECS
- ⬜ Lecture depuis l'éditeur

---

## Phase 7 — Asset System

- ⬜ `AssetHandle` (UUID → asset)
- ⬜ `AssetManager` (registry + cache)
- ⬜ `TextureImporter`
- ⬜ `AudioImporter`
- ⬜ Hot-reload des assets en éditeur

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

