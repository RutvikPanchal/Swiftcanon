#pragma once
#include "pch.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "VulkanInstance.h"

class Swapchain
{
    // TODO: Doesn't seem quite perfect, maybe use inheritance or make variables public
    friend class CommandDispatcher;
    friend class GraphicsPipeline;
    friend class Swiftcanon;
    
public:
    Swapchain(VulkanInstance* vkInstance);
   ~Swapchain();

    VkSwapchainKHR              getSwapchain()              const { return swapChain; }
    VkExtent2D                  getSwapchainExtent()        const { return swapChainExtent; }
    VkFormat                    getSwapchainImageFormat()   const { return swapChainImageFormat; }
    std::vector<VkFramebuffer>  getSwapchainFrameBuffers()  const { return swapChainFramebuffers; }
   
    void        recreateSwapchain();
    void        createSwapchainFramebuffers(VkRenderPass renderPass, VkImageView depthImageView);

private:
    void getSurfaceCapabilities();
    void getSuitableSurfaceFormat();
    void getSuitablePresentMode();
    
    void createSwapchain();
    void createSwapchainImageViews();

private:
    static int                  id;
    VkSwapchainKHR              swapChain;
    std::vector<VkImage>        swapChainImages;
    std::vector<VkImageView>    swapChainImageViews;

    std::vector<VkFramebuffer>  swapChainFramebuffers;

    VkSurfaceCapabilitiesKHR    capabilities;
    VkSurfaceFormatKHR          surfaceFormat;
    VkPresentModeKHR            presentMode;

    VkFormat                    swapChainImageFormat;
    VkExtent2D                  swapChainExtent;

private:
    VulkanInstance* vkInstance;

private:
    Logger logger = Logger((std::string("SWAPCHAIN ").append(std::to_string(id))).c_str());
};
