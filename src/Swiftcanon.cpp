#include "Swiftcanon.h"

#include <iostream>
#include <fstream>
#include <set>
#include <algorithm>
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
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
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
            std::cout << "[VULKAN] Enabling Validation Layers:" << std::endl;
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
        throw std::runtime_error("[VULKAN] Failed to create Logical Device");
    }
}

void Swiftcanon::createSwapChain()
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;

    std::cout << "[VULKAN] SwapChain:" << std::endl;

    // Get capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
    
    // Get surfaceFormats
    uint32_t formatCount;
    std::vector<VkSurfaceFormatKHR> availableFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    std::cout << "[VULKAN]   " << formatCount << " Formats available" << std::endl;
    if (formatCount != 0) {
        availableFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, availableFormats.data());
    }
    // Pick a suitable surfaceFormat, TODO: rank the best and then choose the best available
    for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }
    swapChainImageFormat = surfaceFormat.format;

    // Get presentModes
    uint32_t presentModeCount;
    std::vector<VkPresentModeKHR> availablePresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    std::cout << "[VULKAN]   " << presentModeCount << " Present Modes available" << std::endl;
    if (presentModeCount != 0) {
        availablePresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, availablePresentModes.data());
    }
    // Pick a suitable presentMode, TODO: rank the best and then choose the best available, this should also be user selectable
    presentMode = VK_PRESENT_MODE_FIFO_KHR; // Will always be present
    for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = availablePresentMode;
            break;
        }
    }

    // Get Swap Extent
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent = capabilities.currentExtent;
    }
    else{
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        
        extent = actualExtent;
    }
    swapChainExtent = extent;

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
        throw std::runtime_error("[VULKAN] Failed to create SwapChain");
    }
}

void Swiftcanon::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType                            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image                            = swapChainImages[i];
        createInfo.viewType                         = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format                           = swapChainImageFormat;
        createInfo.components.r                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel    = 0;
        createInfo.subresourceRange.levelCount      = 1;
        createInfo.subresourceRange.baseArrayLayer  = 0;
        createInfo.subresourceRange.layerCount      = 1;

        VkResult result = vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]);
        if (result != VK_SUCCESS) {
            std::cerr << string_VkResult(result) << std::endl;
            throw std::runtime_error("[VULKAN] Failed to create SwapChain ImageViews");
        }
    }
}

void Swiftcanon::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format          = swapChainImageFormat;
    colorAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp         = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment   = 0;
    colorAttachmentRef.layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount  = 1;
    renderPassInfo.pAttachments     = &colorAttachment;
    renderPassInfo.subpassCount     = 1;
    renderPassInfo.pSubpasses       = &subpass;

    VkResult result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Render Pass");
    }
}

void Swiftcanon::createGraphicsPipeline()
{
    std::vector<char> vertShaderCode = readFile("src/shaders/compiled/vert.spv");
    std::vector<char> fragShaderCode = readFile("src/shaders/compiled/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage   = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module  = vertShaderModule;
    vertShaderStageInfo.pName   = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage   = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module  = fragShaderModule;
    fragShaderStageInfo.pName   = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount  = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates     = dynamicStates.data();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 0;
    vertexInputInfo.pVertexBindingDescriptions      = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions    = nullptr; // Optional

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                             = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology                          = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable            = VK_FALSE;

    VkViewport viewport{};
    viewport.x          = 0.0f;
    viewport.y          = 0.0f;
    viewport.width      = (float) swapChainExtent.width;
    viewport.height     = (float) swapChainExtent.height;
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;

    VkRect2D scissor{};
    scissor.offset      = {0, 0};
    scissor.extent      = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                    = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable         = VK_FALSE;
    rasterizer.rasterizerDiscardEnable  = VK_FALSE;
    rasterizer.polygonMode              = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth                = 1.0f;
    rasterizer.cullMode                 = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace                = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable          = VK_FALSE;
    rasterizer.depthBiasConstantFactor  = 0.0f; // Optional
    rasterizer.depthBiasClamp           = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor     = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading      = 1.0f;     // Optional
    multisampling.pSampleMask           = nullptr;  // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable      = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask         = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable            = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor    = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstColorBlendFactor    = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp           = VK_BLEND_OP_ADD;      // Optional
    colorBlendAttachment.srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp           = VK_BLEND_OP_ADD;      // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType                         = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable                 = VK_FALSE;
    colorBlending.logicOp                       = VK_LOGIC_OP_COPY;     // Optional
    colorBlending.attachmentCount               = 1;
    colorBlending.pAttachments                  = &colorBlendAttachment;
    colorBlending.blendConstants[0]             = 0.0f;                 // Optional
    colorBlending.blendConstants[1]             = 0.0f;                 // Optional
    colorBlending.blendConstants[2]             = 0.0f;                 // Optional
    colorBlending.blendConstants[3]             = 0.0f;                 // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                    = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount           = 0;        // Optional
    pipelineLayoutInfo.pSetLayouts              = nullptr;  // Optional
    pipelineLayoutInfo.pushConstantRangeCount   = 0;        // Optional
    pipelineLayoutInfo.pPushConstantRanges      = nullptr;  // Optional

    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Pipeline Layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount             = 2;
    pipelineInfo.pStages                = shaderStages;
    pipelineInfo.pVertexInputState      = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState    = &inputAssembly;
    pipelineInfo.pViewportState         = &viewportState;
    pipelineInfo.pRasterizationState    = &rasterizer;
    pipelineInfo.pMultisampleState      = &multisampling;
    pipelineInfo.pDepthStencilState     = nullptr;          // Optional
    pipelineInfo.pColorBlendState       = &colorBlending;
    pipelineInfo.pDynamicState          = &dynamicState;
    pipelineInfo.layout                 = pipelineLayout;
    pipelineInfo.renderPass             = renderPass;
    pipelineInfo.subpass                = 0;
    pipelineInfo.basePipelineHandle     = VK_NULL_HANDLE;   // Optional
    pipelineInfo.basePipelineIndex      = -1;               // Optional

    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Graphics Pipeline");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void Swiftcanon::createFramebuffers()
{
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = { swapChainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]);
        if (result != VK_SUCCESS) {
            std::cerr << string_VkResult(result) << std::endl;
            throw std::runtime_error("[VULKAN] Failed to create Framebuffer");
        }
    }
}

void Swiftcanon::createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags              = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex   = physicalDeviceIndices.graphicsFamily.value();

    VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create CommandPool");
    }
}

void Swiftcanon::createCommandBuffer()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType                 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool           = commandPool;
    allocInfo.level                 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount    = 1;

    VkResult result = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create CommandBuffer");
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
    std::string* string = new std::string(deviceProperties.deviceName);
    deviceDetails.name = string->c_str();
    deviceDetails.score = 0;
    deviceDetails.deviceIndex = deviceIndex;
    deviceDetails.extensionCount = deviceExtensionCount;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        deviceDetails.score += 1000;
    }
    // Maximum possible size of textures affects graphics quality
    deviceDetails.score += deviceProperties.limits.maxImageDimension2D;

    // Check if device supports required extensions
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
    
    // Check if device supports required queues
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

void Swiftcanon::recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType             = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags             = 0;        // Optional
    beginInfo.pInheritanceInfo  = nullptr;  // Optional

    VkRenderPassBeginInfo renderPassInfo{};
    VkClearValue clearColor             = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.sType                = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass           = renderPass;
    renderPassInfo.framebuffer          = swapChainFramebuffers[image_index];
    renderPassInfo.renderArea.offset    = {0, 0};
    renderPassInfo.renderArea.extent    = swapChainExtent;
    renderPassInfo.clearValueCount      = 1;
    renderPassInfo.pClearValues         = &clearColor;

    VkViewport viewport{};
    viewport.x          = 0.0f;
    viewport.y          = 0.0f;
    viewport.width      = static_cast<float>(swapChainExtent.width);
    viewport.height     = static_cast<float>(swapChainExtent.height);
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;

    VkRect2D scissor{};
    scissor.offset      = {0, 0};
    scissor.extent      = swapChainExtent;

    VkResult result = vkBeginCommandBuffer(command_buffer, &beginInfo);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to initialize recording CommandBuffer");
    }
    vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    vkCmdDraw(command_buffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(command_buffer);
    result = vkEndCommandBuffer(command_buffer);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to record CommandBuffer");
    }
}

void Swiftcanon::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Semaphore");
    }
    result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Semaphore");
    }
    result = vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Fence");
    }
}

VkShaderModule Swiftcanon::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    if (result == VK_SUCCESS) {
        return shaderModule;
    }
    else {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Shader Module");
    }
}

std::vector<char> Swiftcanon::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("[READFILE] Failed to open file: " + filename);
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

void Swiftcanon::drawFrame()
{ }

void Swiftcanon::mainLoop()
{
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
}

void Swiftcanon::cleanup()
{
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
for (VkFramebuffer framebuffer : swapChainFramebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
}
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
for (VkImageView imageView : swapChainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
}
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(vkInstance, surface, nullptr);
    vkDestroyInstance(vkInstance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}