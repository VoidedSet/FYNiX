#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

class GUIManager
{
public:
    ImGuiIO io;
    int windowWidth, windowHeight;

    GUIManager(GLFWwindow *window, int windowWidth, int windowHeight);
    void Start();
    void Render();
    void Shutdown();
};