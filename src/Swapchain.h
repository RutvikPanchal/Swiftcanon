#pragma once
#include "pch.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "VulkanInstance.h"

class Swapchain
{
public:
    Swapchain(VulkanInstance* vkInstance);
   ~Swapchain();

private:
    void getSurfaceCapabilities();
    void getSuitableSurfaceFormat();
    void getSuitablePresentMode();
    
    void createSwapChain();
    void createSwapChainImageViews();

private:
    static int                  id;
    VkSwapchainKHR              swapChain;
    std::vector<VkImage>        swapChainImages;
    std::vector<VkImageView>    swapChainImageViews;

    VkSurfaceCapabilitiesKHR    capabilities;
    VkSurfaceFormatKHR          surfaceFormat;
    VkPresentModeKHR            presentMode;

private:
    VulkanInstance* vkInstance;

private:
    Logger logger = Logger((std::string("SWAPCHAIN ").append(std::to_string(id))).c_str());
};