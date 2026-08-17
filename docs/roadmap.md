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
- ⬜ Intégration dans l'éditeur (raccourcis, navigation caméra) — nécessite l'Editor (Phase 4) ; navigation caméra déjà validée dans le Sandbox en attendant

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

---

## Phase 5 — Physics 2D

- ✅ Intégration **Box2D**
- ✅ `Physics2D` — wrapper Box2D (monde, step)
- ✅ `RigidBodyComponent` (`Static`, `Dynamic`, `Kinematic`)
- ✅ `BoxColliderComponent`
- ✅ `CircleColliderComponent`
- ✅ `PhysicsSystem` — synchronise Transform ↔ Box2D body
- ✅ Callbacks de collision

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

