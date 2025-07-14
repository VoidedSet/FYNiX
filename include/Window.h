#pragma once

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Window
{
public:
    const GLFWvidmode *mode;

    Window(const char *title);
    GLFWwindow *getWindowObject();

private:
    GLFWwindow *window;

    void setViewport();
};