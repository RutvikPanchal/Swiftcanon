#pragma once
#include "pch.h"

#include <GLFW/glfw3.h>

#include "Logger.h"

class Window
{
public:
    Window();
   ~Window();

    void    listen()    { glfwPollEvents(); }
    bool    isOpen()    { return !glfwWindowShouldClose(window); }

    void         getFramebufferSize  (int* width, int* height);
    const char** getVulkanExtensions (uint32_t* requiredExtensionCount) { return glfwGetRequiredInstanceExtensions(requiredExtensionCount); }

    bool getFramebufferResized() const      { return framebufferResized; }
    void setFramebufferResized(bool value)  { framebufferResized = value; }

    // TODO: remove this
    GLFWwindow* getNativeWindow() const     { return window; }

private:
    static int  id;

    int         width               = 800;
    int         height              = 600;
    const char* title               = "Vulkan";

    GLFWwindow* window;
    bool        framebufferResized  = false;

    Logger logger = Logger((std::string("WINDOW ").append(std::to_string(id))).c_str());
};