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
    logger.INFO("(MEMORY) Cleaning up Window");
    logger.TRACE("  Destroying window...");
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::getFramebufferSize(int* width, int* height)
{
    glfwGetFramebufferSize(window, width, height);
}

const char** Window::getVulkanExtensions(uint32_t *requiredExtensionCount)
{
    return glfwGetRequiredInstanceExtensions(requiredExtensionCount);
}

void Window::createWindowSurface(VkInstance vkInstance, VkSurfaceKHR *vkSurface)
{
    VkResult result = glfwCreateWindowSurface(vkInstance, window, nullptr, vkSurface);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[VULKAN] Failed to create Window Surface");
    }
}
