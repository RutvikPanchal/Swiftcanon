#include "Swapchain.h"

int Swapchain::id;

Swapchain::Swapchain(VulkanInstance* vkInstance)
    :vkInstance(vkInstance)
{
    id += 1;

    if (!vkInstance->surface) {
        throw std::runtime_error("[SWAPCHAIN] No window surface found to bind the Swapchain to");
    }
    else{
        getSurfaceCapabilities();
        getSuitableSurfaceFormat();
        getSuitablePresentMode();

        createSwapChain();
        createSwapChainImageViews();
    }
}

Swapchain::~Swapchain()
{
    logger.INFO("(MEMORY) Cleaning up Swapchain");
    logger.TRACE("  Destroying swapChainImageViews...");
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        vkDestroyImageView(vkInstance->logicalDevice, swapChainImageViews[i], nullptr);
    }
    logger.TRACE("  Destroying swapChain...");
    vkDestroySwapchainKHR(vkInstance->logicalDevice, swapChain, nullptr);
}

void Swapchain::getSurfaceCapabilities()
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkInstance->physicalDevice, vkInstance->surface, &capabilities);
}
void Swapchain::getSuitableSurfaceFormat()
{
    // TODO: rank the best and then choose the best available
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkInstance->physicalDevice, vkInstance->surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> availableFormats(formatCount);
    if (formatCount != 0) {
        vkGetPhysicalDeviceSurfaceFormatsKHR(vkInstance->physicalDevice, vkInstance->surface, &formatCount, availableFormats.data());
    }
    for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }
}
void Swapchain::getSuitablePresentMode()
{
    // TODO: rank the best and then choose the best available
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkInstance->physicalDevice, vkInstance->surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
    if (presentModeCount != 0) {
        vkGetPhysicalDeviceSurfacePresentModesKHR(vkInstance->physicalDevice, vkInstance->surface, &presentModeCount, availablePresentModes.data());
    }
    presentMode = VK_PRESENT_MODE_FIFO_KHR; // Will always be present
    for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = availablePresentMode;
            break;
        }
    }
}

void Swapchain::createSwapChain()
{
    // Get Swap Extent
    VkExtent2D extent;
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent = capabilities.currentExtent;
    }
    else{
        int width, height;
        vkInstance->window->getFramebufferSize(&width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        
        extent = actualExtent;
    }

    // Get Image Count
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    // Create Swapchain
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType                        = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface                      = vkInstance->surface;
    createInfo.minImageCount                = imageCount;
    createInfo.imageFormat                  = surfaceFormat.format;
    createInfo.imageColorSpace              = surfaceFormat.colorSpace;
    createInfo.imageExtent                  = extent;
    createInfo.imageArrayLayers             = 1;
    createInfo.imageUsage                   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    uint32_t queueFamilyIndices[] = {
        vkInstance->physicalDeviceIndices.graphicsFamily.value(),
        vkInstance->physicalDeviceIndices.presentFamily.value()
    };
    if (vkInstance->physicalDeviceIndices.graphicsFamily != vkInstance->physicalDeviceIndices.presentFamily) {
        createInfo.imageSharingMode         = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount    = 2;
        createInfo.pQueueFamilyIndices      = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount    = 0;        // Optional
        createInfo.pQueueFamilyIndices      = nullptr;  // Optional
    }
    createInfo.preTransform                 = capabilities.currentTransform;
    createInfo.compositeAlpha               = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode                  = presentMode;
    createInfo.clipped                      = VK_TRUE;
    createInfo.oldSwapchain                 = VK_NULL_HANDLE;
    VkResult result = vkCreateSwapchainKHR(vkInstance->logicalDevice, &createInfo, nullptr, &swapChain);
    if (result == VK_SUCCESS) {
        vkGetSwapchainImagesKHR(vkInstance->logicalDevice, swapChain, &imageCount, nullptr); swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(vkInstance->logicalDevice, swapChain, &imageCount, swapChainImages.data());
    }
    else {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[SWAPCHAIN] Failed to create SwapChain");
    }
}
void Swapchain::createSwapChainImageViews()
{
    // Create SwapchainImageViews
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType                            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image                            = swapChainImages[i];
        createInfo.viewType                         = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format                           = surfaceFormat.format;
        createInfo.components.r                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel    = 0;
        createInfo.subresourceRange.levelCount      = 1;
        createInfo.subresourceRange.baseArrayLayer  = 0;
        createInfo.subresourceRange.layerCount      = 1;

        VkResult result = vkCreateImageView(vkInstance->logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]);
        if (result != VK_SUCCESS) {
            std::cerr << string_VkResult(result) << std::endl;
            throw std::runtime_error("[VULKAN] Failed to create SwapChain ImageViews");
        }
    }
}
