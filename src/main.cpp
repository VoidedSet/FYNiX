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

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "ElementBuffer.h"

using namespace std;

float vertices[] = {
    -0.5f, -0.5f, 0.0f, // bottom left
    0.5f, -0.5f, 0.0f,  // bottom right
    0.5f, 0.5f, 0.0f,   // top right
    -0.5f, 0.5f, 0.0f   // top left
};

unsigned int indices[] = {
    0, 1, 2, // triangle 1
    0, 3, 2  // triangle 2
};

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
    ElementBuffer EBO(indices, sizeof(indices));

    VAO.AddAttribLayout(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

    VBO.UnBind();
    VAO.UnBind();
    EBO.UnBind();

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
        EBO.Bind();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        //===== SWAP BUFFERS AND POLL EVENTS ===
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}