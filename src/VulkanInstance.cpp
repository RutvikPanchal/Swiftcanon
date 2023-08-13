#include "VulkanInstance.h"

#include <iostream>

int VulkanInstance::id;

VulkanInstance::VulkanInstance()
    :vulkanValidationLayers({
        "VK_LAYER_KHRONOS_validation"
    }),
    vulkanExtensions({
        #ifdef APPLE
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
        #endif
    }),
    vulkanInstanceFlags(
        #ifdef APPLE
            VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
        #endif
    )
{
    id += 1;
    checkVulkanValidationLayers();
    configureVulkanExtensions();
    createVulkanInstance();
}

VulkanInstance::~VulkanInstance()
{
    vkDestroyInstance(vkInstance, nullptr);
}

void VulkanInstance::checkVulkanValidationLayers()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    bool layerFound;
    for (const char* layerName : vulkanValidationLayers) {
        layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
    }
    if (enableValidationLayers) {
        if (layerFound) {
            logger.TRACE(" Enabling Validation Layers:");
            for (size_t i = 0; i < vulkanValidationLayers.size(); i++) {
                logger.TRACE("   {0}", vulkanValidationLayers.data()[i]);
            }
        }
        else {
            throw std::runtime_error("[VULKAN] Validation layers requested, but are not available");
        }
    }
}
void VulkanInstance::configureVulkanExtensions()
{
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    logger.TRACE(" {0} Vulkan Instance Extensions available", extensionCount);

    uint32_t requiredExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
    for (size_t i = 0; i < requiredExtensionCount; i++) {
        vulkanExtensions.push_back(glfwExtensions[i]);
    }
    logger.TRACE(" {0} Vulkan Instance Extensions enabled:", vulkanExtensions.size());

    for (size_t i = 0; i < vulkanExtensions.size(); i++) {
        logger.TRACE("   {0}", vulkanExtensions[i]);
    }
}
void VulkanInstance::createVulkanInstance()
{
    VkInstanceCreateInfo createInfo{};
    createInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.flags                    = vulkanInstanceFlags;
    createInfo.enabledExtensionCount    = static_cast<uint32_t>(vulkanExtensions.size());
    createInfo.ppEnabledExtensionNames  = vulkanExtensions.data();
    if (enableValidationLayers) {
        createInfo.enabledLayerCount    = static_cast<uint32_t>(vulkanValidationLayers.size());
        createInfo.ppEnabledLayerNames  = vulkanValidationLayers.data();
    }
    else {
        createInfo.enabledLayerCount    = 0;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &vkInstance);
    if (result == VK_SUCCESS) {
        logger.INFO("Vulkan Instance Created");
    }
    else {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Vulkan instance");
    }
}
