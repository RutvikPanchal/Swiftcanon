#include "GraphicsPipeline.h"

int GraphicsPipeline::id;

GraphicsPipeline::GraphicsPipeline(VulkanInstance* vkInstance, Swapchain* swapchain)
    :vkInstance(vkInstance),
    swapchain(swapchain)
{
    id += 1;

    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
}

GraphicsPipeline::~GraphicsPipeline()
{
    logger.INFO("(MEMORY) Cleaning up Graphics Pipeline");
    logger.TRACE("  Destroying descriptorSetLayout...");
    vkDestroyDescriptorSetLayout(vkInstance->getVulkanLogicalDevice(), descriptorSetLayout, nullptr);
    logger.TRACE("  Destroying graphicsPipeline...");
    vkDestroyPipeline(vkInstance->getVulkanLogicalDevice(), graphicsPipeline, nullptr);
    logger.TRACE("  Destroying graphicsPipelineLayout...");
    vkDestroyPipelineLayout(vkInstance->getVulkanLogicalDevice(), pipelineLayout, nullptr);
    logger.TRACE("  Destroying graphicsPipelineRenderPass...");
    vkDestroyRenderPass(vkInstance->getVulkanLogicalDevice(), renderPass, nullptr);
}

void GraphicsPipeline::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format          = swapchain->getSwapchainImageFormat();
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

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format          = utils::findSupportedFormat(vkInstance->getPhysicalDevice(), {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    depthAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp         = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment   = 1;
    depthAttachmentRef.layout       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass           = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass           = 0;
    dependency.srcStageMask         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask        = 0;
    dependency.dstStageMask         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount  = static_cast<uint32_t>(attachments.size());;
    renderPassInfo.pAttachments     = attachments.data();
    renderPassInfo.subpassCount     = 1;
    renderPassInfo.pSubpasses       = &subpass;
    renderPassInfo.dependencyCount  = 1;
    renderPassInfo.pDependencies    = &dependency;

    VkResult result = vkCreateRenderPass(vkInstance->getVulkanLogicalDevice(), &renderPassInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Render Pass");
    }
}
void GraphicsPipeline::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding            = 0;
    uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount    = 1;
    uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings    = &uboLayoutBinding;

    VkResult result = vkCreateDescriptorSetLayout(vkInstance->getVulkanLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayout);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Descriptor Set Layout");
    }
}
void GraphicsPipeline::createGraphicsPipeline()
{
    std::vector<char> vertShaderCode = utils::readFile("src/shaders/compiled/vert.spv");
    std::vector<char> fragShaderCode = utils::readFile("src/shaders/compiled/frag.spv");

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

    auto bindingDescription = utils::Vertex::getBindingDescription();
    auto attributeDescriptions = utils::Vertex::getAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                             = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology                          = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable            = VK_FALSE;

    VkViewport viewport{};
    viewport.x          = 0.0f;
    viewport.y          = 0.0f;
    viewport.width      = (float) swapchain->getSwapchainExtent().width;
    viewport.height     = (float) swapchain->getSwapchainExtent().height;
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;

    VkRect2D scissor{};
    scissor.offset      = {0, 0};
    scissor.extent      = swapchain->getSwapchainExtent();

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
    rasterizer.frontFace                = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType                  = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable        = VK_TRUE;
    depthStencil.depthWriteEnable       = VK_TRUE;
    depthStencil.depthCompareOp         = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable  = VK_FALSE;
    depthStencil.minDepthBounds         = 0.0f; // Optional
    depthStencil.maxDepthBounds         = 1.0f; // Optional
    depthStencil.stencilTestEnable      = VK_FALSE;
    depthStencil.front                  = {};   // Optional
    depthStencil.back                   = {};   // Optional

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
    pipelineLayoutInfo.setLayoutCount           = 1;
    pipelineLayoutInfo.pSetLayouts              = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount   = 0;        // Optional
    pipelineLayoutInfo.pPushConstantRanges      = nullptr;  // Optional

    VkResult result = vkCreatePipelineLayout(vkInstance->getVulkanLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout);
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
    pipelineInfo.pDepthStencilState     = &depthStencil;
    pipelineInfo.pColorBlendState       = &colorBlending;
    pipelineInfo.pDynamicState          = &dynamicState;
    pipelineInfo.layout                 = pipelineLayout;
    pipelineInfo.renderPass             = renderPass;
    pipelineInfo.subpass                = 0;
    pipelineInfo.basePipelineHandle     = VK_NULL_HANDLE;   // Optional
    pipelineInfo.basePipelineIndex      = -1;               // Optional

    result = vkCreateGraphicsPipelines(vkInstance->getVulkanLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Graphics Pipeline");
    }

    vkDestroyShaderModule(vkInstance->getVulkanLogicalDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(vkInstance->getVulkanLogicalDevice(), vertShaderModule, nullptr);
}

VkShaderModule GraphicsPipeline::createShaderModule(const std::vector<char> &code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(vkInstance->getVulkanLogicalDevice(), &createInfo, nullptr, &shaderModule);
    if (result == VK_SUCCESS) {
        return shaderModule;
    }
    else {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Shader Module");
    }
}
