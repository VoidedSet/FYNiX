#include "GUI.h"
#include "glm/gtc/type_ptr.hpp"
#include "JobSystem.h"
#include "Camera.h"

extern Camera *globalCamera;

#include <windows.h>
#include <psapi.h>
#include <chrono>

ImGuiIO GUIManager::io;

// --- Encapsulation for file-scoped variables and helpers ---
// An anonymous namespace is used to keep all static variables and helper functions
// local to this file, preventing naming conflicts with other parts of the engine.
namespace
{
    // --- UI State Variables ---
    bool showAddNodeModal = false;
    char nodeNameInput[128] = "NewNode";
    char modelPathInput[256] = "";
    char shaderNameInput[256] = "";
    int parentNodeId = 0;
    int selectedNodeType = 1;       // Default to Model
    int selectedLightType = 0;      // Default to Directional
    int selectedRigidBodyShape = 0; // Default to Cube
    int maxParticles = 1000;
    float rigidBodyMass = 1.0f;
    bool drawLights = true;
    bool drawPhysics = true;
    bool simulatePhysics = false;

    // --- Benchmark State Variables ---
    bool benchmarkExecuted = false;
    double lastBenchmarkTimeLF = 0.0;
    double lastBenchmarkTimeMutex = 0.0;

    // --- Resource Overlay State ---
    constexpr int FPS_HISTORY_COUNT = 90;
    float fps_history[FPS_HISTORY_COUNT] = {};
    int fps_history_index = 0;

    // --- UI Layout Constants ---
    constexpr float SIDE_PANEL_WIDTH = 350.0f;
    constexpr float CONSOLE_HEIGHT = 320.0f;

    // --- Forward declarations for static helper functions ---
    void InspectModelNode(SceneManager *scene, Node *selectedNode);
    void InspectLightNode(SceneManager *scene, Node *selectedNode);
    void InspectParticleEmitterNode(SceneManager *scene, Node *particleNode);
    void InspectRigidBodyNode(SceneManager *scene, Node *rigidBodyNode);
    void InspectEmptyNode(SceneManager *scene, Node *selectedNode);
}

// ===================================================================================
// ========================= CONSOLE OUTPUT REDIRECTION ==============================
// ===================================================================================
class ImGuiConsoleBuffer : public std::stringbuf
{
public:
    std::vector<std::string> lines;
    std::mutex mutex;
    static constexpr size_t MAX_CONSOLE_LINES = 500;

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex);
        lines.clear();
    }

    int sync() override
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::string s = str();
        if (s.empty())
            return 0;

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
        str(""); // Clear internal buffer

        if (lines.size() > MAX_CONSOLE_LINES)
        {
            lines.erase(lines.begin(), lines.begin() + (lines.size() - MAX_CONSOLE_LINES));
        }

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
}

void RestoreOutput()
{
    if (g_originalCoutBuf)
        std::cout.rdbuf(g_originalCoutBuf);
    if (g_originalCerrBuf)
        std::cerr.rdbuf(g_originalCerrBuf);
}

static void DrawConsolePanel(int windowWidth, int windowHeight);
static void DrawResourceOverlay(int windowWidth, int windowHeight);

// ===================================================================================
// ========================= GUIManager CLASS IMPLEMENTATION =========================
// ===================================================================================

GUIManager::GUIManager(GLFWwindow *window, SceneManager &sceneRef, int w, int h)
    : scene(&sceneRef), windowWidth(w), windowHeight(h)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // VISUAL IMPROVEMENT: Apply a more professional custom dark theme
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f); // Center window titles

    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.27f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.33f, 0.36f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.30f, 0.50f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.40f, 0.65f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.50f, 0.80f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.55f, 0.90f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.35f, 0.58f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.45f, 0.75f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.50f, 0.85f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.41f, 0.42f, 0.44f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);

    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init();

    RedirectOutputToConsoleBuffer();
    std::cout << "[GUIManager] Initialized." << std::endl;
}

void GUIManager::Start()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    DrawSidePanel(windowWidth, windowHeight);
    DrawConsolePanel(windowWidth, windowHeight);
    DrawAddNodeModal();
    DrawResourceOverlay(windowWidth, windowHeight);
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
    RestoreOutput();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// ===================================================================================
// ============================ MEMBER DRAWING FUNCTIONS =============================
// ===================================================================================

void GUIManager::DrawSidePanel(int windowWidth, int windowHeight)
{
    // Keyboard shortcuts for Undo/Redo
    if (ImGui::GetIO().KeyCtrl)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Z))
            scene->undo();
        if (ImGui::IsKeyPressed(ImGuiKey_Y))
            scene->redo();
    }

    ImGui::SetNextWindowPos(ImVec2(windowWidth - SIDE_PANEL_WIDTH, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(SIDE_PANEL_WIDTH, windowHeight), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); // No border for a cleaner look

    if (ImGui::Begin("Inspector"))
    {
        if (ImGui::CollapsingHeader("Scene Options", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Add Node...", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 2, 0)))
                showAddNodeModal = true;
            ImGui::SameLine();

            // Highlight Save button when scene has unsaved changes (dirty)
            bool wasDirty = scene->isDirty;
            if (wasDirty)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
            }
            else
            {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
            }

            if (ImGui::Button("Save Scene", ImVec2(-1, 0)))
                scene->saveScene(); // Fill remaining space

            if (wasDirty)
                ImGui::PopStyleColor(3);
            else
                ImGui::PopStyleVar();

            // Undo / Redo controls in UI
            ImGui::Spacing();
            bool canUndo = !scene->undoStack.empty();
            bool canRedo = !scene->redoStack.empty();

            if (!canUndo)
                ImGui::BeginDisabled();
            if (ImGui::Button("Undo", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 2, 0)))
                scene->undo();
            if (!canUndo)
                ImGui::EndDisabled();

            ImGui::SameLine();

            if (!canRedo)
                ImGui::BeginDisabled();
            if (ImGui::Button("Redo", ImVec2(-1, 0)))
                scene->redo();
            if (!canRedo)
                ImGui::EndDisabled();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Camera View");

            const char *currentCamLabel = "Viewport Camera";
            if (scene->activeCameraID != 0)
            {
                Node *camNode = scene->find_node(scene->activeCameraID);
                if (camNode)
                    currentCamLabel = camNode->name.c_str();
            }

            if (ImGui::BeginCombo("Active Camera", currentCamLabel))
            {
                bool isSelected = (scene->activeCameraID == 0);
                if (ImGui::Selectable("Viewport Camera", isSelected))
                {
                    if (scene->activeCameraID != 0)
                    {
                        if (globalCamera)
                        {
                            scene->cameraChangesPending = false;
                            globalCamera->camPos = scene->viewportCamPos;
                            globalCamera->yaw = scene->viewportYaw;
                            globalCamera->pitch = scene->viewportPitch;
                            globalCamera->camUp = glm::vec3(0.0f, 1.0f, 0.0f);

                            glm::vec3 direction;
                            direction.x = cos(glm::radians(globalCamera->yaw)) * cos(glm::radians(globalCamera->pitch));
                            direction.y = sin(glm::radians(globalCamera->pitch));
                            direction.z = sin(glm::radians(globalCamera->yaw)) * cos(glm::radians(globalCamera->pitch));
                            globalCamera->camTarget = glm::normalize(direction);
                            *globalCamera->view = glm::lookAt(globalCamera->camPos, globalCamera->camPos + globalCamera->camTarget, globalCamera->camUp);
                        }
                        scene->activeCameraID = 0;
                    }
                }

                for (Node *n : scene->nodes)
                {
                    if (n->type == NodeType::Camera)
                    {
                        bool isSel = (scene->activeCameraID == n->ID);
                        if (ImGui::Selectable(n->name.c_str(), isSel))
                        {
                            if (scene->activeCameraID != n->ID)
                            {
                                if (globalCamera)
                                {
                                    if (scene->activeCameraID == 0)
                                    {
                                        scene->viewportCamPos = globalCamera->camPos;
                                        scene->viewportYaw = globalCamera->yaw;
                                        scene->viewportPitch = globalCamera->pitch;
                                    }

                                    scene->cameraChangesPending = false;

                                    glm::mat4 cameraWorldMat = scene->getWorldTransform(n->ID);
                                    globalCamera->camPos = glm::vec3(cameraWorldMat[3]);
                                    globalCamera->camTarget = -glm::normalize(glm::vec3(cameraWorldMat[2]));
                                    globalCamera->camUp = glm::normalize(glm::vec3(cameraWorldMat[1]));

                                    globalCamera->pitch = glm::degrees(asin(globalCamera->camTarget.y));
                                    globalCamera->yaw = glm::degrees(atan2(globalCamera->camTarget.z, globalCamera->camTarget.x));

                                    *globalCamera->view = glm::lookAt(globalCamera->camPos, globalCamera->camPos + globalCamera->camTarget, globalCamera->camUp);
                                }
                                scene->activeCameraID = n->ID;
                            }
                        }
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Checkbox("Draw Light Gizmos", &drawLights))
                scene->drawLights = drawLights;
            if (ImGui::Checkbox("Draw Physics Debug", &drawPhysics))
                scene->drawPhysics = drawPhysics;
            if (ImGui::Checkbox("Simulate Physics", &simulatePhysics))
                scene->simulate = simulatePhysics;

            bool groundEnabled = scene->infiniteFloor;
            if (ImGui::Checkbox("Infinite Floor", &groundEnabled))
            {
                scene->infiniteFloor = groundEnabled;
                if (scene->physics)
                {
                    scene->physics->setGroundPlaneEnabled(groundEnabled);
                }
            }

            static float physicsGravity = 10.0f;
            if (ImGui::DragFloat("Gravity", &physicsGravity, 0.1f, -100.0f, 100.0f, "%.2f"))
            {
                if (scene->physics)
                    scene->physics->setGravity(physicsGravity);
            }

            if (!simulatePhysics)
            {
                ImGui::SameLine();
                if (ImGui::Button("Reset Physics"))
                {
                    scene->ResetPhysics();
                }
            }
        }

        if (ImGui::CollapsingHeader("Scene Hierarchy", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginChild("Hierarchy", ImVec2(0, 250), false, ImGuiWindowFlags_HorizontalScrollbar);
            DrawSceneNode(scene->root);
            ImGui::EndChild();
        }

        if (ImGui::CollapsingHeader("Node Inspector", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginChild("InspectorChild", ImVec2(0, 0), false);

            if (scene->cameraChangesPending)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.5f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.9f, 0.6f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.7f, 0.4f, 0.0f, 1.0f));
                if (ImGui::CollapsingHeader("Unsaved Camera Changes", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::TextWrapped("You moved the active in-scene camera.");
                    ImGui::Spacing();
                    if (ImGui::Button("Save Changes", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 2, 0)))
                    {
                        Node *cameraNode = scene->find_node(scene->activeCameraID);
                        if (cameraNode)
                        {
                            scene->pushUndoState();
                            if (cameraNode->parent)
                            {
                                glm::mat4 parentWorldMat = scene->getWorldTransform(cameraNode->parent->ID);
                                cameraNode->position = glm::vec3(glm::inverse(parentWorldMat) * glm::vec4(scene->pendingCamPos, 1.0f));
                            }
                            else
                            {
                                cameraNode->position = scene->pendingCamPos;
                            }
                            cameraNode->rotation = glm::vec3(glm::radians(scene->pendingCamPitch), glm::radians(scene->pendingCamYaw + 90.0f), 0.0f);
                        }
                        scene->cameraChangesPending = false;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Discard", ImVec2(-1, 0)))
                    {
                        scene->cameraChangesPending = false;
                    }
                    ImGui::Spacing();
                }
                ImGui::PopStyleColor(3);
                ImGui::Separator();
                ImGui::Spacing();
            }

            if (selectedNodeID < 0)
            {
                ImGui::TextDisabled("No node selected.");
            }
            else
            {
                Node *selectedNode = scene->find_node(static_cast<unsigned int>(selectedNodeID));
                if (selectedNode)
                    selectedItemInspector(selectedNode);
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Selected node not found.");
                    selectedNodeID = -1; // Invalidate if node was deleted
                }
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void GUIManager::DrawAddNodeModal()
{
    if (showAddNodeModal)
        ImGui::OpenPopup("Add New Node");

    if (ImGui::BeginPopupModal("Add New Node", &showAddNodeModal, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static const char *nodeTypeLabels[] = {"Root", "Model", "Light", "Particles", "RigidBody", "Empty", "Camera"};
        static const char *lightTypeLabels[] = {"Directional", "Point", "Spot", "Sun"};

        ImGui::InputText("Node Name", nodeNameInput, IM_ARRAYSIZE(nodeNameInput));
        ImGui::Combo("Node Type", &selectedNodeType, nodeTypeLabels, IM_ARRAYSIZE(nodeTypeLabels));
        ImGui::InputInt("Parent Node ID", &parentNodeId);
        ImGui::Separator();

        static bool loadAsynchronously = true;
        static int modelSource = 0; // 0 = File, 1 = Primitive
        static int selectedPrimitive = 0;

        switch (static_cast<NodeType>(selectedNodeType))
        {
        case NodeType::Model:
        {
            ImGui::RadioButton("Load from File", &modelSource, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Create Primitive", &modelSource, 1);

            if (modelSource == 0)
            {
                ImGui::InputText("Model Path", modelPathInput, IM_ARRAYSIZE(modelPathInput));
                ImGui::Checkbox("Load Asynchronously", &loadAsynchronously);
            }
            else
            {
                static const char *primitiveLabels[] = {"Box", "Sphere", "Cylinder", "Cone"};
                ImGui::Combo("Shape", &selectedPrimitive, primitiveLabels, IM_ARRAYSIZE(primitiveLabels));

                if (selectedPrimitive == 0)
                    strcpy(modelPathInput, "primitive:box");
                else if (selectedPrimitive == 1)
                    strcpy(modelPathInput, "primitive:sphere");
                else if (selectedPrimitive == 2)
                    strcpy(modelPathInput, "primitive:cylinder");
                else if (selectedPrimitive == 3)
                    strcpy(modelPathInput, "primitive:cone");

                loadAsynchronously = false; // Primitives are instant
            }
        }
        break;
        case NodeType::Light:
            ImGui::Combo("Light Type", &selectedLightType, lightTypeLabels, IM_ARRAYSIZE(lightTypeLabels));
            break;
        case NodeType::Particles:
            ImGui::InputText("Shader Name", shaderNameInput, IM_ARRAYSIZE(shaderNameInput));
            ImGui::InputInt("Max Particles", &maxParticles);
            break;
        case NodeType::RigidBody:
            ImGui::InputFloat("Mass (0=static)", &rigidBodyMass, 0.1f, 1.0f, "%.2f");
            break;
        default:
            break;
        }

        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            scene->pushUndoState();
            std::string nameStr(nodeNameInput);
            std::string modelPathStr(modelPathInput);
            std::string shaderNameStr(shaderNameInput);

            NodeType type = static_cast<NodeType>(selectedNodeType);
            if (type == NodeType::Model)
            {
                if (loadAsynchronously)
                    scene->addToParentAsync(nameStr, modelPathStr, type, parentNodeId);
                else
                    scene->addToParent(nameStr, modelPathStr, type, parentNodeId);
            }
            else if (type == NodeType::Light)
                scene->addToParent(nameStr, type, parentNodeId, static_cast<LightType>(selectedLightType));
            else if (type == NodeType::Particles)
                scene->addToParent(nameStr, type, parentNodeId, shaderNameStr, static_cast<unsigned int>(maxParticles));
            else if (type == NodeType::RigidBody)
                scene->addToParent(nameStr, type, parentNodeId, RigidBodyShape::CUBE, rigidBodyMass);
            else
                scene->addToParent(nameStr, type, parentNodeId);

            showAddNodeModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            showAddNodeModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void GUIManager::DrawSceneNode(Node *node)
{
    if (!node)
        return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selectedNodeID == node->ID)
        flags |= ImGuiTreeNodeFlags_Selected;
    if (node->children.empty())
        flags |= ImGuiTreeNodeFlags_Leaf;

    const std::string name = node->name + " (ID: " + std::to_string(node->ID) + ")";
    bool open = ImGui::TreeNodeEx(reinterpret_cast<void *>(static_cast<intptr_t>(node->ID)), flags, "%s", name.c_str());

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

void GUIManager::selectedItemInspector(Node *selectedNode)
{
    ImGui::Text("Name: %s (ID: %d)", selectedNode->name.c_str(), selectedNode->ID);
    ImGui::Text("Type: %s", scene->nodeTypeToString(selectedNode->type).c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    switch (selectedNode->type)
    {
    case NodeType::Model:
        InspectModelNode(scene, selectedNode);
        break;
    case NodeType::Light:
        InspectLightNode(scene, selectedNode);
        break;
    case NodeType::Particles:
        InspectParticleEmitterNode(scene, selectedNode);
        break;
    case NodeType::RigidBody:
        InspectRigidBodyNode(scene, selectedNode);
        break;
    case NodeType::Empty:
    case NodeType::Root:
    case NodeType::Camera:
        InspectEmptyNode(scene, selectedNode);
        break;
    default:
        ImGui::TextDisabled("This node type has no editable properties.");
        break;
    }

    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.9f, 0.9f));
    if (ImGui::Button("Delete Node", ImVec2(-1, 0)))
    {
        std::cout << "Deleting node with ID: " << selectedNode->ID << std::endl;
        scene->pushUndoState();
        scene->deleteNode(selectedNode->ID);
        selectedNodeID = -1;
    }
    ImGui::PopStyleColor(3);
}

// ===================================================================================
// ========================== STATIC HELPER IMPLEMENTATIONS ==========================
// ===================================================================================

namespace
{
    float GetProcessRAMUsageMB()
    {
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc)))
        {
            return static_cast<float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);
        }
        return 0.0f;
    }

    float GetSystemRAMUsagePercent()
    {
        MEMORYSTATUSEX memInfo = {};
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memInfo))
        {
            return static_cast<float>(memInfo.ullTotalPhys - memInfo.ullAvailPhys) / static_cast<float>(memInfo.ullTotalPhys);
        }
        return 0.0f;
    }

    void InspectModelNode(SceneManager *scene, Node *selectedNode)
    {
        Model *model = scene->getModelByID(selectedNode->ID);
        if (!model)
            return;

        ImGui::Text("Path: %s", model->directory.c_str());

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Transform");
        ImGui::Spacing();

        glm::vec3 position = model->getPosition();
        glm::vec3 rotation = model->getRotation();
        glm::vec3 scale = model->getScale();

        ImGui::PushItemWidth(-FLT_MIN * 0.5f); // Make drag floats take up half the width
        if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.01f))
        {
            model->setPosition(position);
            selectedNode->position = position;
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        if (ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.1f))
        {
            model->setRotation(rotation);
            selectedNode->rotation = rotation;
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f))
        {
            model->setScale(scale);
            selectedNode->scale = scale;
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Material Parameters");
        ImGui::Spacing();

        ImGui::PushItemWidth(-FLT_MIN * 0.5f);
        if (ImGui::ColorEdit3("Ambient", glm::value_ptr(model->material.ambient)))
        {
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        if (ImGui::ColorEdit3("Diffuse", glm::value_ptr(model->material.diffuse)))
        {
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        if (ImGui::ColorEdit3("Specular", glm::value_ptr(model->material.specular)))
        {
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        if (ImGui::DragFloat("Shininess", &model->material.shininess, 0.5f, 1.0f, 256.0f, "%.1f"))
        {
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();
        ImGui::PopItemWidth();

        if (model->hasAnimation)
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Animation");
            ImGui::Spacing();

            Animator &animator = model->getAnimator();
            Animation *currentAnim = animator.getCurrentAnimation();
            if (currentAnim)
            {
                const char *current_anim_name = animator.animationNames[animator.currentAnimationIndex].c_str();
                if (ImGui::BeginCombo("Animation", current_anim_name))
                {
                    for (size_t i = 0; i < animator.animationNames.size(); ++i)
                    {
                        const bool is_selected = (animator.currentAnimationIndex == i);
                        if (ImGui::Selectable(animator.animationNames[i].c_str(), is_selected))
                            animator.setAnimation(i);
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                if (animator.isPaused ? ImGui::Button("Play") : ImGui::Button("Pause"))
                    animator.isPaused ? animator.play() : animator.pause();
                ImGui::SameLine();
                ImGui::Text("%.2f / %.2f s", animator.currentTime, currentAnim->duration);
                if (ImGui::SliderFloat("Seek", &animator.currentTime, 0.0f, currentAnim->duration))
                    model->seek(animator.currentTime);
            }
        }
    }

    void InspectLightNode(SceneManager *scene, Node *selectedNode)
    {
        Light *light = scene->getLightByID(selectedNode->ID);
        if (!light)
            return;
        if (ImGui::DragFloat3("Position", glm::value_ptr(light->position), 0.1f))
        {
            selectedNode->position = light->position;
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        glm::vec3 eulerRotation = glm::degrees(selectedNode->rotation);
        if (ImGui::DragFloat3("Rotation", glm::value_ptr(eulerRotation), 0.1f))
        {
            selectedNode->rotation = glm::radians(eulerRotation);
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        if (ImGui::DragFloat("Intensity", &light->intensity, 0.05f, 0.0f, 100.0f))
        {
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        if (ImGui::ColorEdit3("Color", glm::value_ptr(light->color)))
        {
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();
    }

    void InspectParticleEmitterNode(SceneManager *scene, Node *particleNode)
    {
        ParticleEmitter *emitter = scene->getEmitterByID(particleNode->ID);
        if (!emitter || particleNode->children.empty())
            return;
        Light *light = scene->getLightByID(particleNode->children[0]->ID);
        if (!light)
            return;

        if (ImGui::DragFloat3("Position", glm::value_ptr(emitter->Position), 0.1f))
        {
            light->position = emitter->Position;
            particleNode->position = emitter->Position;
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        if (ImGui::ColorEdit4("Color", glm::value_ptr(emitter->Color)))
        {
            light->color = glm::vec3(emitter->Color);
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();
    }

    void InspectRigidBodyNode(SceneManager *scene, Node *rigidBodyNode)
    {
        btRigidBody *body = scene->getRigidBodyByID(rigidBodyNode->ID);
        if (!body)
            return;

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Transform");
        ImGui::Spacing();

        btTransform trans;
        body->getMotionState()->getWorldTransform(trans);
        glm::vec3 position(trans.getOrigin().x(), trans.getOrigin().y(), trans.getOrigin().z());
        glm::quat rotationQuat(trans.getRotation().w(), trans.getRotation().x(), trans.getRotation().y(), trans.getRotation().z());
        glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(rotationQuat));
        btVector3 scaleVec = body->getCollisionShape()->getLocalScaling();
        glm::vec3 scale(scaleVec.x(), scaleVec.y(), scaleVec.z());

        bool transformChanged = false;
        if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.01f))
        {
            transformChanged = true;
            rigidBodyNode->position = position;
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        if (ImGui::DragFloat3("Rotation", glm::value_ptr(eulerRotation), 1.0f))
        {
            transformChanged = true;
            rigidBodyNode->rotation = glm::radians(eulerRotation);
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        if (transformChanged)
        {
            trans.setOrigin(btVector3(position.x, position.y, position.z));
            glm::quat newRotQuat = glm::quat(glm::radians(eulerRotation));
            trans.setRotation(btQuaternion(newRotQuat.x, newRotQuat.y, newRotQuat.z, newRotQuat.w));
            body->setWorldTransform(trans);
            body->getMotionState()->setWorldTransform(trans);
            body->activate(true);
        }

        if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f, 0.1f))
        {
            body->getCollisionShape()->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
            scene->physics->getDynamicsWorld()->updateSingleAabb(body);

            // Recompute local inertia and update mass props
            float mass = (body->getInvMass() == 0.0f) ? 0.0f : 1.0f / body->getInvMass();
            btVector3 localInertia(0, 0, 0);
            if (mass > 0.0f)
                body->getCollisionShape()->calculateLocalInertia(mass, localInertia);
            body->setMassProps(mass, localInertia);
            body->updateInertiaTensor();

            rigidBodyNode->scale = scale;
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Physics Properties");
        ImGui::Spacing();

        float mass = (body->getInvMass() == 0.0f) ? 0.0f : 1.0f / body->getInvMass();
        if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.0f, 10000.0f))
        {
            btVector3 localInertia(0, 0, 0);
            if (mass > 0.0f)
                body->getCollisionShape()->calculateLocalInertia(mass, localInertia);
            body->setMassProps(mass, localInertia);
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        float friction = body->getFriction();
        if (ImGui::DragFloat("Friction", &friction, 0.05f, 0.0f, 5.0f))
            body->setFriction(friction);
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        float restitution = body->getRestitution();
        if (ImGui::DragFloat("Restitution", &restitution, 0.05f, 0.0f, 1.0f))
            body->setRestitution(restitution);
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();
    }

    void InspectEmptyNode(SceneManager *scene, Node *selectedNode)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Transform");
        ImGui::Spacing();

        ImGui::PushItemWidth(-FLT_MIN * 0.5f);
        if (ImGui::DragFloat3("Position", glm::value_ptr(selectedNode->position), 0.01f))
        {
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        glm::vec3 eulerRotation = glm::degrees(selectedNode->rotation);
        if (ImGui::DragFloat3("Rotation", glm::value_ptr(eulerRotation), 0.1f))
        {
            selectedNode->rotation = glm::radians(eulerRotation);
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        if (ImGui::DragFloat3("Scale", glm::value_ptr(selectedNode->scale), 0.01f))
        {
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
            scene->pushUndoState();

        ImGui::PopItemWidth();
    }

    double ExecuteMicroBenchmark(bool useLockFree)
    {
        // Save original scheduler state
        bool wasLockFree = JobSystem::Get().IsUsingLockFree();

        // Toggle to the target queue type for the benchmark
        JobSystem::Get().ToggleQueueType(useLockFree);

        constexpr int ITEM_COUNT = 100000;
        std::atomic<int> completionCounter{ITEM_COUNT};

        auto startTime = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < ITEM_COUNT; ++i)
        {
            Job testJob;
            testJob.completionCounter = &completionCounter;
            testJob.work = []()
            {
                // Simulate light microsecond math workloads typical to engine transformations or order booking
                volatile int counter = 0;
                for (int j = 0; j < 50; ++j)
                    counter++;
            };
            JobSystem::Get().Submit(testJob);
        }

        // Main thread helps execution instead of freezing
        JobSystem::Get().Wait(&completionCounter);

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;

        std::cout << "[BENCHMARK] (" << (useLockFree ? "Lock-Free" : "Mutex")
                  << ") Processed " << ITEM_COUNT << " jobs in " << elapsed.count() << " ms\n";

        // Restore original scheduler state
        JobSystem::Get().ToggleQueueType(wasLockFree);

        return elapsed.count();
    }
} // end anonymous namespace

// ===================================================================================
// ========================= NON-MEMBER DRAWING FUNCTIONS ============================
// ===================================================================================

static void DrawConsolePanel(int windowWidth, int windowHeight)
{
    const float consoleHeight = 350.0f;
    const float resourceWidth = 320.0f;

    ImGui::SetNextWindowPos(ImVec2(0, windowHeight - consoleHeight - 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(windowWidth - SIDE_PANEL_WIDTH - resourceWidth, consoleHeight), ImGuiCond_Always);

    static bool autoScroll = true; // Tracks if we should auto-scroll

    if (ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
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

static void DrawResourceOverlay(int windowWidth, int windowHeight)
{
    const float consoleHeight = 350.0f;
    const float resourceWidth = 320.0f;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    ImGui::SetNextWindowPos(ImVec2(windowWidth - SIDE_PANEL_WIDTH - resourceWidth, windowHeight - consoleHeight - 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(resourceWidth, consoleHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.85f);

    if (ImGui::Begin("Resource Monitor", nullptr, flags))
    {
        ImGuiIO &io = ImGui::GetIO();
        float current_fps = io.DeltaTime > 0.0f ? (1.0f / io.DeltaTime) : 0.0f;
        fps_history[fps_history_index] = current_fps;
        fps_history_index = (fps_history_index + 1) % FPS_HISTORY_COUNT;
        char fps_overlay[32];
        sprintf(fps_overlay, "FPS: %.1f", current_fps);
        ImGui::Text("%s", fps_overlay);
        ImGui::PlotLines("##FPS", fps_history, FPS_HISTORY_COUNT, fps_history_index, nullptr, 0.0f, 200.0f, ImVec2(180, 40));
        ImGui::Separator();
        float ramUsageMB = GetProcessRAMUsageMB();
        float systemRamPercent = GetSystemRAMUsagePercent();
        ImGui::Text("Process RAM: %.2f MB", ramUsageMB);
        char ramLabel[32];
        snprintf(ramLabel, sizeof(ramLabel), "Sys RAM: %.1f%%", systemRamPercent * 100.0f);
        ImGui::ProgressBar(systemRamPercent, ImVec2(180, 0), ramLabel);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Concurrency Profiler");

        // Core Engine Scheduler Metrics
        size_t queueDepth = JobSystem::Get().GetCurrentQueueDepth();
        size_t totalJobs = JobSystem::Get().GetTotalJobsExecuted();
        size_t peakDepth = JobSystem::Get().GetPeakQueueDepth();
        bool isLF = JobSystem::Get().IsUsingLockFree();

        ImGui::Text("Active Queue Depth: %llu", queueDepth);
        ImGui::Text("Peak Queue Depth: %llu", peakDepth);
        ImGui::Text("Total Jobs Executed: %llu", totalJobs);
        ImGui::Text("Active Pipeline: %s", isLF ? "Lock-Free MPMC Ring Buffer" : "Standard Mutex Queue");

        // Reset peak for the next frame's tracking
        JobSystem::Get().ResetPeakQueueDepth();

        if (ImGui::Button("Toggle Scheduler Mode"))
        {
            JobSystem::Get().ToggleQueueType(!isLF);
        }

        ImGui::Spacing();
        if (ImGui::Button("Run Micro-Benchmark (100k Jobs)"))
        {
            // Run both sequentially under identical constraints to generate real-time hardware metrics
            lastBenchmarkTimeLF = ExecuteMicroBenchmark(true);     // Test Dmitri Vyukov's Lock-Free Ring Buffer
            lastBenchmarkTimeMutex = ExecuteMicroBenchmark(false); // Test Mutex/Deque setup
            benchmarkExecuted = true;
        }

        if (benchmarkExecuted)
        {
            ImGui::Text("Lock-Free MPMC: %.3f ms", lastBenchmarkTimeLF);
            ImGui::Text("Mutex Guarded:  %.3f ms", lastBenchmarkTimeMutex);

            float efficiencyGain = 0.0f;
            if (lastBenchmarkTimeLF > 0)
            {
                efficiencyGain = ((lastBenchmarkTimeMutex - lastBenchmarkTimeLF) / lastBenchmarkTimeMutex) * 100.0f;
            }

            if (efficiencyGain > 0)
            {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Lock-Free is %.1f%% faster", efficiencyGain);
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Lock-Free is %.1f%% slower (Low core contention)", -efficiencyGain);
            }
        }
    }
    ImGui::End();
}
