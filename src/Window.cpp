#include "Window.h"

#include <iostream>

void Window::init()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
    logger.info("Window Created");
}

Window::Window()
{
    init();
}

Window::Window(bool* outFramebufferResized)
{
    init();
    glfwSetWindowUserPointer(window, outFramebufferResized);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int new_width, int new_height) {
        bool* framebufferResized = reinterpret_cast<bool*>(glfwGetWindowUserPointer(window));
        *framebufferResized = true;
    });
}

void Window::getFramebufferSize(int* width, int* height)
{
    glfwGetFramebufferSize(window, width, height);
}

VkResult Window::createWindowSurface(VkInstance vkInstance, VkSurfaceKHR* vkSurface)
{
    return glfwCreateWindowSurface(vkInstance, window, nullptr, vkSurface);
}

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}