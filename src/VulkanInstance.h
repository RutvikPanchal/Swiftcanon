#pragma once
#include "pch.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "Window.h"
#include "Logger.h"

class VulkanInstance
{
private:
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

    // TODO: remove this
    VkInstance getVkInstance() const { return vkInstance; }

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

    std::vector<const char*>    vulkanValidationLayers;
    std::vector<const char*>    vulkanExtensions;
    VkInstanceCreateFlags       vulkanInstanceFlags;

    std::vector<const char*>        vulkanDeviceExtensions;
    std::vector<VkPhysicalDevice>   devices;
    std::vector<DeviceDetails>      allDeviceDetails;
    std::vector<QueueFamilyIndices> allDeviceIndices;

    VkQueue                         graphicsQueue;
    VkQueue                         presentQueue;

private:
    Window*         window;
    VkSurfaceKHR    surface;

private:
    #ifdef NDEBUG
        const bool enableValidationLayers   = false;
    #else
        const bool enableValidationLayers   = true;
    #endif
    Logger              logger              = Logger((std::string("VULKAN ").append(std::to_string(id))).c_str());
};