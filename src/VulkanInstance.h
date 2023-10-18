#pragma once
#include "pch.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "Window.h"
#include "Logger.h"

class VulkanInstance
{
private:
    // TODO: Doesn't seem quite perfect, maybe use inheritance or make variables public
    friend class Swapchain;
    friend class CommandDispatcher;
    friend class Swiftcanon;

    struct DeviceDetails {
        const char* name;
        int         deviceIndex;
        int         score;
        uint32_t    extensionCount;
    };
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

public:
    VulkanInstance(Window* windowPtr = nullptr);
   ~VulkanInstance();

    // TODO: remove these maybe?    
    VkInstance      getVkInstance()             const { return vkInstance; }
    VkSurfaceKHR    getWindowSurface()          const { return surface; }
    VkDevice        getVulkanLogicalDevice()    const { return logicalDevice; }
    
private:
    void checkVulkanValidationLayers();
    void configureVulkanExtensions();
    void createVulkanInstance();

    void pickPhysicalGraphicsDevice();
    void ratePhysicalGraphicsDevices();
    void createVulkanLogicalDevice();

private:
    static int              id;
    VkInstance              vkInstance              = VK_NULL_HANDLE;
    VkDevice                logicalDevice           = VK_NULL_HANDLE;
    VkPhysicalDevice        physicalDevice          = VK_NULL_HANDLE;
    DeviceDetails           physicalDeviceDetails;
    QueueFamilyIndices      physicalDeviceIndices;

    VkQueue                 graphicsQueue;
    VkQueue                 presentQueue;

private:
    std::vector<const char*>    vulkanValidationLayers;
    std::vector<const char*>    vulkanExtensions;
    VkInstanceCreateFlags       vulkanInstanceFlags;

    std::vector<const char*>        vulkanDeviceExtensions;
    std::vector<VkPhysicalDevice>   devices;
    std::vector<DeviceDetails>      allDeviceDetails;
    std::vector<QueueFamilyIndices> allDeviceIndices;

private:
    Window*         window  = nullptr;
    VkSurfaceKHR    surface = VK_NULL_HANDLE;

private:
    #ifdef NDEBUG
        const bool enableValidationLayers   = false;
    #else
        const bool enableValidationLayers   = true;
    #endif
    Logger              logger              = Logger((std::string("VULKAN ").append(std::to_string(id))).c_str());
};
