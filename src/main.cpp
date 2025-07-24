// default cpp includes
#include <iostream>
#include <windows.h>
#include <dirent.h>

// opengl and related includes
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// project includes
#include "Window.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture.h"

#include "BufferObjects/VertexArray.h"
#include "BufferObjects/VertexBuffer.h"
#include "BufferObjects/ElementBuffer.h"

#include "SceneManager.h"

#include "GUI.h"

using namespace std;

extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

float vertices[] = {
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f};

unsigned int indices[] = {
    0, 1, 2, // triangle 1
    0, 3, 2  // triangle 2
};

Camera *globalCamera = nullptr;

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

int main()
{
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
    string texType = "diffuse";

    Texture obamaTex("assets/obama.png", GL_TEXTURE_2D, 0, texType), trumpTex("assets/trump.png", GL_TEXTURE_2D, 1, texType);

    Shader defaultShader("shaders/basic/vertex.glsl", "shaders/basic/fragment.glsl");
    defaultShader.createProgram();

    defaultShader.use();

    int texUnit0 = 0, texUnit1 = 1;

    defaultShader.setUniforms("texture1", static_cast<unsigned int>(UniformType::Int), (void *)&texUnit0);
    defaultShader.setUniforms("texture2", static_cast<unsigned int>(UniformType::Int), (void *)&texUnit1);

    glm::mat4 model = glm::mat4(1.f);

    glm::mat4 projection = glm::mat4(1.f);
    projection = glm::perspective(glm::radians(45.f), (float)(windowManager.mode->width / windowManager.mode->height), 0.1f, 100.f);

    defaultShader.setUniforms("model", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(model));
    defaultShader.setUniforms("view", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(view));
    defaultShader.setUniforms("projection", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(projection));

    float deltaTime = 0.0f, lastFrame = 0.0f;

    scene.LoadScene(path);

    VertexArray lightVAO;
    VertexBuffer lightVBO(vertices, sizeof(vertices));
    lightVBO.Bind();

    lightVAO.AddAttribLayout(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    lightVAO.AddAttribLayout(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));

    lightVAO.UnBind();
    lightVBO.UnBind();

    cout << "[FYNiX] FYNiX: Framework for Yet-to-be Named eXperiences is ready!" << endl;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        // ==== DELTA TIME ====
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ===== GUI SECTION ===
        gui.Start();

        //===== INPUT SECTION =====
        inputHandler(window, deltaTime, globalCamera ? *globalCamera : cam);
        defaultShader.setUniforms("view", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(view));

        //===== RENDER SECTION =====
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 cubeModel = glm::mat4(1.0f);
        defaultShader.setUniforms("model", (unsigned int)UniformType::Mat4f, glm::value_ptr(cubeModel));

        defaultShader.use();
        obamaTex.Bind(texUnit0);
        defaultShader.setUniforms("texture_diffuse0", static_cast<unsigned int>(UniformType::Int), (void *)&texUnit0);
        lightVAO.Bind();
        lightVBO.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);
        lightVAO.UnBind();
        lightVBO.UnBind();

        defaultShader.use();

        if (scene.models.size() > 0)
            scene.RenderModels(defaultShader);

        gui.Render();
        //===== SWAP BUFFERS AND POLL EVENTS ===
        glfwSwapBuffers(window);
    }

    gui.Shutdown();
    glfwTerminate();
    return 0;
}