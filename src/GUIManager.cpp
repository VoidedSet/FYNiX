#include "GUI.h"

#include <cstdio>
#include <windows.h>
#include <psapi.h>

ImGuiIO GUIManager::io;

class ImGuiConsoleBuffer : public std::stringbuf
{
public:
    std::vector<std::string> lines;
    std::mutex mutex;

    int sync() override
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::string s = str();
        size_t start = 0;
        size_t end;
        while ((end = s.find('\n', start)) != std::string::npos)
        {
            lines.push_back(s.substr(start, end - start));
            start = end + 1;
        }
        if (start < s.size())
        {
            lines.push_back(s.substr(start));
        }
        str(""); // Clear buffer
        return 0;
    }
};

static ImGuiConsoleBuffer g_consoleBuffer;
static std::streambuf *g_originalCoutBuf = nullptr;
static std::streambuf *g_originalCerrBuf = nullptr;

void RedirectOutputToConsoleBuffer()
{
    g_originalCoutBuf = std::cout.rdbuf(&g_consoleBuffer);
    g_originalCerrBuf = std::cerr.rdbuf(&g_consoleBuffer);
    // You can also redirect printf by using freopen but less clean
}

void RestoreOutput()
{
    if (g_originalCoutBuf)
        std::cout.rdbuf(g_originalCoutBuf);
    if (g_originalCerrBuf)
        std::cerr.rdbuf(g_originalCerrBuf);
}

float GetProcessRAMUsageMB();
float GetRAMUsagePercent();

void DrawConsolePanel(int windowWidth, int windowHeight);
void drawResourceOverlay(bool *p_open = nullptr); // Optional pointer

static bool showAddNodeModal = false;
static char modelPathInput[256] = "";
static float fps_history[90] = {};
static int fps_history_index = 0;
static char inputBuffer[256] = "";

static char nodeNameInput[128] = "NewNode";
static int selectedNodeType = 1, ParentNodeId = 0;

static const char *nodeTypeLabels[] = {"Root", "Model", "Shapes", "Light", "Empty"};

GUIManager::GUIManager(GLFWwindow *window, SceneManager &scene, int windowWidth, int windowHeight)
    : scene(&scene), windowWidth(windowWidth), windowHeight(windowHeight)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init();

    RedirectOutputToConsoleBuffer();

    std::cout << "[GUIManager] Initialized with window size: " << windowWidth << "x" << windowHeight << std::endl;
}

void GUIManager::Start()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    DrawSidePanel(windowWidth, windowHeight);
    DrawConsolePanel(windowWidth, windowHeight);
    DrawAddNodeModal();
    drawResourceOverlay();
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
    RestoreOutput();
}

// ================= Side Panel =================
void GUIManager::DrawSidePanel(int windowWidth, int windowHeight)
{
    const float panelWidth = 300.0f;

    ImGui::SetNextWindowPos(ImVec2(windowWidth - panelWidth, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, windowHeight - 82), ImGuiCond_Always);

    if (ImGui::Begin("Inspector"))
    {
        ImGui::Text("Scene Options");
        if (ImGui::Button("Add Node"))
        {
            showAddNodeModal = true;
        }

        ImGui::Separator();
        DrawSceneNode(scene->root);
    }
    ImGui::End();
}

// ================= Add Node Modal =================
void GUIManager::DrawAddNodeModal()
{
    if (showAddNodeModal)
        ImGui::OpenPopup("Add New Node");

    if (ImGui::BeginPopupModal("Add New Node", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Node Name:");
        ImGui::InputText("##NodeName", nodeNameInput, IM_ARRAYSIZE(nodeNameInput));

        ImGui::Text("Node Type:");
        ImGui::Combo("##NodeType", &selectedNodeType, nodeTypeLabels, IM_ARRAYSIZE(nodeTypeLabels));

        ImGui::Text("Parent Node ID");
        ImGui::InputInt("##ParentNodeID", &ParentNodeId);

        if (selectedNodeType == static_cast<int>(NodeType::Model))
        {
            ImGui::Text("Model Path:");
            ImGui::InputText("##ModelPath", modelPathInput, IM_ARRAYSIZE(modelPathInput));
        }

        if (ImGui::Button("OK"))
        {
            std::string nameStr = nodeNameInput;
            std::string pathStr = modelPathInput;
            NodeType type = static_cast<NodeType>(selectedNodeType);

            if (type == NodeType::Model)
                scene->addToParent(nameStr, pathStr, type, ParentNodeId);
            else
                scene->addToParent(nameStr, type, ParentNodeId);

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

// ================= Scene Hierarchy =================
void GUIManager::DrawSceneNode(Node *node)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

    bool open = ImGui::TreeNodeEx((void *)(intptr_t)node->ID, flags, "%s", node->name.c_str());
    if (open)
    {
        for (Node *child : node->children)
        {
            DrawSceneNode(child);
        }
        ImGui::TreePop();
    }
}

// =================== Console Panel ===================
void DrawConsolePanel(int windowWidth, int windowHeight)
{
    const float consoleHeight = 180.0f;

    ImGui::SetNextWindowPos(ImVec2(0, windowHeight - consoleHeight - 70), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(windowWidth - 300.f, consoleHeight), ImGuiCond_Always);

    static bool autoScroll = true; // Tracks if we should auto-scroll

    if (ImGui::Begin("Console"))
    {
        ImGui::BeginChild("LogRegion", ImVec2(0, -30), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        std::lock_guard<std::mutex> lock(g_consoleBuffer.mutex);
        for (const auto &line : g_consoleBuffer.lines)
            ImGui::TextWrapped("%s", line.c_str());

        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f); // Only scroll if at the bottom

        ImGui::EndChild();
    }
    ImGui::End();
}

// =================== RAM Usage via WinAPI ===================
float GetRAMUsagePercent()
{
    MEMORYSTATUSEX memInfo = {};
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (!GlobalMemoryStatusEx(&memInfo))
        return 0.0f;

    DWORDLONG total = memInfo.ullTotalPhys;
    DWORDLONG used = memInfo.ullTotalPhys - memInfo.ullAvailPhys;

    return (float)used / (float)total;
}

// =================== Resource Overlay ===================
void drawResourceOverlay(bool *p_open)
{
    ImGuiIO &io = ImGui::GetIO();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    const float PAD = 10.0f;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImVec2 pos = ImVec2(viewport->WorkPos.x + PAD, viewport->WorkPos.y + PAD);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);

    if (!ImGui::Begin("Resource Overlay", p_open, flags))
    {
        ImGui::End();
        return;
    }

    // --- FPS Line Plot ---
    float current_fps = 1.0f / io.DeltaTime;
    fps_history[fps_history_index] = current_fps;
    fps_history_index = (fps_history_index + 1) % IM_ARRAYSIZE(fps_history);

    ImGui::Text("FPS: %.1f", current_fps);
    ImGui::PlotLines("##FPS", fps_history, IM_ARRAYSIZE(fps_history), fps_history_index, NULL, 0.0f, 144.0f, ImVec2(180, 40));

    // --- RAM Usage ---
    float ramUsage = GetRAMUsagePercent();
    char ramLabel[32];
    snprintf(ramLabel, sizeof(ramLabel), "RAM: %.1f%%", ramUsage * 100.0f);
    ImGui::ProgressBar(ramUsage, ImVec2(180, 0), ramLabel);
    float ramUsageMB = GetProcessRAMUsageMB();
    ImGui::Text("Process RAM Usage: %.2f MB", ramUsageMB);
    ImGui::End();
}

float GetProcessRAMUsageMB()
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc)))
    {
        SIZE_T memUsedBytes = pmc.WorkingSetSize;
        return static_cast<float>(memUsedBytes) / (1024.0f * 1024.0f); // Return in MB
    }
    return 0.0f;
}