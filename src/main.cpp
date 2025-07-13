#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Window.h"
#include "Shader.h"

#include "VertexArray.h"
#include "VertexBuffer.h"

using namespace std;

const float vertices[] = {
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    0.0f, 0.5f, 0.0f};

void inputHandler(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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

    VertexArray VAO;
    VertexBuffer VBO(vertices, sizeof(vertices));

    VAO.AddAttribLayout(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

    VBO.UnBind();
    VAO.UnBind();

    Shader defaultShader("shaders/basic/vertex.glsl", "shaders/basic/fragment.glsl");
    defaultShader.createProgram();

    while (!glfwWindowShouldClose(window))
    {
        //===== INPUT SECTION =====
        inputHandler(window);

        //===== RENDER SECTION =====
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        defaultShader.use();
        VAO.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);

        //===== SWAP BUFFERS AND POLL EVENTS ===
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}