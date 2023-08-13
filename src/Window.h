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

    void    listen()    { glfwPollEvents(); }
    bool    isOpen()    { return !glfwWindowShouldClose(window); }

    void        getFramebufferSize  (int* width, int* height);
    VkResult    createWindowSurface (VkInstance vkInstance, VkSurfaceKHR* vkSurface);

    bool getFramebufferResized() const      { return framebufferResized; }
    void setFramebufferResized(bool value)  { framebufferResized = value; }

private:
    static int  id;

    int         width               = 800;
    int         height              = 600;
    const char* title               = "Vulkan";

    GLFWwindow* window;
    bool        framebufferResized  = false;

    Logger logger = Logger((std::string("WINDOW ").append(std::to_string(id))).c_str());
};