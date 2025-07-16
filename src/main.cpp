// default cpp includes
#include <iostream>

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

#include "Model.h"

using namespace std;

// float vertices[] = {
//     -0.5f, -0.5f, 0.0f, // bottom left
//     0.5f, -0.5f, 0.0f,  // bottom right
//     0.5f, 0.5f, 0.0f,   // top right
//     -0.5f, 0.5f, 0.0f   // top left
// };

// float vertices[] = {
//     // positions // colors // texture coords
//     0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // top right
//     0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // bottom right
//     -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
//     -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f   // top left
// };

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

glm::vec3 cubePositions[] = {
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(2.0f, 5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3(2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f, 3.0f, -7.5f),
    glm::vec3(1.3f, -2.0f, -2.5f),
    glm::vec3(1.5f, 2.0f, -2.5f),
    glm::vec3(1.5f, 0.2f, -1.5f),
    glm::vec3(-1.3f, 1.0f, -1.5f)};

unsigned int indices[] = {
    0, 1, 2, // triangle 1
    0, 3, 2  // triangle 2
};

Camera *globalCamera = nullptr;

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (globalCamera)
        globalCamera->mouseInput(xpos, ypos);
}

void inputHandler(GLFWwindow *window, float deltaTime, Camera &camera)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    camera.processInput(window, deltaTime);
}

int main()
{
    if (!glfwInit())
    {
        cout << "Failed to initialize GLFW" << endl;
        return -1;
    }

    Window windowManager("FYNiX - Framework for Yet-to-be Named eXperiences");
    GLFWwindow *window = windowManager.getWindowObject();

    glm::mat4 view = glm::mat4(1.f);
    Camera cam(&view);
    globalCamera = &cam;

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    Texture obamaTex("assets/obama.png", GL_TEXTURE_2D, 0), trumpTex("assets/trump.png", GL_TEXTURE_2D, 1);

    VertexArray VAO;
    VertexBuffer VBO(vertices, sizeof(vertices));
    ElementBuffer EBO(indices, sizeof(indices));

    VAO.AddAttribLayout(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    // VAO.AddAttribLayout(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    VAO.AddAttribLayout(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));

    VBO.UnBind();
    VAO.UnBind();
    EBO.UnBind();

    Shader defaultShader("shaders/basic/vertex.glsl", "shaders/basic/fragment.glsl");
    defaultShader.createProgram();

    defaultShader.use();

    int texUnit0 = 0, texUnit1 = 1;

    defaultShader.setUniforms("texture1", static_cast<unsigned int>(UniformType::Int), (void *)&texUnit0);
    defaultShader.setUniforms("texture2", static_cast<unsigned int>(UniformType::Int), (void *)&texUnit1);

    glm::mat4 model = glm::mat4(1.f);
    model = glm::rotate(model, glm::radians(-55.f), glm::vec3(1.f, 0.f, 0.f));

    glm::mat4 projection = glm::mat4(1.f);
    projection = glm::perspective(glm::radians(45.f), (float)(windowManager.mode->width / windowManager.mode->height), 0.1f, 100.f);

    defaultShader.setUniforms("model", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(model));
    defaultShader.setUniforms("view", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(view));
    defaultShader.setUniforms("projection", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(projection));

    float deltaTime = 0.0f, lastFrame = 0.0f;

    Model testModel("assets/hand2.glb");

    while (!glfwWindowShouldClose(window))
    {
        // ==== DELTA TIME ====
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //===== INPUT SECTION =====
        inputHandler(window, deltaTime, globalCamera ? *globalCamera : cam);
        defaultShader.setUniforms("view", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(view));

        //===== RENDER SECTION =====
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        defaultShader.use();

        float timeValue = glfwGetTime();
        float greenValue = sin(timeValue) / 2.0f + 0.5f;
        glm::vec3 color = glm::vec3(0.0f, greenValue, 0.0f);
        defaultShader.setUniforms("uColor", static_cast<unsigned int>(UniformType::Vec3f), (void *)glm::value_ptr(color));

        testModel.Draw(defaultShader);

        // obamaTex.Bind();
        // trumpTex.Bind();
        // VAO.Bind();
        // // EBO.Bind();
        // // glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
        // for (unsigned int i = 0; i < 10; i++)
        // {
        //     glm::mat4 model = glm::mat4(1.0f);
        //     model = glm::translate(model, cubePositions[i]);
        //     float angle = 20.0f * i;
        //     model = glm::rotate(model, glm::radians(angle),
        //                         glm::vec3(1.0f, 0.3f, 0.5f));
        //     defaultShader.setUniforms("model", static_cast<unsigned int>(UniformType::Mat4f), (void *)glm::value_ptr(model));
        //     glDrawArrays(GL_TRIANGLES, 0, 36);
        // }

        //===== SWAP BUFFERS AND POLL EVENTS ===
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}