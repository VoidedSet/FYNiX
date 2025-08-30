#include "GUI.h"
#include "glm/gtc/type_ptr.hpp"

#include <windows.h>
#include <psapi.h>

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
    int selectedNodeType = 1;  // Default to Model
    int selectedLightType = 0; // Default to Directional
    int maxParticles = 1000;
    float rigidBodyMass = 1.0f;
    bool drawLights = true;
    bool drawPhysics = true;
    bool simulatePhysics = false;

    // --- Console State ---
    bool consoleAutoScroll = true;

    // --- Resource Overlay State ---
    constexpr int FPS_HISTORY_COUNT = 90;
    float fps_history[FPS_HISTORY_COUNT] = {};
    int fps_history_index = 0;

    // --- UI Layout Constants ---
    constexpr float SIDE_PANEL_WIDTH = 350.0f;
    constexpr float CONSOLE_HEIGHT = 220.0f;

    // --- Forward declarations for static helper functions ---
    void InspectModelNode(SceneManager *scene, Node *selectedNode);
    void InspectLightNode(SceneManager *scene, Node *selectedNode);
    void InspectParticleEmitterNode(SceneManager *scene, Node *particleNode);
    void InspectRigidBodyNode(SceneManager *scene, Node *rigidBodyNode);
}

// ===================================================================================
// ========================= CONSOLE OUTPUT REDIRECTION ==============================
// ===================================================================================
class ImGuiConsoleBuffer : public std::stringbuf
{
public:
    std::vector<std::string> lines;
    std::mutex mutex;

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
static void DrawResourceOverlay();

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
    DrawResourceOverlay();
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
            if (ImGui::Button("Save Scene", ImVec2(-1, 0)))
                scene->saveScene(); // Fill remaining space

            ImGui::Spacing();
            if (ImGui::Checkbox("Draw Light Gizmos", &drawLights))
                scene->drawLights = drawLights;
            if (ImGui::Checkbox("Draw Physics Debug", &drawPhysics))
                scene->drawPhysics = drawPhysics;
            if (ImGui::Checkbox("Simulate Physics", &simulatePhysics))
                scene->simulate = simulatePhysics;
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
        static const char *nodeTypeLabels[] = {"Root", "Model", "Light", "Particles", "RigidBody", "Empty"};
        static const char *lightTypeLabels[] = {"Directional", "Point", "Spot", "Sun"};

        ImGui::InputText("Node Name", nodeNameInput, IM_ARRAYSIZE(nodeNameInput));
        ImGui::Combo("Node Type", &selectedNodeType, nodeTypeLabels, IM_ARRAYSIZE(nodeTypeLabels));
        ImGui::InputInt("Parent Node ID", &parentNodeId);
        ImGui::Separator();

        switch (static_cast<NodeType>(selectedNodeType))
        {
        case NodeType::Model:
            ImGui::InputText("Model Path", modelPathInput, IM_ARRAYSIZE(modelPathInput));
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
            std::string nameStr(nodeNameInput);
            std::string modelPathStr(modelPathInput);
            std::string shaderNameStr(shaderNameInput);

            NodeType type = static_cast<NodeType>(selectedNodeType);
            if (type == NodeType::Model)
                scene->addToParent(nameStr, modelPathStr, type, parentNodeId);
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
            model->setPosition(position);
        if (ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.1f))
            model->setRotation(rotation);
        if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f))
            model->setScale(scale);
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
        ImGui::DragFloat3("Position", glm::value_ptr(light->position), 0.1f);
        ImGui::ColorEdit3("Color", glm::value_ptr(light->color));
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
        }
        if (ImGui::ColorEdit4("Color", glm::value_ptr(emitter->Color)))
        {
            light->color = glm::vec3(emitter->Color);
        }
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
            transformChanged = true;
        if (ImGui::DragFloat3("Rotation", glm::value_ptr(eulerRotation), 1.0f))
            transformChanged = true;

        if (transformChanged)
        {
            trans.setOrigin(btVector3(position.x, position.y, position.z));
            glm::quat newRotQuat = glm::quat(glm::radians(eulerRotation));
            trans.setRotation(btQuaternion(newRotQuat.x, newRotQuat.y, newRotQuat.z, newRotQuat.w));
            body->setWorldTransform(trans);
            body->getMotionState()->setWorldTransform(trans);
            body->activate(true);
        }

        if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f))
        {
            body->getCollisionShape()->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
            scene->physics->getDynamicsWorld()->updateSingleAabb(body);
        }

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
        float friction = body->getFriction();
        if (ImGui::DragFloat("Friction", &friction, 0.05f, 0.0f, 5.0f))
            body->setFriction(friction);
        float restitution = body->getRestitution();
        if (ImGui::DragFloat("Restitution", &restitution, 0.05f, 0.0f, 1.0f))
            body->setRestitution(restitution);
    }
} // end anonymous namespace

// ===================================================================================
// ========================= NON-MEMBER DRAWING FUNCTIONS ============================
// ===================================================================================

static void DrawConsolePanel(int windowWidth, int windowHeight)
{
    ImGui::SetNextWindowPos(ImVec2(0, windowHeight - CONSOLE_HEIGHT), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(windowWidth - SIDE_PANEL_WIDTH, CONSOLE_HEIGHT), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("Console"))
    {
        if (ImGui::Button("Clear"))
            g_consoleBuffer.clear();
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &consoleAutoScroll);
        ImGui::Separator();

        ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        std::lock_guard<std::mutex> lock(g_consoleBuffer.mutex);
        for (const auto &line : g_consoleBuffer.lines)
        {
            ImGui::TextUnformatted(line.c_str());
        }
        if (consoleAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

static void DrawResourceOverlay()
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    const float PADDING = 10.0f;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImVec2 pos(viewport->WorkPos.x + PADDING, viewport->WorkPos.y + PADDING);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.5f);

    if (ImGui::Begin("Resource Overlay", nullptr, flags))
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
    }
    ImGui::End();
}
