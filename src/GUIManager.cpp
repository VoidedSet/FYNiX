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
static char modelPathInput[256] = "", shaderName[256] = "";
static float fps_history[90] = {};
static int fps_history_index = 0;
static char inputBuffer[256] = "";

static char nodeNameInput[128] = "NewNode";
static int selectedNodeType = 1, ParentNodeId = 0, selectedLightType = 1, maxParticles = 0;
static bool drawLight = true;

static const char *nodeTypeLabels[] = {"Root", "Model", "Light", "Particles", "Empty"};
static const char *lightTypeLabels[] = {"Directional Light", "Point Light", "Spot Light", "Sun Light"};

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
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        if (ImGui::Button("Add Node"))
        {
            showAddNodeModal = true;
        }
        if (ImGui::Checkbox("Draw Light?", &drawLight))
        {
            scene->drawLights = drawLight;
        }

        if (ImGui::Button(("Save Scene")))
        {
            scene->saveScene();
        }

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 5.0f));

        DrawSceneNode(scene->root);
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        ImGui::Text("Selected Node Inspector");
        {
            Node *selectedNode = scene->find_node(static_cast<unsigned int>(selectedNodeID)); // Implement getNodeById()

            if (selectedNode)
            {
                selectedItemInspector(selectedNode); // Now passes actual node
            }
            else
            {
                ImGui::Text("No node selected.");
            }
        }
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

        if (selectedNodeType == (int)NodeType::Light)
        {
            ImGui::Text("Light Type");
            ImGui::Combo("##LightType", &selectedLightType, lightTypeLabels, IM_ARRAYSIZE(lightTypeLabels));
        }

        if (selectedNodeType == (int)NodeType::Particles)
        {

            ImGui::Text("Shader Name: ");
            ImGui::InputText("##ShaderName", shaderName, IM_ARRAYSIZE(shaderName));

            ImGui::Text("Max Particles");
            ImGui::InputInt("##MaxParticles", &maxParticles);
        }

        if (ImGui::Button("OK"))
        {
            std::string nameStr = nodeNameInput;
            std::string pathStr = modelPathInput;
            std::string shaderNamestr = shaderName;
            NodeType type = static_cast<NodeType>(selectedNodeType);

            if (type == NodeType::Model)
                scene->addToParent(nameStr, pathStr, type, ParentNodeId);
            else if (type == NodeType::Light)
                scene->addToParent(nameStr, type, ParentNodeId, (LightType)selectedLightType);
            else if (type == NodeType::Particles)
                scene->addToParent(nameStr, type, ParentNodeId, shaderNamestr, (unsigned int)maxParticles);
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
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    const std::string name = node->name + " (ID: " + std::to_string(node->ID) + ")";
    bool open = ImGui::TreeNodeEx((void *)(intptr_t)node->ID, flags, "%s", name.c_str());

    if (ImGui::IsItemClicked())
    {
        selectedNodeID = node->ID;
        std::cout << "Selected Node ID: " << selectedNodeID << std::endl;
    }

    if (open)
    {
        for (Node *child : node->children)
        {
            DrawSceneNode(child);
        }
        ImGui::TreePop();
    }
}

// =================== Selected Item Inspector =============
void GUIManager::selectedItemInspector(Node *selectedNode)
{
    if (selectedNode)
    {
        ImGui::Text("Selected Node: %s (ID: %d)", selectedNode->name.c_str(), selectedNode->ID);
        ImGui::Text("Type: %s", scene->nodeTypeToString(selectedNode->type).c_str());

        if (selectedNode->type == NodeType::Model)
        {
            Model *selectedModel = scene->getModelByID(selectedNode->ID);

            if (!selectedModel)
                return;

            ImGui::Text("Model Path: %s", selectedModel->directory.c_str());
            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            // --- Standard Transform Controls ---
            glm::vec3 position = selectedModel->getPosition();
            glm::vec3 rotation = selectedModel->getRotation();
            glm::vec3 scale = selectedModel->getScale();
            if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.01f))
                selectedModel->setPosition(position);
            if (ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.01f))
                selectedModel->setRotation(rotation);
            if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f))
                selectedModel->setScale(scale);

            // ===============================================
            // ============== ANIMATION UI HERE ==============
            // ===============================================
            if (selectedModel->hasAnimation)
            {
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                ImGui::Text("Animation Controls");

                Animator &animator = selectedModel->getAnimator();
                Animation *currentAnim = animator.getCurrentAnimation();

                if (currentAnim)
                {
                    // --- Animation Selection Dropdown ---
                    const char *current_anim_name = animator.animationNames[animator.currentAnimationIndex].c_str();
                    if (ImGui::BeginCombo("Animation", current_anim_name))
                    {
                        for (int i = 0; i < animator.animationNames.size(); ++i)
                        {
                            const bool is_selected = (animator.currentAnimationIndex == i);
                            if (ImGui::Selectable(animator.animationNames[i].c_str(), is_selected))
                            {
                                animator.setAnimation(i);
                            }
                            if (is_selected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }

                    // --- Playback Controls (Play/Pause Button) ---
                    if (animator.isPaused)
                    {
                        if (ImGui::Button("Play"))
                        {
                            animator.play();
                        }
                    }
                    else
                    {
                        if (ImGui::Button("Pause"))
                        {
                            animator.pause();
                        }
                    }
                    ImGui::SameLine();
                    ImGui::Text("%.2f / %.2f", animator.currentTime, currentAnim->duration);

                    // --- Seek Slider ---
                    if (ImGui::SliderFloat("Seek", &animator.currentTime, 0.0f, currentAnim->duration))
                    {
                        selectedModel->seek(animator.currentTime);
                    }
                }
            }
        }
        else if (selectedNode->type == NodeType::Light)
        {
            Light *selectedLight = scene->getLightByID(selectedNode->ID);
            if (!selectedLight)
                return;
            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            glm::vec3 position = selectedLight->position, color = selectedLight->color;
            if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.01f))
                selectedLight->position = position;
            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            if (ImGui::ColorPicker3("Light Color", glm::value_ptr(color)))
                selectedLight->color = color;
        }
        else if (selectedNode->type == NodeType::Particles)
        {
            ParticleEmitter *selectedEmitter = scene->getEmitterByID(selectedNode->ID);
            Light *selectedLight = scene->getLightByID(selectedNode->children[0]->ID);

            if (!selectedEmitter)
                return;
            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            glm::vec3 position = selectedEmitter->Position;
            glm::vec4 color = selectedEmitter->Color;

            if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.01f))
            {
                selectedEmitter->Position = position;
                selectedLight->position = position;
            }

            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            if (ImGui::ColorPicker4("Light Color", glm::value_ptr(color)))
            {
                selectedEmitter->Color = color;
                selectedLight->color = glm::vec3(color);
            }
        }
        ImGui::Dummy(ImVec2(0.0f, 5.0f));

        if (ImGui::Button("Delete Node"))
        {
            std::cout << "Deleting node with ID: " << selectedNode->ID << std::endl;
            scene->deleteNode(selectedNode->ID);
        }
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