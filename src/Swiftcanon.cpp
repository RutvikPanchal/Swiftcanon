#include "Swiftcanon.h"

#include <iostream>
#include <set>
#include <vulkan/vk_enum_string_helper.h>

Swiftcanon::Swiftcanon()
    :requiredValidationLayers({
        "VK_LAYER_KHRONOS_validation"
    }),
    requiredVulkanExtensions({
        #ifdef APPLE
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
        #endif
    }),
    requiredDeviceExtensions({
        #ifdef APPLE
            "VK_KHR_portability_subset",
        #endif
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
    addVulkanValidationLayers();
    addVulkanInstanceExtensions();
    createVulkanInstance();
    createSurface();
    pickPhysicalGraphicsDevice();
    createVulkanLogicalDevice();
    // createSwapChain();
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
                std::cout << "[VULKAN]   " << requiredValidationLayers.data()[i] << std::endl;
            }
        }
        else {
            throw std::runtime_error("[VULKAN] Validation layers requested, but are not available");
        }
    }
}

void Swiftcanon::addVulkanInstanceExtensions()
{
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::cout << "[VULKAN] " << extensionCount << " Vulkan Instance Extensions available" << std::endl;

    uint32_t requiredExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
    for(uint32_t i = 0; i < requiredExtensionCount; i++) {
        requiredVulkanExtensions.push_back(glfwExtensions[i]);
    }
    std::cout << "[VULKAN] " << requiredVulkanExtensions.size() << " Vulkan Instance Extensions enabled:" << std::endl;

    for(uint32_t i = 0; i < requiredVulkanExtensions.size(); i++) {
        std::cout << "[VULKAN]   " << requiredVulkanExtensions[i] << std::endl;
    }
}

void Swiftcanon::createVulkanInstance()
{
    VkInstanceCreateInfo createInfo{};
    createInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    #ifdef APPLE
        createInfo.flags                = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif
    createInfo.enabledExtensionCount    = static_cast<uint32_t>(requiredVulkanExtensions.size());
    createInfo.ppEnabledExtensionNames  = requiredVulkanExtensions.data();
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
        throw std::runtime_error("[VULKAN] Failed to create Vulkan instance");
    }
}

void Swiftcanon::createSurface()
{
    VkResult result = glfwCreateWindowSurface(vkInstance, window, nullptr, &surface);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Window Surface");
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
                std::cout << "[VULKAN]   * " << allDeviceDetails[i].name << ", score: " << allDeviceDetails[i].score << std::endl;
            }
            else{
                std::cout << "[VULKAN]     " << allDeviceDetails[i].name << ", score: " << allDeviceDetails[i].score << std::endl;
            }
        }

        // Gets the device with the highest score
        if (allDeviceDetails[0].score > 0){
            physicalDevice = devices[allDeviceDetails[0].deviceIndex];
            physicalDeviceDetails = allDeviceDetails[0];
            physicalDeviceIndices = allDeviceIndices[0];
            std::cout << "[VULKAN] Device Details: " << physicalDeviceDetails.name << std::endl;
            std::cout << "[VULKAN]   QueueFamily Indices:" << std::endl;
            std::cout << "[VULKAN]     Graphics:     " << physicalDeviceIndices.graphicsFamily.value() << std::endl;
            std::cout << "[VULKAN]     Presentation: " << physicalDeviceIndices.presentFamily.value() << std::endl;
        }
        else {
            throw std::runtime_error("[Vulkan] Failed to find a suitable GPU");
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("[Vulkan] Failed to find a suitable GPU");
        }
    }
}

void Swiftcanon::createVulkanLogicalDevice()
{
    std::cout << "[VULKAN] " << physicalDeviceDetails.extensionCount << " Device Extensions available" << std::endl;

    std::cout << "[VULKAN] " << requiredDeviceExtensions.size() << " Device Extensions enabled:" << std::endl;
    for (int i = 0; i < requiredDeviceExtensions.size(); i++) {
        std::cout << "[VULKAN]   " << requiredDeviceExtensions[i] << std::endl;
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { physicalDeviceIndices.graphicsFamily.value(), physicalDeviceIndices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
        createInfo.ppEnabledLayerNames = requiredValidationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
    if (result == VK_SUCCESS) {
        vkGetDeviceQueue(device, physicalDeviceIndices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, physicalDeviceIndices.presentFamily.value(), 0, &presentQueue);
    }
    else {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create logical device");
    }
}

void Swiftcanon::createSwapChain()
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;

    // Get capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
    
    // Get surfaceFormats
    uint32_t formatCount;
    std::vector<VkSurfaceFormatKHR> availableFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        availableFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, availableFormats.data());
    }

    // Get presentModes
    uint32_t presentModeCount;
    std::vector<VkPresentModeKHR> availablePresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        availablePresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, availablePresentModes.data());
    }

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType                        = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface                      = surface;
    createInfo.minImageCount                = imageCount;
    createInfo.imageFormat                  = surfaceFormat.format;
    createInfo.imageColorSpace              = surfaceFormat.colorSpace;
    createInfo.imageExtent                  = extent;
    createInfo.imageArrayLayers             = 1;
    createInfo.imageUsage                   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    uint32_t queueFamilyIndices[] = {
        physicalDeviceIndices.graphicsFamily.value(),
        physicalDeviceIndices.presentFamily.value()
    };
    if (physicalDeviceIndices.graphicsFamily != physicalDeviceIndices.presentFamily) {
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

    VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
    if (result == VK_SUCCESS) {
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr); swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
    }
    else {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create logical device");
    }
}

// TODO: Massively improve scoring factors to better score the GPUs
void Swiftcanon::ratePhysicalGraphicsDevices(VkPhysicalDevice device, int deviceIndex)
{
    VkPhysicalDeviceFeatures deviceFeatures;
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    uint32_t deviceExtensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> availableDeviceExtensions(deviceExtensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, availableDeviceExtensions.data());

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    DeviceDetails deviceDetails;
    char* str = new char[strlen(deviceProperties.deviceName)];
    strcpy(str, deviceProperties.deviceName);
    deviceDetails.name = str;
    deviceDetails.score = 0;
    deviceDetails.deviceIndex = deviceIndex;
    deviceDetails.extensionCount = deviceExtensionCount;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        deviceDetails.score += 1000;
    }
    // Maximum possible size of textures affects graphics quality
    deviceDetails.score += deviceProperties.limits.maxImageDimension2D;

    // Check if device suports required extensions
    int supportedExtensions = 0;
    for (const char* requiredDeviceExtension : requiredDeviceExtensions) {
        for (VkExtensionProperties extension : availableDeviceExtensions) {
            if (strcmp(requiredDeviceExtension, extension.extensionName) == 0) {
                supportedExtensions++;
                continue;
            }
        }
    }
    if (supportedExtensions != requiredDeviceExtensions.size()) {
        deviceDetails.score = 0;
        std::cout << "[VULKAN] WARNING: Physical Device " << deviceDetails.name << " does not support required Vulkan Extensions, setting score to 0" << std::endl;
    }
    
    // Check if device suports required queues
    VkBool32 presentSupport;
    QueueFamilyIndices deviceIndices;
    for (int i = 0; i < queueFamilies.size(); i++) {
        presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if(presentSupport && (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)){
            deviceIndices.presentFamily = i;
            deviceIndices.graphicsFamily = i;
            break;
        }
        else{
            if (presentSupport) {
                deviceIndices.presentFamily = i;
            }
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                deviceIndices.graphicsFamily = i;
            }
        }
    }
    if(deviceIndices.isComplete() == false){
        deviceDetails.score = 0;
        std::cout << "[VULKAN] WARNING: Physical Device " << deviceDetails.name << " does not have Vulkan Compute and Render capabilities, setting score to 0" << std::endl;
    }

    // Insert Entry in DeviceDetails Array
    if (allDeviceDetails.size() == 0){
        allDeviceDetails.push_back(deviceDetails);
        allDeviceIndices.push_back(deviceIndices);
    }
    else{
        for (int i = 0; i < allDeviceDetails.size(); i++) {
            if (deviceDetails.score > allDeviceDetails[i].score){
                allDeviceDetails.insert(allDeviceDetails.begin() + i, deviceDetails);
                allDeviceIndices.insert(allDeviceIndices.begin() + i, deviceIndices);
                break;
            }
            if (i == allDeviceDetails.size() - 1) {
                allDeviceDetails.push_back(deviceDetails);
                allDeviceIndices.push_back(deviceIndices);
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
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(vkInstance, surface, nullptr);
    vkDestroyInstance(vkInstance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}