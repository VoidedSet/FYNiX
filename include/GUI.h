#pragma once

#include <streambuf>
#include <iostream>
#include <sstream>
#include <vector>
#include <mutex>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "SceneManager.h"

class GUIManager
{
public:
    GUIManager(GLFWwindow *window, SceneManager &scene, int windowWidth, int windowHeight);
    void Start();
    void Render();
    void Shutdown();

private:
    SceneManager *scene;
    int windowWidth, windowHeight, selectedNodeID = 0;
    static ImGuiIO io;

    void DrawSidePanel(int windowWidth, int windowHeight);
    void DrawAddNodeModal();
    void DrawSceneNode(Node *node);
    void selectedItemInspector(Node *selectedNode);
};

class ImGuiConsoleBuffer;
