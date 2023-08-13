#pragma once
#include "pch.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <GLFW/glfw3.h>

#include "Window.h"
#include "Logger.h"

class VulkanInstance
{
public:
    VulkanInstance();
   ~VulkanInstance();

   VkInstance getVkInstance() const { return vkInstance; }

private:
    void checkVulkanValidationLayers();
    void configureVulkanExtensions();
    void createVulkanInstance();

private:
    static int          id;
    VkInstance          vkInstance          = VK_NULL_HANDLE;
    VkPhysicalDevice    physicalDevice      = VK_NULL_HANDLE;
    VkDevice            logicalDevice       = VK_NULL_HANDLE;

    std::vector<const char*>    vulkanValidationLayers;
    std::vector<const char*>    vulkanExtensions;
    VkInstanceCreateFlags       vulkanInstanceFlags;

    #ifdef NDEBUG
        const bool enableValidationLayers   = false;
    #else
        const bool enableValidationLayers   = true;
    #endif
    Logger              logger              = Logger((std::string("VULKAN ").append(std::to_string(id))).c_str());
};