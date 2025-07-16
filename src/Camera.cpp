#include "Camera.h"

Camera::Camera(glm::mat4 *viewMatrix) : view(viewMatrix)
{
    *view = glm::lookAt(camPos, camPos + camTarget, camUp);
}

void Camera::processInput(GLFWwindow *window, float deltaTime)
{
    const float camSpeed = 20.f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camPos += camSpeed * camTarget;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camPos -= camSpeed * camTarget;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camPos -= glm::normalize(glm::cross(camTarget, camUp)) * camSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camPos += glm::normalize(glm::cross(camTarget, camUp)) * camSpeed;

    *view = glm::lookAt(camPos, camPos + camTarget, camUp);
}

void Camera::mouseInput(double xpos, double ypos)
{
    if (firstMove)
    {
        lastX = xpos;
        lastY = ypos;
        firstMove = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // y is inverted

    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // clamp pitch
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camTarget = glm::normalize(direction);

    *view = glm::lookAt(camPos, camPos + camTarget, camUp);
}