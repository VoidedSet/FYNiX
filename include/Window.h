#pragma once

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Window
{
public:
    Window(const char *title);
    GLFWwindow *getWindowObject();

private:
    const GLFWvidmode *mode;
    GLFWwindow *window;

    void setViewport();
};