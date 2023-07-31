#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

#include "Logger.h"

class Window
{
public:
    Window();
   ~Window();
    Window(bool* outFramebufferResized);

    void        init();
    void        listen()            { glfwPollEvents(); }
    bool        isOpen()            { return !glfwWindowShouldClose(window); }

    void        getFramebufferSize  (int* width, int* height);
    VkResult    createWindowSurface (VkInstance vkInstance, VkSurfaceKHR* vkSurface);

private:
    int         width   = 800;
    int         height  = 600;
    std::string title   = "Vulkan";

    GLFWwindow* window;
    Logger logger = Logger("WINDOW");
};