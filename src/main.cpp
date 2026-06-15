// default cpp includes
#include <iostream>
#include <windows.h>
#include <dirent.h>

// opengl and related includes
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// project includes
#include "Window.h"
#include "Shader.h"
#include "Camera.h"

#include "SceneManager.h"

#include "ShaderManager.h"

#include "GUI.h"

#include "ParticleSystem.h"

#include "PhysicsEngine.h"

#include "JobSystem.h"

using namespace std;

extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

Camera *globalCamera = nullptr;

glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    if (ImGui::GetIO().WantCaptureMouse)
        return;
    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureMouse && globalCamera)
        globalCamera->mouseInput(xpos, ypos);
}
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureMouse)
    {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            globalCamera->cameraLock = true;
            globalCamera->firstMove = true;
        }
    }
}
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}
void char_callback(GLFWwindow *window, unsigned int c)
{
    ImGui_ImplGlfw_CharCallback(window, c);
}
void inputHandler(GLFWwindow *window, float deltaTime, Camera &camera)
{
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCharCallback(window, char_callback);

    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            camera.cameraLock = false;
        }
        camera.processInput(window, deltaTime);
    }
}
std::string FindFynxProjectFile(const std::string &folderPath)
{
    DIR *dir;
    struct dirent *ent;

    dir = opendir(folderPath.c_str());
    if (dir == nullptr)
    {
        std::cerr << "Could not open directory: " << folderPath << std::endl;
        return "";
    }

    while ((ent = readdir(dir)) != nullptr)
    {
        std::string filename = ent->d_name;
        if (filename.size() > 5 && filename.substr(filename.size() - 5) == ".fynx")
        {
            closedir(dir);
            return folderPath + "/" + filename;
        }
    }

    closedir(dir);
    return "";
}

void ShowSplashScreen()
{
    // Load splash image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true); 
    unsigned char* pixels = stbi_load("banner.png", &width, &height, &channels, 4);
    if (!pixels)
    {
        std::cerr << "[Splash] Failed to load banner.png" << std::endl;
        return;
    }

    // Scale banner size down by 65%
    int splashWidth = static_cast<int>(width * 0.65f);
    int splashHeight = static_cast<int>(height * 0.65f);

    // Configure splash window hints (borderless, centered, on top)
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int xPos = (mode->width - splashWidth) / 2;
    int yPos = (mode->height - splashHeight) / 2;

    GLFWwindow* splashWindow = glfwCreateWindow(splashWidth, splashHeight, "FYNiX Splash", NULL, NULL);
    if (!splashWindow)
    {
        std::cerr << "[Splash] Failed to create splash window" << std::endl;
        stbi_image_free(pixels);
        return;
    }

    glfwSetWindowPos(splashWindow, xPos, yPos);
    glfwMakeContextCurrent(splashWindow);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "[Splash] Failed to initialize GLAD for splash" << std::endl;
        glfwDestroyWindow(splashWindow);
        stbi_image_free(pixels);
        return;
    }

    // Set viewport to the scaled window dimensions
    glViewport(0, 0, splashWidth, splashHeight);

    // Set up splash shaders
    const char* vsSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoords;
        out vec2 TexCoords;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoords = aTexCoords;
        }
    )";
    const char* fsSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoords;
        uniform sampler2D splashTex;
        void main() {
            FragColor = texture(splashTex, TexCoords);
        }
    )";

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSource, NULL);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSource, NULL);
    glCompileShader(fs);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    // Quad geometry (NDC coordinates)
    float vertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // Create and bind texture
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(pixels);

    // Render loop for splash screen (approx 2.5 seconds)
    double startTime = glfwGetTime();
    while (glfwGetTime() - startTime < 2.5)
    {
        if (glfwWindowShouldClose(splashWindow))
            break;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(splashWindow);
        glfwPollEvents();
    }

    // Cleanup splash resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
    glfwDestroyWindow(splashWindow);

    // Reset window hints for main window creation
    glfwDefaultWindowHints();
}

int main(int argc, char *argv[])
{
    // Hide terminal console window by default unless --show argument is passed
    bool showConsole = false;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--show")
        {
            showConsole = true;
        }
    }
    if (!showConsole)
    {
        FreeConsole();
    }

    std::string path = FindFynxProjectFile("Projects/Load_Project");
    if (path.empty())
    {
        std::cerr << "[FYNiX] No .fynx file found." << std::endl;
        getchar();
        return -1;
    }

    if (!glfwInit())
    {
        cout << "Failed to initialize GLFW" << endl;
        return -1;
    }

    ShowSplashScreen();

    JobSystem::Get().Initialize();

    std::string projectName = "[" + path.substr(path.find_last_of('/') + 1) + "] FYNiX - Framework for Yet-to-be Named eXperiences";
    Window windowManager((char *)projectName.c_str());
    GLFWwindow *window = windowManager.getWindowObject();

    SceneManager scene(path);

    GUIManager gui(windowManager.getWindowObject(), scene, windowManager.mode->width, windowManager.mode->height);

    glm::mat4 view = glm::mat4(1.f);
    Camera cam(&view);
    globalCamera = &cam;

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    ShaderManager sm;
    scene.sm = &sm;

    sm.addShader("light", "shaders/light/vertex.glsl", "shaders/light/fragment.glsl"),
        sm.addShader("default", "shaders/model/vertex.glsl", "shaders/model/fragment.glsl"),
        sm.addShader("particle", "shaders/particles/particles.vert", "shaders/particles/particles.frag");

    Shader &lightShader = sm.findShader("light");
    Shader &particleShader = sm.findShader("particle");
    Shader &defaultShader = sm.findShader("default");

    sm.listShaders();

    defaultShader.use();

    glm::mat4 model = glm::mat4(0.f);
    glm::mat4 projection = glm::mat4(0.f);
    projection = glm::perspective(glm::radians(45.f), (float)windowManager.mode->width / (float)windowManager.mode->height, 0.1f, 100.f);

    defaultShader.setUniforms("model", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(model));
    defaultShader.setUniforms("view", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(view));
    defaultShader.setUniforms("projection", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(projection));

    scene.LoadScene(path);

    lightShader.use();

    lightShader.setUniforms("view", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(view));
    lightShader.setUniforms("projection", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(projection));

    particleShader.use();

    particleShader.setUniforms("view", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(view));
    particleShader.setUniforms("projection", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(projection));

    cout << "[FYNiX] FYNiX: Framework for Yet-to-be Named eXperiences is ready!" << endl;

    float deltaTime = 0.0f, lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        static bool lastDirtyState = false;
        if (scene.isDirty != lastDirtyState)
        {
            std::string title = projectName + (scene.isDirty ? " *" : "");
            glfwSetWindowTitle(window, title.c_str());
            lastDirtyState = scene.isDirty;
        }

        // ==== DELTA TIME ====
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ===== GUI SECTION ===
        gui.Start();

        //===== INPUT SECTION =====
        inputHandler(window, deltaTime, globalCamera ? *globalCamera : cam);

        // Synchronize Active Camera with SceneGraph if in-scene camera is selected
        Camera &activeCam = globalCamera ? *globalCamera : cam;
        if (scene.activeCameraID != 0)
        {
            Node *cameraNode = scene.find_node(scene.activeCameraID);
            if (cameraNode && cameraNode->type == NodeType::Camera)
            {
                if (activeCam.cameraLock)
                {
                    // Flying mode: update pending properties
                    scene.cameraChangesPending = true;
                    scene.pendingCamPos = activeCam.camPos;
                    scene.pendingCamYaw = activeCam.yaw;
                    scene.pendingCamPitch = activeCam.pitch;
                }
                else
                {
                    // Not flying mode
                    if (scene.cameraChangesPending)
                    {
                        // Use pending properties so view stays where we flew
                        activeCam.camPos = scene.pendingCamPos;
                        activeCam.yaw = scene.pendingCamYaw;
                        activeCam.pitch = scene.pendingCamPitch;

                        glm::vec3 direction;
                        direction.x = cos(glm::radians(activeCam.yaw)) * cos(glm::radians(activeCam.pitch));
                        direction.y = sin(glm::radians(activeCam.pitch));
                        direction.z = sin(glm::radians(activeCam.yaw)) * cos(glm::radians(activeCam.pitch));
                        activeCam.camTarget = glm::normalize(direction);

                        *activeCam.view = glm::lookAt(activeCam.camPos, activeCam.camPos + activeCam.camTarget, activeCam.camUp);
                    }
                    else
                    {
                        // Sync view to the camera node's actual world transform
                        glm::mat4 cameraWorldMat = scene.getWorldTransform(cameraNode->ID);
                        activeCam.camPos = glm::vec3(cameraWorldMat[3]);
                        activeCam.camTarget = -glm::normalize(glm::vec3(cameraWorldMat[2]));
                        activeCam.camUp = glm::normalize(glm::vec3(cameraWorldMat[1]));

                        activeCam.pitch = glm::degrees(asin(activeCam.camTarget.y));
                        activeCam.yaw = glm::degrees(atan2(activeCam.camTarget.z, activeCam.camTarget.x));

                        *activeCam.view = glm::lookAt(activeCam.camPos, activeCam.camPos + activeCam.camTarget, activeCam.camUp);
                    }
                }
            }
            else
            {
                // Active camera node was deleted or invalid, fall back to Viewport Camera
                if (globalCamera)
                {
                    globalCamera->camPos = scene.viewportCamPos;
                    globalCamera->yaw = scene.viewportYaw;
                    globalCamera->pitch = scene.viewportPitch;
                    globalCamera->camUp = glm::vec3(0.0f, 1.0f, 0.0f);

                    glm::vec3 direction;
                    direction.x = cos(glm::radians(globalCamera->yaw)) * cos(glm::radians(globalCamera->pitch));
                    direction.y = sin(glm::radians(globalCamera->pitch));
                    direction.z = sin(glm::radians(globalCamera->yaw)) * cos(glm::radians(globalCamera->pitch));
                    globalCamera->camTarget = glm::normalize(direction);
                    *globalCamera->view = glm::lookAt(globalCamera->camPos, globalCamera->camPos + globalCamera->camTarget, globalCamera->camUp);
                }
                scene.activeCameraID = 0;
                scene.cameraChangesPending = false;
            }
        }

        defaultShader.use();
        defaultShader.setUniforms("view", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(view));
        defaultShader.setUniforms("uCamPos", static_cast<unsigned int>(UniformType::Vec3f), (void *)glm::value_ptr(globalCamera ? globalCamera->camPos : cam.camPos));

        lightShader.use();
        lightShader.setUniforms("view", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(view));

        particleShader.use();
        particleShader.setUniforms("view", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(view));

        //===== RENDER SECTION =====
        scene.UpdateAsyncLoads();

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        int renderWidth = fbWidth - 355;
        int renderHeight = fbHeight - 285;
        if (renderWidth < 100)
            renderWidth = 100;
        if (renderHeight < 100)
            renderHeight = 100;

        glViewport(0, 285, renderWidth, renderHeight);

        glm::mat4 projection = glm::perspective(glm::radians(45.f), (float)renderWidth / (float)renderHeight, 0.1f, 100.f);

        defaultShader.use();
        defaultShader.setUniforms("projection", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(projection));

        lightShader.use();
        lightShader.setUniforms("projection", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(projection));

        particleShader.use();
        particleShader.setUniforms("projection", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(projection));

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        defaultShader.use();

        if (!scene.models.empty())
            scene.RenderModels(defaultShader, deltaTime);
        if (!scene.lights.empty())
            scene.RenderLights(lightShader);
        scene.RenderCameras(lightShader);
        if (!scene.particleEmitters.empty())
            scene.RenderParticles(deltaTime);
        if (!scene.rigidBodies.empty())
            scene.RenderPhysics(deltaTime, lightShader);

        gui.Render();
        //===== SWAP BUFFERS AND POLL EVENTS ===
        glfwSwapBuffers(window);
    }

    gui.Shutdown();
    glfwTerminate();
    JobSystem::Get().Shutdown();
    return 0;
}