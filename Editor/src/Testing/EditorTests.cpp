#include "Testing/EditorTests.h"

#ifdef MYENGINE_EDITOR_TESTS

#include "EditorLayer.h"
#include <Core/Log.h>
#include <Assets/AssetManager.h>
#include <Scene/Components.h>
#include <Scene/SceneSerializer.h>
#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_te_engine.h>
#include <imgui_te_context.h>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <string>

namespace
{
    // Les widgets de l'Inspecteur s'adressent par leur chemin dans la pile d'ID ImGui :
    // "<Section>/<Label>", plus "/$$n" pour la n-ième composante d'un DragFloat2/3.
    // Le bouton de réinitialisation, lui, porte un ID suffixé par le nom de sa section
    // (voir BeginComponentSection dans InspectorPanel.cpp).
    std::string ResetButtonOf(const char *section)
    {
        return std::string(section) + "/Reset##" + section;
    }

    // Les entités de la hiérarchie tirent leur ID ImGui de leur handle et non de leur
    // nom (SceneHierarchyPanel::DrawEntityNode) — deux entités peuvent porter le même
    // nom. On récupère donc la liste des items du panel pour retrouver le bon par label.
    Engine::Entity SelectEntity(ImGuiTestContext *ctx, EditorLayer &editor, const char *name)
    {
        ctx->SetRef("Scene Hierarchy");

        ImGuiTestItemList items;
        ctx->GatherItems(&items, "");

        for (const ImGuiTestItemInfo &item : items)
        {
            if (strcmp(item.DebugLabel, name) == 0)
            {
                ctx->ItemClick(item.ID);
                return editor.GetSelectedEntityForTests();
            }
        }

        ctx->LogError("Entity '%s' not found in the hierarchy", name);
        return {};
    }

    bool NearlyEqual(float a, float b) { return std::fabs(a - b) < 0.0001f; }

    int CountEntities(EditorLayer &editor)
    {
        int count = 0;
        for (auto entityHandle : editor.GetEditorScene().GetAllEntitiesWith<Engine::IDComponent>())
        {
            (void)entityHandle;
            ++count;
        }
        return count;
    }
}

void RegisterEditorTests(ImGuiTestEngine *engine, EditorLayer &editor)
{
    ImGuiTest *t = nullptr;

    // --- Hiérarchie ---------------------------------------------------------

    t = IM_REGISTER_TEST(engine, "hierarchy", "select_entity");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        Engine::Entity square = SelectEntity(ctx, editor, "Square");
        IM_CHECK(square);
        IM_CHECK_STR_EQ(square.GetName().c_str(), "Square");

        Engine::Entity camera = SelectEntity(ctx, editor, "Main Camera");
        IM_CHECK(camera);
        IM_CHECK_STR_EQ(camera.GetName().c_str(), "Main Camera");
    };

    // --- Inspecteur ---------------------------------------------------------

    // Régression : les DragFloat doivent accepter une valeur tapée au clavier
    // (io.ConfigDragClickToInputText), pas seulement un glissement de souris.
    t = IM_REGISTER_TEST(engine, "inspector", "type_value_with_keyboard");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        Engine::Entity square = SelectEntity(ctx, editor, "Square");
        IM_CHECK(square);
        auto &transform = square.GetComponent<Engine::TransformComponent>();
        const Engine::TransformComponent saved = transform;

        ctx->SetRef("Inspector");
        ctx->ItemInputValue("Transform/Position/$$0", 2.5f);
        IM_CHECK(NearlyEqual(transform.Position.x, 2.5f));

        ctx->ItemInputValue("Transform/Rotation", 45.0f);
        IM_CHECK(NearlyEqual(transform.Rotation, 45.0f));

        transform = saved;
    };

    // Régression : une valeur tapée doit rester dans les bornes du widget
    // (ImGuiSliderFlags_AlwaysClamp) — sinon un rayon négatif partirait dans Box2D.
    t = IM_REGISTER_TEST(engine, "inspector", "typed_value_stays_clamped");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        Engine::Entity circle = SelectEntity(ctx, editor, "Circle");
        IM_CHECK(circle);
        auto &collider = circle.GetComponent<Engine::CircleColliderComponent>();
        const Engine::CircleColliderComponent saved = collider;

        ctx->SetRef("Inspector");
        ctx->ItemInputValue("Circle Collider/Radius", -5.0f);
        IM_CHECK_GT(collider.Radius, 0.0f);

        ctx->ItemInputValue("Circle Collider/Friction", 999.0f);
        IM_CHECK_LE(collider.Friction, 1.0f);

        collider = saved;
    };

    t = IM_REGISTER_TEST(engine, "inspector", "reset_transform_to_defaults");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        Engine::Entity square = SelectEntity(ctx, editor, "Square");
        IM_CHECK(square);
        auto &transform = square.GetComponent<Engine::TransformComponent>();
        const Engine::TransformComponent saved = transform;

        transform.Position = {3.0f, 4.0f, 0.0f};
        transform.Rotation = 30.0f;
        transform.Scale = {2.0f, 2.0f, 1.0f};

        ctx->SetRef("Inspector");
        ctx->ItemClick(ResetButtonOf("Transform").c_str());

        const Engine::TransformComponent defaults;
        IM_CHECK(NearlyEqual(transform.Position.x, defaults.Position.x));
        IM_CHECK(NearlyEqual(transform.Position.y, defaults.Position.y));
        IM_CHECK(NearlyEqual(transform.Rotation, defaults.Rotation));
        IM_CHECK(NearlyEqual(transform.Scale.x, defaults.Scale.x));

        transform = saved;
    };

    // Le reset doit préserver le handle Box2D du corps en cours de simulation :
    // l'écraser ferait disparaître le corps en plein Play. Le test se déroule donc
    // pendant le Play — en édition les handles sont nuls et la vérification serait vide.
    t = IM_REGISTER_TEST(engine, "inspector", "reset_keeps_box2d_handle");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        ctx->SetRef("DockSpace");
        ctx->ItemClick("**/Play");
        IM_CHECK(editor.IsPlayingForTests());
        ctx->Yield(5);

        Engine::Entity square = SelectEntity(ctx, editor, "Square");
        IM_CHECK(square);
        auto &rb = square.GetComponent<Engine::RigidBodyComponent>();
        IM_CHECK(b2Body_IsValid(rb.RuntimeBody));
        const b2BodyId handleBefore = rb.RuntimeBody;

        rb.FixedRotation = true;

        ctx->SetRef("Inspector");
        ctx->ItemClick(ResetButtonOf("Rigid Body").c_str());

        IM_CHECK_EQ(rb.FixedRotation, false);
        IM_CHECK(b2Body_IsValid(rb.RuntimeBody));
        IM_CHECK_EQ(rb.RuntimeBody.index1, handleBefore.index1);
        IM_CHECK_EQ(rb.RuntimeBody.generation, handleBefore.generation);

        ctx->SetRef("DockSpace");
        ctx->ItemClick("**/Stop");
    };

    // --- Play / Stop --------------------------------------------------------

    // Régression sur la séparation scène d'édition / scène runtime : le Play simule
    // sur une copie, et le Stop doit rendre la scène d'édition intacte.
    t = IM_REGISTER_TEST(engine, "runtime", "play_then_stop_restores_scene");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        Engine::Entity square = SelectEntity(ctx, editor, "Square");
        IM_CHECK(square);
        const float startY = square.GetComponent<Engine::TransformComponent>().Position.y;

        ctx->SetRef("DockSpace");
        ctx->ItemClick("**/Play");
        IM_CHECK(editor.IsPlayingForTests());

        // Le carré est Dynamic : la gravité doit l'avoir fait descendre.
        ctx->Yield(60);
        Engine::Entity runtimeSquare = editor.GetSelectedEntityForTests();
        IM_CHECK(runtimeSquare);
        IM_CHECK_LT(runtimeSquare.GetComponent<Engine::TransformComponent>().Position.y, startY);

        ctx->ItemClick("**/Stop");
        IM_CHECK(!editor.IsPlayingForTests());

        // Après le Stop, la scène d'édition doit être telle qu'avant le Play.
        Engine::Entity editorSquare = SelectEntity(ctx, editor, "Square");
        IM_CHECK(editorSquare);
        IM_CHECK(NearlyEqual(editorSquare.GetComponent<Engine::TransformComponent>().Position.y, startY));
    };

    // --- Raccourcis d'édition ------------------------------------------------

    t = IM_REGISTER_TEST(engine, "edit", "duplicate_then_undo");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        const int before = CountEntities(editor);
        Engine::Entity square = SelectEntity(ctx, editor, "Square");
        IM_CHECK(square);

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_D);
        IM_CHECK_EQ(CountEntities(editor), before + 1);
        // La duplication sélectionne la copie, qui porte le même nom mais un autre UUID.
        Engine::Entity duplicate = editor.GetSelectedEntityForTests();
        IM_CHECK(duplicate);
        IM_CHECK_STR_EQ(duplicate.GetName().c_str(), "Square");
        IM_CHECK(duplicate.GetUUID() != square.GetUUID());

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        IM_CHECK_EQ(CountEntities(editor), before);

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Y);
        IM_CHECK_EQ(CountEntities(editor), before + 1);

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        IM_CHECK_EQ(CountEntities(editor), before);
    };

    // Annuler une suppression doit rendre l'entité intacte : même identité et mêmes
    // valeurs de components, sinon toute référence à celle-ci pointe dans le vide.
    t = IM_REGISTER_TEST(engine, "edit", "delete_then_undo_restores_entity");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        const int before = CountEntities(editor);
        Engine::Entity circle = SelectEntity(ctx, editor, "Circle");
        IM_CHECK(circle);

        const Engine::UUID uuid = circle.GetUUID();
        const float radius = circle.GetComponent<Engine::CircleColliderComponent>().Radius;
        const glm::vec3 position = circle.GetComponent<Engine::TransformComponent>().Position;

        ctx->KeyPress(ImGuiKey_Delete);
        IM_CHECK_EQ(CountEntities(editor), before - 1);
        IM_CHECK(!editor.GetEditorScene().FindEntityByUUID(uuid));

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        IM_CHECK_EQ(CountEntities(editor), before);

        Engine::Entity restored = editor.GetEditorScene().FindEntityByUUID(uuid);
        IM_CHECK(restored);
        IM_CHECK(restored.HasComponent<Engine::CircleColliderComponent>());
        IM_CHECK(NearlyEqual(restored.GetComponent<Engine::CircleColliderComponent>().Radius, radius));
        IM_CHECK(NearlyEqual(restored.GetComponent<Engine::TransformComponent>().Position.x, position.x));
        IM_CHECK(NearlyEqual(restored.GetComponent<Engine::TransformComponent>().Position.y, position.y));
    };

    t = IM_REGISTER_TEST(engine, "edit", "copy_paste_then_undo");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        const int before = CountEntities(editor);
        Engine::Entity ground = SelectEntity(ctx, editor, "Ground");
        IM_CHECK(ground);
        const glm::vec3 position = ground.GetComponent<Engine::TransformComponent>().Position;

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_C);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_V);
        IM_CHECK_EQ(CountEntities(editor), before + 1);

        Engine::Entity pasted = editor.GetSelectedEntityForTests();
        IM_CHECK(pasted);
        IM_CHECK_STR_EQ(pasted.GetName().c_str(), "Ground");
        IM_CHECK(NearlyEqual(pasted.GetComponent<Engine::TransformComponent>().Position.y, position.y));

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        IM_CHECK_EQ(CountEntities(editor), before);
    };

    // Une édition de l'Inspecteur doit être annulable, et un drag entier ne doit
    // compter que pour une seule annulation (la commande n'est empilée qu'au
    // relâchement du widget, pas à chaque frame).
    t = IM_REGISTER_TEST(engine, "edit", "undo_inspector_edit");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        Engine::Entity square = SelectEntity(ctx, editor, "Square");
        IM_CHECK(square);
        const float startX = square.GetComponent<Engine::TransformComponent>().Position.x;

        ctx->SetRef("Inspector");
        ctx->ItemInputValue("Transform/Position/$$0", 4.0f);
        IM_CHECK(NearlyEqual(square.GetComponent<Engine::TransformComponent>().Position.x, 4.0f));

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        IM_CHECK(NearlyEqual(square.GetComponent<Engine::TransformComponent>().Position.x, startX));

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Y);
        IM_CHECK(NearlyEqual(square.GetComponent<Engine::TransformComponent>().Position.x, 4.0f));

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        IM_CHECK(NearlyEqual(square.GetComponent<Engine::TransformComponent>().Position.x, startX));
    };

    t = IM_REGISTER_TEST(engine, "edit", "undo_inspector_reset");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        Engine::Entity square = SelectEntity(ctx, editor, "Square");
        IM_CHECK(square);
        const glm::vec3 startScale = square.GetComponent<Engine::TransformComponent>().Scale;

        ctx->SetRef("Inspector");
        ctx->ItemClick(ResetButtonOf("Transform").c_str());
        IM_CHECK(NearlyEqual(square.GetComponent<Engine::TransformComponent>().Scale.x, 1.0f));

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        IM_CHECK(NearlyEqual(square.GetComponent<Engine::TransformComponent>().Scale.x, startScale.x));
    };

    // --- Console --------------------------------------------------------------

    t = IM_REGISTER_TEST(engine, "console", "severity_filters_and_collapse");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        // Deux avertissements strictement identiques : c'est ce que le regroupement
        // doit fusionner en une seule ligne.
        LOG_WARN("Console test warning");
        LOG_WARN("Console test warning");
        LOG_ERROR("Console test error");
        ctx->Yield(3);

        ConsolePanel &console = editor.GetConsolePanelForTests();
        const size_t withEverything = console.GetVisibleLineCountForTests();
        IM_CHECK_GT(withEverything, (size_t)0);

        // Les comptes sont relatifs : les tests précédents ont déjà rempli la console.
        ctx->SetRef("Console");
        ctx->ItemClick("###Warning");
        ctx->Yield(2);
        IM_CHECK_LE(console.GetVisibleLineCountForTests(), withEverything - 2);

        ctx->ItemClick("###Warning");
        ctx->Yield(2);
        IM_CHECK_EQ(console.GetVisibleLineCountForTests(), withEverything);

        ctx->ItemClick("Collapse");
        ctx->Yield(2);
        IM_CHECK(console.IsCollapsedForTests());
        // Au minimum, le doublon d'avertissement a fusionné.
        IM_CHECK_LT(console.GetVisibleLineCountForTests(), withEverything);

        const char *outputFile = "output/captures/console.png";
        ctx->CaptureReset();
        ImStrncpy(ctx->CaptureArgs->InOutputFile, outputFile, IM_ARRAYSIZE(ctx->CaptureArgs->InOutputFile));
        IM_CHECK(ctx->CaptureAddWindow("//Console"));
        IM_CHECK(ctx->CaptureScreenshot());

        ctx->ItemClick("Collapse");
        IM_CHECK(!console.IsCollapsedForTests());
    };

    // Les catégories permettent de filtrer par provenance : deux menus déroulants,
    // l'un pour le moteur, l'autre pour le jeu.
    t = IM_REGISTER_TEST(engine, "console", "category_filters");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        ENGINE_LOG_INFO(Engine::LogCategories::Collision, "test collision");
        GAME_LOG_INFO("Gameplay", "test game message");
        ctx->Yield(3);

        ConsolePanel &console = editor.GetConsolePanelForTests();
        const size_t withEverything = console.GetVisibleLineCountForTests();

        ctx->SetRef("Console");
        // Le menu "Engine" liste les catégories du moteur, pré-enregistrées à l'Init.
        ctx->ItemClick("Engine");
        ctx->ItemUncheck("//$FOCUSED/Collision");
        // Fermer explicitement : tant que le popup est ouvert il recouvre le menu
        // voisin, qui devient impossible à survoler.
        ctx->PopupCloseAll();
        ctx->Yield(2);

        const size_t withoutCollisions = console.GetVisibleLineCountForTests();
        IM_CHECK_LT(withoutCollisions, withEverything);

        // Une catégorie du jeu n'a rien à faire dans le menu du moteur, et inversement.
        ctx->ItemClick("Game");
        IM_CHECK(ctx->ItemExists("//$FOCUSED/Gameplay"));
        IM_CHECK(!ctx->ItemExists("//$FOCUSED/Collision"));
        ctx->PopupCloseAll();

        const char *outputFile = "output/captures/console_categories.png";
        ctx->CaptureReset();
        ImStrncpy(ctx->CaptureArgs->InOutputFile, outputFile, IM_ARRAYSIZE(ctx->CaptureArgs->InOutputFile));
        IM_CHECK(ctx->CaptureAddWindow("//Console"));
        IM_CHECK(ctx->CaptureScreenshot());

        ctx->ItemClick("Engine");
        ctx->ItemCheck("//$FOCUSED/Collision");
        ctx->PopupCloseAll();
        ctx->Yield(2);
        IM_CHECK_EQ(console.GetVisibleLineCountForTests(), withEverything);
    };

    // Régression : le regroupement compare le message brut et non la ligne formatée.
    // Deux occurrences émises à des secondes différentes n'ont pas le même horodatage,
    // donc pas la même ligne — et ne fusionnaient pas.
    t = IM_REGISTER_TEST(engine, "console", "collapse_ignores_timestamp");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        ConsolePanel &console = editor.GetConsolePanelForTests();
        if (!console.IsCollapsedForTests())
        {
            ctx->SetRef("Console");
            ctx->ItemClick("Collapse");
        }
        ctx->Yield(2);
        const size_t before = console.GetVisibleLineCountForTests();

        LOG_WARN("Message repeated across two seconds");
        // L'horodatage a une précision d'une seconde : il faut vraiment franchir un
        // tic d'horloge pour que les deux lignes formatées diffèrent.
        ctx->SleepNoSkip(1.2f, 0.1f);
        LOG_WARN("Message repeated across two seconds");
        ctx->Yield(3);

        // Une seule ligne de plus, portant deux occurrences.
        IM_CHECK_EQ(console.GetVisibleLineCountForTests(), before + 1);

        ctx->SetRef("Console");
        ctx->ItemClick("Collapse");
        IM_CHECK(!console.IsCollapsedForTests());
    };

    // --- Audio ----------------------------------------------------------------

    t = IM_REGISTER_TEST(engine, "audio", "plays_on_runtime_start");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        // La sélection est re-résolue par UUID dans la scène runtime au moment du Play :
        // c'est ce qui donne accès au component de la scène réellement simulée.
        Engine::Entity bip = SelectEntity(ctx, editor, "Bip");
        IM_CHECK(bip);
        IM_CHECK(bip.HasComponent<Engine::AudioComponent>());
        IM_CHECK(!bip.GetComponent<Engine::AudioComponent>().Source);

        // La scène de démo laisse le bip muet au lancement pour ne pas sonner à chaque
        // Play : c'est donc au test d'activer ce qu'il veut vérifier.
        auto &editorAudio = bip.GetComponent<Engine::AudioComponent>();
        const bool previousPlayOnStart = editorAudio.PlayOnStart;
        editorAudio.PlayOnStart = true;

        ctx->SetRef("DockSpace");
        ctx->ItemClick("**/Play");
        IM_CHECK(editor.IsPlayingForTests());
        ctx->Yield(3);

        Engine::Entity runtimeBip = editor.GetSelectedEntityForTests();
        IM_CHECK(runtimeBip);
        auto &audio = runtimeBip.GetComponent<Engine::AudioComponent>();
        IM_CHECK(audio.Source != nullptr);
        IM_CHECK(audio.Source->IsLoaded());
        IM_CHECK(audio.Source->IsPlaying());

        ctx->ItemClick("**/Stop");
        IM_CHECK(!editor.IsPlayingForTests());

        // Le son est relâché à l'arrêt : la scène d'édition n'en garde aucun.
        Engine::Entity editorBip = SelectEntity(ctx, editor, "Bip");
        IM_CHECK(editorBip);
        IM_CHECK(!editorBip.GetComponent<Engine::AudioComponent>().Source);

        editorBip.GetComponent<Engine::AudioComponent>().PlayOnStart = previousPlayOnStart;
    };

    // "Playback from the editor" : écouter un son sans passer par le Play.
    t = IM_REGISTER_TEST(engine, "audio", "preview_from_inspector");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        Engine::Entity bip = SelectEntity(ctx, editor, "Bip");
        IM_CHECK(bip);

        ctx->SetRef("Inspector");
        ctx->ItemClick("Audio/Play");
        ctx->Yield(2);

        auto &audio = bip.GetComponent<Engine::AudioComponent>();
        IM_CHECK(audio.Source != nullptr);
        IM_CHECK(audio.Source->IsLoaded());
        IM_CHECK(audio.Source->IsPlaying());

        const char *outputFile = "output/captures/inspector_audio.png";
        ctx->CaptureReset();
        ImStrncpy(ctx->CaptureArgs->InOutputFile, outputFile, IM_ARRAYSIZE(ctx->CaptureArgs->InOutputFile));
        IM_CHECK(ctx->CaptureAddWindow("//Inspector"));
        IM_CHECK(ctx->CaptureScreenshot());

        ctx->ItemClick("Audio/Stop");
        ctx->Yield(2);
        IM_CHECK(!audio.Source->IsPlaying());
    };

    // --- Assets ---------------------------------------------------------------

    // La texture d'un sprite ne survivait pas à une sauvegarde : c'était un shared_ptr,
    // impossible à écrire dans un fichier. C'est maintenant une référence d'asset.
    t = IM_REGISTER_TEST(engine, "assets", "texture_reference_survives_save_load");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        Engine::Entity square = SelectEntity(ctx, editor, "Square");
        IM_CHECK(square);

        auto &sprite = square.GetComponent<Engine::SpriteRendererComponent>();
        const Engine::AssetHandle previous = sprite.Texture;

        const Engine::AssetHandle texture = Engine::AssetManager::Import("textures/axololt.jpg");
        IM_CHECK((uint64_t)texture != Engine::k_InvalidAssetHandle);
        IM_CHECK(Engine::AssetManager::GetType(texture) == Engine::AssetType::Texture);
        sprite.Texture = texture;

        const std::string file = "output/scene_roundtrip.json";
        const Engine::UUID squareId = square.GetUUID();
        {
            std::error_code ec;
            std::filesystem::create_directories("output", ec);
            auto scene = std::make_shared<Engine::Scene>();
            Engine::Scene::CopyComponents(square, scene->CreateEntityWithUUID(squareId, square.GetName()));
            Engine::SceneSerializer(scene).Serialize(file);
        }

        auto reloaded = std::make_shared<Engine::Scene>();
        IM_CHECK(Engine::SceneSerializer(reloaded).Deserialize(file));

        Engine::Entity restored = reloaded->FindEntityByUUID(squareId);
        IM_CHECK(restored);
        IM_CHECK(restored.HasComponent<Engine::SpriteRendererComponent>());
        IM_CHECK_EQ((uint64_t)restored.GetComponent<Engine::SpriteRendererComponent>().Texture,
                    (uint64_t)texture);

        sprite.Texture = previous;
    };

    // Le rechargement à chaud repose sur la date de modification du fichier.
    //
    // Deux précautions liées au fait que le corps d'un test tourne dans la coroutine du
    // Test Engine, donc sur un autre thread que celui qui possède le contexte OpenGL :
    // le rechargement est déclenché par la boucle de l'éditeur et non appelé ici, et le
    // test n'garde aucune référence sur la texture — la relâcher détruirait une ressource
    // GPU depuis le mauvais thread. D'où l'observation par un simple compteur.
    t = IM_REGISTER_TEST(engine, "assets", "hot_reload_reloads_modified_texture");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        Engine::Entity square = SelectEntity(ctx, editor, "Square");
        IM_CHECK(square);
        auto &sprite = square.GetComponent<Engine::SpriteRendererComponent>();
        const Engine::AssetHandle previous = sprite.Texture;

        // Assigner la texture suffit à la faire charger : RenderSystem la résout à la
        // frame suivante, sur le thread de rendu.
        sprite.Texture = Engine::AssetManager::Import("textures/axololt.jpg");
        ctx->Yield(3);

        const uint64_t reloadsBefore = Engine::AssetManager::GetReloadCount();

        // Rien à modifier dans l'image : seule sa date compte pour la détection.
        std::filesystem::last_write_time(Engine::AssetManager::GetAssetRoot() / "textures/axololt.jpg",
                                         std::filesystem::file_time_type::clock::now());

        // L'éditeur ne consulte le disque que quelques fois par seconde, et son minuteur
        // avance au temps réel : on attend le rechargement au lieu de parier sur un délai.
        uint64_t reloadsAfter = reloadsBefore;
        for (int attempt = 0; attempt < 20 && reloadsAfter == reloadsBefore; ++attempt)
        {
            ctx->SleepNoSkip(0.1f, 0.02f);
            reloadsAfter = Engine::AssetManager::GetReloadCount();
        }
        IM_CHECK_GT(reloadsAfter, reloadsBefore);

        sprite.Texture = previous;
    };

    // Glisser un fichier du Content Browser sur un champ d'asset de l'Inspecteur.
    t = IM_REGISTER_TEST(engine, "assets", "drag_and_drop_texture_onto_sprite");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        Engine::Entity square = SelectEntity(ctx, editor, "Square");
        IM_CHECK(square);
        auto &sprite = square.GetComponent<Engine::SpriteRendererComponent>();
        const Engine::AssetHandle previous = sprite.Texture;
        sprite.Texture = Engine::AssetHandle(Engine::k_InvalidAssetHandle);

        ctx->SetRef("Content Browser");
        ctx->ItemOpen("textures");
        ctx->Yield();

        ctx->ItemDragAndDrop("textures/axololt.jpg", "//Inspector/Sprite Renderer/Texture");
        ctx->Yield(2);

        IM_CHECK((uint64_t)sprite.Texture != Engine::k_InvalidAssetHandle);
        IM_CHECK_STR_EQ(Engine::AssetManager::GetPath(sprite.Texture).c_str(), "textures/axololt.jpg");

        // Un dépôt du mauvais type doit être refusé plutôt que d'installer une
        // référence invalide.
        const Engine::AssetHandle texture = sprite.Texture;
        ctx->SetRef("Content Browser");
        ctx->ItemOpen("audio");
        ctx->Yield();
        ctx->ItemDragAndDrop("audio/bip.wav", "//Inspector/Sprite Renderer/Texture");
        ctx->Yield(2);
        IM_CHECK_EQ((uint64_t)sprite.Texture, (uint64_t)texture);

        sprite.Texture = previous;
    };

    // --- Menu Fichier -------------------------------------------------------

    // Cycle complet : enregistrer sous, repartir d'une scène vide, rouvrir le fichier.
    // Le test se termine sur la scène de démo relue depuis son propre enregistrement,
    // donc les tests suivants y retrouvent bien leurs entités.
    t = IM_REGISTER_TEST(engine, "scene", "save_new_and_open_from_file_menu");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        const std::filesystem::path scenePath =
            Engine::AssetManager::GetAssetRoot() / "scenes" / "test_file_menu.scene";
        std::filesystem::remove(scenePath);

        // Une valeur reconnaissable : elle prouve que c'est bien ce fichier-là qui revient.
        Engine::Entity square = SelectEntity(ctx, editor, "Square");
        IM_CHECK(square);
        const Engine::TransformComponent savedTransform = square.GetComponent<Engine::TransformComponent>();
        square.GetComponent<Engine::TransformComponent>().Position.x = 3.25f;
        const int entityCount = CountEntities(editor);
        IM_CHECK_GT(entityCount, 0);

        ctx->SetRef("DockSpace");
        ctx->MenuClick("File/Save As...");
        ctx->Yield();

        ctx->SetRef("Save Scene As");
        // KeyCharsReplace et pas ItemInputValue : ce dernier valide avec Entrée, ce qui
        // fermerait la boîte avant qu'on ait pu cliquer le bouton qu'on veut tester.
        ctx->ItemClick("##Name");
        ctx->KeyCharsReplace("test_file_menu");
        ctx->ItemClick("Save");
        ctx->Yield(2);

        IM_CHECK(std::filesystem::exists(scenePath));
        IM_CHECK_STR_EQ(editor.GetCurrentScenePathForTests().filename().string().c_str(),
                        "test_file_menu.scene");

        // Une nouvelle scène repart de zéro et oublie le chemin : sans ça, le Ctrl+S
        // suivant écraserait le fichier de la scène précédente.
        ctx->SetRef("DockSpace");
        ctx->MenuClick("File/New Scene");
        ctx->Yield(2);
        IM_CHECK_EQ(CountEntities(editor), 0);
        IM_CHECK(editor.GetCurrentScenePathForTests().empty());

        ctx->MenuClick("File/Open...");
        ctx->Yield();

        // La liste vit dans une fenêtre enfant ImGui, dont le vrai nom est mangled
        // ("Popup/Files_XXXXXXXX") : WindowInfo est le seul moyen de la désigner,
        // un chemin "Open Scene/Files" ne se résoudrait pas.
        ctx->SetRef(ctx->WindowInfo("//Open Scene/Files").Window);
        ctx->ItemClick("test_file_menu.scene");
        ctx->SetRef("Open Scene");
        ctx->ItemClick("Open");
        ctx->Yield(2);

        IM_CHECK_EQ(CountEntities(editor), entityCount);
        Engine::Entity reloaded = SelectEntity(ctx, editor, "Square");
        IM_CHECK(reloaded);
        IM_CHECK(NearlyEqual(reloaded.GetComponent<Engine::TransformComponent>().Position.x, 3.25f));

        // Ctrl+S sur une scène qui a déjà un chemin : réécrit au même endroit, sans
        // rouvrir la moindre boîte de dialogue.
        std::filesystem::remove(scenePath);
        ctx->SetRef("DockSpace");
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_S);
        ctx->Yield(2);
        IM_CHECK(std::filesystem::exists(scenePath));

        reloaded.GetComponent<Engine::TransformComponent>() = savedTransform;
        std::filesystem::remove(scenePath);
    };

    // --- Captures -----------------------------------------------------------

    // Ce test existe surtout pour produire une image à regarder, mais il vérifie que
    // le fichier est bien écrit : sans IMGUI_TEST_ENGINE_ENABLE_CAPTURE, le Test Engine
    // ignore la capture en silence et le test passerait sans rien produire.
    t = IM_REGISTER_TEST(engine, "capture", "inspector_panel");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        SelectEntity(ctx, editor, "Square");

        const char *outputFile = "output/captures/inspector_panel.png";
        ctx->CaptureReset();
        ImStrncpy(ctx->CaptureArgs->InOutputFile, outputFile, IM_ARRAYSIZE(ctx->CaptureArgs->InOutputFile));
        IM_CHECK(ctx->CaptureAddWindow("//Inspector"));
        IM_CHECK(ctx->CaptureScreenshot());
        IM_CHECK(std::filesystem::exists(outputFile));
    };
    // La boîte de dialogue de scène, à regarder : elle est dessinée en ImGui, donc
    // c'est la seule façon de vérifier qu'elle est lisible et bien proportionnée.
    t = IM_REGISTER_TEST(engine, "capture", "scene_file_dialog");
    t->TestFunc = [](ImGuiTestContext *ctx)
    {
        ctx->SetRef("DockSpace");
        ctx->MenuClick("File/Save As...");
        ctx->Yield(2);

        const char *outputFile = "output/captures/scene_file_dialog.png";
        ctx->CaptureReset();
        ImStrncpy(ctx->CaptureArgs->InOutputFile, outputFile, IM_ARRAYSIZE(ctx->CaptureArgs->InOutputFile));
        IM_CHECK(ctx->CaptureAddWindow("//Save Scene As"));
        // Sans IncludeOtherWindows, le Test Engine masque toutes les autres fenêtres
        // pendant la capture — y compris celle qui a ouvert la modale, ce qui referme
        // la modale et ne laisse que le décor derrière sur l'image.
        ctx->CaptureArgs->InFlags |= ImGuiCaptureFlags_IncludeOtherWindows;
        IM_CHECK(ctx->CaptureScreenshot());
        IM_CHECK(std::filesystem::exists(outputFile));

        ctx->SetRef("Save Scene As");
        ctx->ItemClick("Cancel");
    };
    // Le Viewport avec les contours de colliders activés : sert de vérification
    // visuelle qu'ils épousent bien la géométrie envoyée à Box2D.
    t = IM_REGISTER_TEST(engine, "capture", "collider_outlines");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        ctx->SetRef("DockSpace");
        ctx->MenuCheck("View/All collider outlines");
        IM_CHECK(editor.AreColliderOutlinesVisibleForTests());
        ctx->Yield(2);

        const char *outputFile = "output/captures/collider_outlines.png";
        ctx->CaptureReset();
        ImStrncpy(ctx->CaptureArgs->InOutputFile, outputFile, IM_ARRAYSIZE(ctx->CaptureArgs->InOutputFile));
        IM_CHECK(ctx->CaptureAddWindow("//Viewport"));
        IM_CHECK(ctx->CaptureScreenshot());
        IM_CHECK(std::filesystem::exists(outputFile));

        ctx->MenuUncheck("View/All collider outlines");
        IM_CHECK(!editor.AreColliderOutlinesVisibleForTests());
    };
    // Sans la case globale, seule l'entité sélectionnée montre son collider.
    t = IM_REGISTER_TEST(engine, "capture", "selected_collider_outline");
    t->TestFunc = [&editor](ImGuiTestContext *ctx)
    {
        ctx->SetRef("DockSpace");
        ctx->MenuUncheck("View/All collider outlines");
        IM_CHECK(!editor.AreColliderOutlinesVisibleForTests());
        // Le menu resté ouvert recouvre la hiérarchie et empêche d'y cliquer.
        ctx->PopupCloseAll();
        ctx->Yield(2);

        Engine::Entity circle = SelectEntity(ctx, editor, "Circle");
        IM_CHECK(circle);
        ctx->Yield(2);

        const char *outputFile = "output/captures/selected_collider.png";
        ctx->CaptureReset();
        ImStrncpy(ctx->CaptureArgs->InOutputFile, outputFile, IM_ARRAYSIZE(ctx->CaptureArgs->InOutputFile));
        IM_CHECK(ctx->CaptureAddWindow("//Viewport"));
        IM_CHECK(ctx->CaptureScreenshot());
    };
}

#else

void RegisterEditorTests(ImGuiTestEngine *, EditorLayer &) {}

#endif
