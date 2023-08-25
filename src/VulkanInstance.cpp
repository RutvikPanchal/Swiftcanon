#include "VulkanInstance.h"

int VulkanInstance::id;

VulkanInstance::VulkanInstance(Window* windowPtr)
    :window(windowPtr),
    vulkanValidationLayers({
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
    ),
    vulkanDeviceExtensions({
        #ifdef APPLE
            "VK_KHR_portability_subset",
        #endif
    })
{
    id += 1;

    if(window) { vulkanDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME); }

    checkVulkanValidationLayers();
    configureVulkanExtensions();
    createVulkanInstance();

    if(window) { window->createWindowSurface(vkInstance, &surface); }

    ratePhysicalGraphicsDevices();
    pickPhysicalGraphicsDevice();
    createVulkanLogicalDevice();
}

VulkanInstance::~VulkanInstance()
{
    vkDestroyDevice(logicalDevice, nullptr);
    if(window) { vkDestroySurfaceKHR(vkInstance, surface, nullptr); }
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
    const char** glfwExtensions;
    if(window) {
        glfwExtensions = window->getVulkanExtensions(&requiredExtensionCount);
        for (size_t i = 0; i < requiredExtensionCount; i++) {
            vulkanExtensions.push_back(glfwExtensions[i]);
        }
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

// TODO: Massively improve scoring factors to better score the GPUs
void VulkanInstance::ratePhysicalGraphicsDevices()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
    devices.resize(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

    if (deviceCount == 0) {
        throw std::runtime_error("[Vulkan] Failed to find GPUs with Vulkan support");
    }

    VkPhysicalDevice device;
    for (int i = 0; i < devices.size(); i++)
    {
        device = devices[i];
        
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
        std::string* string = new std::string(deviceProperties.deviceName);
        deviceDetails.name = string->c_str();
        deviceDetails.score = 0;
        deviceDetails.deviceIndex = i;
        deviceDetails.extensionCount = deviceExtensionCount;

        // Discrete GPUs have a significant performance advantage
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            deviceDetails.score += 1000;
        }
        // Maximum possible size of textures affects graphics quality
        deviceDetails.score += deviceProperties.limits.maxImageDimension2D;

        // Score based on supported device extensions
        int supportedExtensions = 0;
        for (const char* requiredDeviceExtension : vulkanDeviceExtensions) {
            for (VkExtensionProperties extension : availableDeviceExtensions) {
                if (strcmp(requiredDeviceExtension, extension.extensionName) == 0) {
                    supportedExtensions++;
                    continue;
                }
            }
        }
        if (supportedExtensions != vulkanDeviceExtensions.size()) {
            deviceDetails.score = 0;
            logger.WARN(" Physical Device {0} does not support required Vulkan Extensions, setting score to 0", deviceDetails.name);
        }

        // Score based on supported device queues
        QueueFamilyIndices deviceIndices;
        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            VkBool32 presentSupport = false;
            if(window) {
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            }
            if(presentSupport && (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                deviceIndices.presentFamily = i;
                deviceIndices.graphicsFamily = i;
                break;
            }
            else {
                if (presentSupport) {
                    deviceIndices.presentFamily = i;
                }
                if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    deviceIndices.graphicsFamily = i;
                }
            }
        }
        if(deviceIndices.graphicsFamily.has_value() == false) {
            deviceDetails.score = 0;
            logger.WARN(" Physical Device {0} does not have Vulkan Compute capabilities, setting score to 0", deviceDetails.name);
        }
        if(window && (deviceIndices.isComplete() == false)) {
            deviceDetails.score = 0;
            logger.WARN(" Physical Device {0} does not have Vulkan Compute and Render capabilities, setting score to 0", deviceDetails.name);
        }

        // Insert Entry in DeviceDetails Array
        if (allDeviceDetails.size() == 0) {
            allDeviceDetails.push_back(deviceDetails);
            allDeviceIndices.push_back(deviceIndices);
        }
        else {
            for (size_t i = 0; i < allDeviceDetails.size(); i++) {
                if (deviceDetails.score > allDeviceDetails[i].score) {
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
}
void VulkanInstance::pickPhysicalGraphicsDevice()
{
    logger.TRACE(" {0} Physical Graphics Devices Found:", allDeviceDetails.size());
    for (size_t i = 0; i < allDeviceDetails.size(); i++) {
        if (i == 0) {
            logger.TRACE("   * {0}, score: {1}", allDeviceDetails[i].name, allDeviceDetails[i].score);
        }
        else {
            logger.TRACE("     {0}, score: {1}", allDeviceDetails[i].name, allDeviceDetails[i].score);
        }
    }

    // Default Behavior: Gets the device with the highest score
    if (devices.size() > 0) {
        physicalDevice = devices[allDeviceDetails[0].deviceIndex];
        physicalDeviceDetails = allDeviceDetails[0];
        physicalDeviceIndices = allDeviceIndices[0];
        logger.INFO(" Device Details: {0}", physicalDeviceDetails.name);
        logger.TRACE("   {0} Device Extensions available", physicalDeviceDetails.extensionCount);
        logger.TRACE("   {0} Device Extensions enabled:", vulkanDeviceExtensions.size());
        for (size_t i = 0; i < vulkanDeviceExtensions.size(); i++) {
            logger.TRACE("     {0}", vulkanDeviceExtensions[i]);
        }
        logger.TRACE("   QueueFamily Indices:");
        logger.TRACE("     Graphics:     {0}", physicalDeviceIndices.graphicsFamily.value());
        if(window) { logger.TRACE("     Presentation: {0}", physicalDeviceIndices.presentFamily.value()); }
    }
    else {
        throw std::runtime_error("[Vulkan] Failed to find a suitable GPU");
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("[Vulkan] Failed to find a suitable GPU");
    }
}
void VulkanInstance::createVulkanLogicalDevice()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { physicalDeviceIndices.graphicsFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType               = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex    = queueFamily;
        queueCreateInfo.queueCount          = 1;
        queueCreateInfo.pQueuePriorities    = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pEnabledFeatures         = &deviceFeatures;
    createInfo.queueCreateInfoCount     = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos        = queueCreateInfos.data();
    createInfo.enabledExtensionCount    = static_cast<uint32_t>(vulkanDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames  = vulkanDeviceExtensions.data();
    if (enableValidationLayers) {
        createInfo.enabledLayerCount    = static_cast<uint32_t>(vulkanValidationLayers.size());
        createInfo.ppEnabledLayerNames  = vulkanValidationLayers.data();
    }
    else {
        createInfo.enabledLayerCount    = 0;
    }

    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice);
    if (result == VK_SUCCESS) {
        logger.INFO("Vulkan Logical Device Created");
        vkGetDeviceQueue(logicalDevice, physicalDeviceIndices.graphicsFamily.value(), 0, &graphicsQueue);
        logger.TRACE("  DeviceQueue Bound: Graphics");
        if(window) {
            vkGetDeviceQueue(logicalDevice, physicalDeviceIndices.presentFamily.value(), 0, &presentQueue);
            logger.TRACE("  DeviceQueue Bound: Presentation");
        }
    }
    else {
        logger.ERROR("{0}", string_VkResult(result));
        throw std::runtime_error("[VULKAN] Failed to create Logical Device");
    }
}
