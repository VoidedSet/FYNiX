#include "Window.h"

using namespace std;

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

#include "stb_image.h"

Window::Window(const char *title)
{
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
    mode = glfwGetVideoMode(primaryMonitor);

    window = glfwCreateWindow(mode->width, mode->height, title, NULL, NULL);
    if (window == NULL)
    {
        cout << "[Window - ERROR] Failed to open GLFW window" << endl;
        return;
    }
    else
        cout << "[FYNiX] Launching FYNiX: Framework for Yet-to-be Named eXperiences." << endl;

    // Load and set window icon
    stbi_set_flip_vertically_on_load(false);
    int iconWidth, iconHeight, iconChannels;
    unsigned char* iconPixels = stbi_load("fynix.png", &iconWidth, &iconHeight, &iconChannels, 4);
    if (iconPixels)
    {
        GLFWimage iconImage[1];
        iconImage[0].width = iconWidth;
        iconImage[0].height = iconHeight;
        iconImage[0].pixels = iconPixels;
        glfwSetWindowIcon(window, 1, iconImage);
        stbi_image_free(iconPixels);
    }
    else
    {
        cout << "[Window - WARNING] Failed to load window icon fynix.png" << endl;
    }

    glfwMaximizeWindow(window);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "[Window - ERROR] Failed to initialize GLAD" << endl;
        return;
    }

    glEnable(GL_MULTISAMPLE);

    Window::setViewport();
}

void Window::setViewport()
{
    glViewport(0, 0, mode->width, mode->height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
}

GLFWwindow *Window::getWindowObject()
{
    return window;
}
