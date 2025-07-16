#include "GUI.h"

void DrawSidePanel(int windowWidth, int windowHeight);
void DrawAddNodeModal();
void DrawConsolePanel(int windowWidth, int windowHeight);
void DrawFakeSceneHierarchy();

static bool showAddNodeModal = false;
static char modelPathInput[256] = ""; // For path input field

GUIManager::GUIManager(GLFWwindow *window, int windowWidth, int windowHeight)
    : windowWidth(windowWidth), windowHeight(windowHeight)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    GUIManager::io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init();
}

void GUIManager::Start()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    DrawSidePanel(windowWidth, windowHeight);
    DrawConsolePanel(windowWidth, windowHeight);
    DrawAddNodeModal();
}

void GUIManager::Render()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void GUIManager::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// =================== Side Panel ===================
void DrawSidePanel(int windowWidth, int windowHeight)
{
    const float panelWidth = 300.0f;

    ImGui::SetNextWindowPos(ImVec2(windowWidth - panelWidth, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, windowHeight - 200), ImGuiCond_Always); // Leave space for console

    if (ImGui::Begin("Inspector"))
    {
        ImGui::Text("Scene Options");
        if (ImGui::Button("Add Node"))
        {
            showAddNodeModal = true;
        }

        ImGui::Separator();
        DrawFakeSceneHierarchy();
    }
    ImGui::End();
}

// =================== Add Node Modal ===================
void DrawAddNodeModal()
{
    if (showAddNodeModal)
        ImGui::OpenPopup("Add New Node");

    if (ImGui::BeginPopupModal("Add New Node", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Enter path to model:");
        ImGui::InputText("##ModelPath", modelPathInput, IM_ARRAYSIZE(modelPathInput));

        if (ImGui::Button("OK"))
        {
            // Call your model loading function here later
            // loadModel(modelPathInput);
            showAddNodeModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            showAddNodeModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// =================== Fake Scene Hierarchy ===================
void DrawFakeSceneHierarchy()
{
    if (ImGui::TreeNode("Scene Root"))
    {
        ImGui::BulletText("Camera");
        if (ImGui::TreeNode("Mesh_01"))
        {
            ImGui::BulletText("Material: Metal");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Mesh_02"))
        {
            ImGui::BulletText("Material: Plastic");
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }
}

// =================== Console Panel ===================
void DrawConsolePanel(int windowWidth, int windowHeight)
{
    const float consoleHeight = 200.0f;

    ImGui::SetNextWindowPos(ImVec2(0, windowHeight - consoleHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, consoleHeight), ImGuiCond_Always);

    if (ImGui::Begin("Console"))
    {
        ImGui::TextWrapped("Console initialized...");
        // Later: link this to actual error/warning log
    }
    ImGui::End();
}
