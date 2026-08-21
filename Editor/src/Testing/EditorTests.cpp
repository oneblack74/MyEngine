#include "Testing/EditorTests.h"

#ifdef MYENGINE_EDITOR_TESTS

#include "EditorLayer.h"
#include <Scene/Components.h>
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
        return std::string(section) + "/Réinit.##" + section;
    }

    // Les entités de la hiérarchie tirent leur ID ImGui de leur handle et non de leur
    // nom (SceneHierarchyPanel::DrawEntityNode) — deux entités peuvent porter le même
    // nom. On récupère donc la liste des items du panel pour retrouver le bon par label.
    Engine::Entity SelectEntity(ImGuiTestContext *ctx, EditorLayer &editor, const char *name)
    {
        ctx->SetRef("Hiérarchie de la scène");

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

        ctx->LogError("Entité '%s' introuvable dans la hiérarchie", name);
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

        ctx->SetRef("Inspecteur");
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

        ctx->SetRef("Inspecteur");
        ctx->ItemInputValue("Circle Collider/Rayon", -5.0f);
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

        ctx->SetRef("Inspecteur");
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

        ctx->SetRef("Inspecteur");
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
        IM_CHECK(ctx->CaptureAddWindow("//Inspecteur"));
        IM_CHECK(ctx->CaptureScreenshot());
        IM_CHECK(std::filesystem::exists(outputFile));
    };
}

#else

void RegisterEditorTests(ImGuiTestEngine *, EditorLayer &) {}

#endif
