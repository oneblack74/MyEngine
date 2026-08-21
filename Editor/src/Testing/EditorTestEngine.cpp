#include "Testing/EditorTestEngine.h"

#ifndef MYENGINE_EDITOR_TESTS

// Build sans Dear ImGui Test Engine : tout devient no-op.
void EditorTestEngine::Start(EditorLayer &, const EditorTestOptions &) {}
void EditorTestEngine::Stop() {}
void EditorTestEngine::OnFrameStart() {}
void EditorTestEngine::RenderUI() {}

#else

#include "EditorLayer.h"
#include "Testing/EditorTests.h"
#include <Core/Application.h>
#include <Core/Log.h>
#include <imgui.h>
#include <imgui_te_engine.h>
#include <imgui_te_context.h>
#include <imgui_te_ui.h>
#include "glad/gl.h"
#include <GLFW/glfw3.h>
#include <cstring>
#include <vector>

namespace
{
    // Appelée par le Test Engine depuis PostSwap pour récupérer une portion du rendu.
    // Les coordonnées reçues sont absolues (repère écran, origine en haut à gauche) ;
    // OpenGL lit en coordonnées locales à la fenêtre avec l'origine en bas à gauche.
    bool ScreenCapture(ImGuiID viewportId, int x, int y, int w, int h, unsigned int *pixels, void *)
    {
        ImGuiViewport *viewport = ImGui::FindViewportByID(viewportId);
        if (viewport == nullptr)
            return false;

        const int localX = x - (int)viewport->Pos.x;
        const int localY = y - (int)viewport->Pos.y;
        const int glY = (int)viewport->Size.y - (localY + h);

        // Le viewport visé n'est pas forcément la fenêtre principale (une fenêtre OS
        // détachée a son propre contexte GL), d'où la bascule le temps de la lecture.
        GLFWwindow *backupContext = glfwGetCurrentContext();
        GLFWwindow *window = (GLFWwindow *)viewport->PlatformHandle;
        if (window != nullptr && window != backupContext)
            glfwMakeContextCurrent(window);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(localX, glY, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        if (window != nullptr && window != backupContext)
            glfwMakeContextCurrent(backupContext);

        // Le Test Engine attend la première ligne en haut, OpenGL l'a rendue en bas.
        const size_t stride = (size_t)w * 4;
        std::vector<unsigned char> lineTmp(stride);
        unsigned char *lineA = (unsigned char *)pixels;
        unsigned char *lineB = (unsigned char *)pixels + stride * ((size_t)h - 1);
        while (lineA < lineB)
        {
            memcpy(lineTmp.data(), lineA, stride);
            memcpy(lineA, lineB, stride);
            memcpy(lineB, lineTmp.data(), stride);
            lineA += stride;
            lineB -= stride;
        }

        return true;
    }
}

void EditorTestEngine::Start(EditorLayer &editor, const EditorTestOptions &options)
{
    if (!options.RunTests)
        return;

    m_Options = options;
    m_Engine = ImGuiTestEngine_CreateContext();

    ImGuiTestEngineIO &testIO = ImGuiTestEngine_GetIO(m_Engine);
    // En headless personne ne regarde : on déroule à la vitesse maximale. En interactif
    // au contraire, les actions sont jouées assez lentement pour être suivies à l'œil.
    testIO.ConfigRunSpeed = options.Headless ? ImGuiTestRunSpeed_Fast : ImGuiTestRunSpeed_Normal;
    testIO.ConfigNoThrottle = options.Headless;
    testIO.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
    // En headless, la sortie du terminal est le seul retour disponible : sans ça, un
    // test qui échoue ne dit pas pourquoi.
    testIO.ConfigLogToTTY = options.Headless;
    testIO.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
    testIO.ScreenCaptureFunc = ScreenCapture;
    // Les captures demandées par un test atterrissent dans output/captures/, relatif
    // au dossier de travail.
    testIO.ConfigCaptureEnabled = true;

    RegisterEditorTests(m_Engine, editor);
    ImGuiTestEngine_Start(m_Engine, ImGui::GetCurrentContext());
}

void EditorTestEngine::Stop()
{
    if (m_Engine == nullptr)
        return;

    ImGuiTestEngine_Stop(m_Engine);
    // DestroyContext doit venir après ImGui::DestroyContext() (l'appelant s'en charge
    // dans OnDetach), sinon le Test Engine ne peut plus sauver ses propres réglages.
    ImGuiTestEngine_DestroyContext(m_Engine);
    m_Engine = nullptr;
}

void EditorTestEngine::OnFrameStart()
{
    if (m_Engine == nullptr)
        return;

    // Rien à présenter avant la toute première frame.
    if (m_FrameCount > 0)
        ImGuiTestEngine_PostSwap(m_Engine);
    ++m_FrameCount;

    if (!m_Options.Headless)
        return;

    if (!m_TestsQueued)
    {
        // On laisse l'éditeur se stabiliser avant de lancer quoi que ce soit : la mise
        // en place du dock layout est différée d'une frame et les panels n'ont leur
        // taille définitive qu'après une passe complète.
        if (m_FrameCount < m_Options.WarmupFrames)
            return;

        const char *filter = m_Options.TestFilter.empty() ? nullptr : m_Options.TestFilter.c_str();
        ImGuiTestEngine_QueueTests(m_Engine, ImGuiTestGroup_Tests, filter);
        m_TestsQueued = true;
        return;
    }

    if (!ImGuiTestEngine_IsTestQueueEmpty(m_Engine))
        return;

    ImGuiTestEngineResultSummary summary;
    ImGuiTestEngine_GetResultSummary(m_Engine, &summary);

    const int failed = summary.CountTested - summary.CountSuccess;
    if (summary.CountTested == 0)
    {
        LOG_ERROR("Tests: no test ran (filter '{0}'?)", m_Options.TestFilter);
        m_ExitCode = 1;
    }
    else if (failed > 0)
    {
        LOG_ERROR("Tests: {0} failure(s) out of {1}", failed, summary.CountTested);
        m_ExitCode = 1;
    }
    else
    {
        LOG_INFO("Tests: {0}/{1} OK", summary.CountSuccess, summary.CountTested);
        m_ExitCode = 0;
    }

    Engine::Application::Get().Close();
}

void EditorTestEngine::RenderUI()
{
    if (m_Engine != nullptr && m_Options.ShowTestUI)
        ImGuiTestEngine_ShowTestEngineWindows(m_Engine, nullptr);
}

#endif
