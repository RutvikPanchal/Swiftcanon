#include "Window.h"

#include <iostream>

int Window::id;

Window::Window()
{
    id += 1;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    logger.INFO("{0} Window Created", title);
    
    glfwSetWindowUserPointer(window, &framebufferResized);
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