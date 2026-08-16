#include "Panels/ConsolePanel.h"
#include <Core/Log.h>
#include <imgui.h>

void ConsolePanel::OnImGuiRender()
{
    ImGui::Begin("Console");

    for (const auto &line : Engine::Log::GetConsoleMessages())
        ImGui::TextUnformatted(line.c_str());

    // Auto-scroll : ne suit le bas que si l'utilisateur n'a pas remonté manuellement
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::End();
}
