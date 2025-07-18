#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera
{
public:
    glm::mat4 *view;
    glm::vec3 camPos = glm::vec3(0.f, 0.f, 3.f);
    glm::vec3 camTarget = glm::vec3(0.f, 0.f, -1.f);
    glm::vec3 camUp = glm::vec3(0.f, 1.f, 0.f);

    float yaw = -90.f;
    float pitch = 0.f;

    float lastX = 0.0f;
    float lastY = 0.0f;
    bool firstMove = true;
    bool cameraLock = true;

    Camera(glm::mat4 *viewMatrix);

    void processInput(GLFWwindow *window, float deltaTime);
    void mouseInput(double xpos, double ypos);
};