#include "Window.h"

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

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::getFramebufferSize(int* width, int* height)
{
    glfwGetFramebufferSize(window, width, height);
}