#include "Swiftcanon.h"

#include <iostream>
#include <map>
#include <vulkan/vk_enum_string_helper.h>

Swiftcanon::Swiftcanon()
    :requiredValidationLayers({
        "VK_LAYER_KHRONOS_validation"
    })
{}

void Swiftcanon::init()
{
    initWindow();
    initVulkan();
}

void Swiftcanon::run()
{
    mainLoop();
    cleanup();
}

void Swiftcanon::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    std::cout << "[GLFW] Vulkan Window Created" << std::endl;
}

void Swiftcanon::initVulkan()
{
    addVulkanExtensions();
    addVulkanValidationLayers();
    createVulkanInstance();
    pickPhysicalGraphicsDevice();
}

void Swiftcanon::addVulkanValidationLayers()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    bool layerFound;
    for (const char* layerName : requiredValidationLayers) {
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
            std::cout << "[VULKAN] Adding Validation Layers:" << std::endl;
            for (uint32_t i = 0; i < requiredValidationLayers.size(); i++) {
                std::cout << "[VULKAN]  " << requiredValidationLayers.data()[i] << std::endl;
            }
        }
        else {
            throw std::runtime_error("validation layers requested, but not available!");
        }
    }
}

void Swiftcanon::addVulkanExtensions()
{
    uint32_t extensionCount;
    uint32_t requiredExtensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);

    // MacOS Specific Config
        for(uint32_t i = 0; i < requiredExtensionCount; i++) {
            requiredExtensions.push_back(glfwExtensions[i]);
        }
        requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    std::cout << "[VULKAN] " << extensionCount << " extensions available" << std::endl;
    std::cout << "[VULKAN] " << requiredExtensions.size() << " extensions enabled" << std::endl;
}

void Swiftcanon::createVulkanInstance()
{
    VkInstanceCreateInfo createInfo{};
    createInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.flags                    = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    createInfo.enabledExtensionCount    = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames  = requiredExtensions.data();
    if (enableValidationLayers) {
        createInfo.enabledLayerCount    = static_cast<uint32_t>(requiredValidationLayers.size());
        createInfo.ppEnabledLayerNames  = requiredValidationLayers.data();
    }
    else {
        createInfo.enabledLayerCount    = 0;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &vkInstance);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}

void Swiftcanon::pickPhysicalGraphicsDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("[Vulkan] Failed to find GPUs with Vulkan support");
    }
    else{
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

        for (int i = 0; i < devices.size(); i++) {
            ratePhysicalGraphicsDevices(devices[i], i);
        }

        // TODO: Improve log formatting
        std::cout << "[VULKAN] " << deviceCount << " Physical Graphics Devices Found: " << std::endl;
        for (int i = 0; i < allDeviceDetails.size(); i++) {
            if (i == 0) {
                std::cout << "[VULKAN]  * " << allDeviceDetails[i].name << ", score: " << allDeviceDetails[i].score << std::endl;
            }
            else{
                std::cout << "[VULKAN]    " << allDeviceDetails[i].name << ", score: " << allDeviceDetails[i].score << std::endl;
            }
        }

        // Gets the device with the highest score
        if (allDeviceDetails[0].score > 0){
            physicalDevice = devices[allDeviceDetails[0].deviceIndex];
        }
        else {
            throw std::runtime_error("[Vulkan] Failed to find a suitable GPU");
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("[Vulkan] Failed to find a suitable GPU");
        }
    }
}

// TODO: Massively improve scoring factors to better score the GPUs
void Swiftcanon::ratePhysicalGraphicsDevices(VkPhysicalDevice device, int deviceIndex)
{
    VkPhysicalDeviceFeatures deviceFeatures;
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }
    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    bool queueFamilySupport = false;
    for (int i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilySupport = true;
        }
    }
    if(queueFamilySupport == false){
        score = 0;
    }

    DeviceDetails deviceDetails;
    char* str = new char[strlen(deviceProperties.deviceName)];
    strcpy(str, deviceProperties.deviceName);
    deviceDetails.name = str;
    deviceDetails.score = score;
    deviceDetails.deviceIndex = deviceIndex;
    if (allDeviceDetails.size() == 0){
        allDeviceDetails.push_back(deviceDetails);
    }
    else{
        for (int i = 0; i < allDeviceDetails.size(); i++) {
            if (score > allDeviceDetails[i].score){
                allDeviceDetails.insert(allDeviceDetails.begin() + i, deviceDetails);
                break;
            }
            if (i == allDeviceDetails.size() - 1) {
                allDeviceDetails.push_back(deviceDetails);
                break;
            }
        }
    }
}

void Swiftcanon::mainLoop()
{
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void Swiftcanon::cleanup()
{
    vkDestroyInstance(vkInstance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}