#include "Swiftcanon.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <vulkan/vk_enum_string_helper.h>

Swiftcanon::Swiftcanon()
{
}

void Swiftcanon::init()
{
    initVulkan();
}

void Swiftcanon::run()
{
    mainLoop();
    cleanup();
}

void Swiftcanon::initVulkan()
{
    createDepthResources();
    vulkanSwapchain.createSwapchainFramebuffers(vulkanGraphicsPipeline.renderPass, depthImageView);
    createCommandPool(); // done
    createCommandBuffer(); // done
    loadModel("src/models/bunny.obj");
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createSyncObjects(); // done
}

void Swiftcanon::recreateSwapchain()
{
    int width = 0, height = 0;
    window.getFramebufferSize(&width, &height);
    while (width == 0 || height == 0) {
        window.getFramebufferSize(&width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(vulkanInstance.logicalDevice);
    
    cleanupDepthResources();

    vulkanSwapchain.recreateSwapchain();
    createDepthResources();
    vulkanSwapchain.createSwapchainFramebuffers(vulkanGraphicsPipeline.renderPass, depthImageView);
}

void Swiftcanon::cleanupDepthResources()
{
    vkDestroyImageView(vulkanInstance.logicalDevice, depthImageView, nullptr);
    vkDestroyImage(vulkanInstance.logicalDevice, depthImage, nullptr);
    vkFreeMemory(vulkanInstance.logicalDevice, depthImageMemory, nullptr);
}

void Swiftcanon::createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags              = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex   = vulkanInstance.physicalDeviceIndices.graphicsFamily.value();

    VkResult result = vkCreateCommandPool(vulkanInstance.logicalDevice, &poolInfo, nullptr, &commandPool);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create CommandPool");
    }
}

void Swiftcanon::createCommandBuffer()
{
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType                 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool           = commandPool;
    allocInfo.level                 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount    = static_cast<uint32_t>(commandBuffers.size());

    VkResult result = vkAllocateCommandBuffers(vulkanInstance.logicalDevice, &allocInfo, commandBuffers.data());
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create CommandBuffer");
    }
}

void Swiftcanon::createDepthResources()
{
    VkFormat depthFormat = utils::findSupportedFormat(
        vulkanInstance.physicalDevice,
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

    createImage(
        vulkanSwapchain.getSwapchainExtent().width,
        vulkanSwapchain.getSwapchainExtent().height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depthImage,
        depthImageMemory
    );

    depthImageView = createImageView(
        depthImage,
        depthFormat,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
}

void Swiftcanon::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data;
    vkMapMemory(vulkanInstance.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(vulkanInstance.logicalDevice, stagingBufferMemory);

    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertexBuffer,
        vertexBufferMemory
    );

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    vkDestroyBuffer(vulkanInstance.logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(vulkanInstance.logicalDevice, stagingBufferMemory, nullptr);
}

void Swiftcanon::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory
    );

    void* data;
    vkMapMemory(vulkanInstance.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(vulkanInstance.logicalDevice, stagingBufferMemory);

    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indexBuffer,
        indexBufferMemory
    );

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(vulkanInstance.logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(vulkanInstance.logicalDevice, stagingBufferMemory, nullptr);
}

void Swiftcanon::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniformBuffers[i],
            uniformBuffersMemory[i]
        );
        vkMapMemory(
            vulkanInstance.logicalDevice,
            uniformBuffersMemory[i],
            0,
            bufferSize,
            0,
            &uniformBuffersMapped[i]
        );
    }
}

void Swiftcanon::createDescriptorPool()
{
    VkDescriptorPoolSize poolSize{};
    poolSize.type               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount    = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount  = 1;
    poolInfo.pPoolSizes     = &poolSize;
    poolInfo.maxSets        = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    
    VkResult result = vkCreateDescriptorPool(vulkanInstance.logicalDevice, &poolInfo, nullptr, &descriptorPool);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create DescriptorPool");
    }
}

void Swiftcanon::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, vulkanGraphicsPipeline.descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType                 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool        = descriptorPool;
    allocInfo.descriptorSetCount    = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts           = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    VkResult result = vkAllocateDescriptorSets(vulkanInstance.logicalDevice, &allocInfo, descriptorSets.data());
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to allocate DescriptorSets");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer   = uniformBuffers[i];
        bufferInfo.offset   = 0;
        bufferInfo.range    = sizeof(UniformBufferObject);
    
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType               = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet              = descriptorSets[i];
        descriptorWrite.dstBinding          = 0;
        descriptorWrite.dstArrayElement     = 0;
        descriptorWrite.descriptorType      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount     = 1;
        descriptorWrite.pBufferInfo         = &bufferInfo;
        descriptorWrite.pImageInfo          = nullptr;      // Optional
        descriptorWrite.pTexelBufferView    = nullptr;      // Optional

        vkUpdateDescriptorSets(vulkanInstance.logicalDevice, 1, &descriptorWrite, 0, nullptr);
    }
}

void Swiftcanon::recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType             = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags             = 0;        // Optional
    beginInfo.pInheritanceInfo  = nullptr;  // Optional

    VkRenderPassBeginInfo renderPassInfo{};
    std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color            = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil     = {1.0f, 0};
    renderPassInfo.sType                = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass           = vulkanGraphicsPipeline.renderPass;
    renderPassInfo.framebuffer          = vulkanSwapchain.swapChainFramebuffers[image_index];
    renderPassInfo.renderArea.offset    = {0, 0};
    renderPassInfo.renderArea.extent    = vulkanSwapchain.getSwapchainExtent();
    renderPassInfo.clearValueCount      = static_cast<uint32_t>(clearValues.size());;
    renderPassInfo.pClearValues         = clearValues.data();

    VkViewport viewport{};
    viewport.x          = 0.0f;
    viewport.y          = 0.0f;
    viewport.width      = static_cast<float>(vulkanSwapchain.getSwapchainExtent().width);
    viewport.height     = static_cast<float>(vulkanSwapchain.getSwapchainExtent().height);
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;

    VkRect2D scissor{};
    scissor.offset      = {0, 0};
    scissor.extent      = vulkanSwapchain.getSwapchainExtent();

    VkResult result = vkBeginCommandBuffer(command_buffer, &beginInfo);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to initialize recording CommandBuffer");
    }

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBeginRenderPass        (command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline           (command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanGraphicsPipeline.graphicsPipeline);
    vkCmdBindVertexBuffers      (command_buffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer        (command_buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets     (command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanGraphicsPipeline.pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
    vkCmdSetViewport            (command_buffer, 0, 1, &viewport);
    vkCmdSetScissor             (command_buffer, 0, 1, &scissor);
    vkCmdDrawIndexed            (command_buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    vkCmdEndRenderPass          (command_buffer);
    result = vkEndCommandBuffer (command_buffer);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to record CommandBuffer");
    }
}

void Swiftcanon::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkResult result = vkCreateSemaphore(vulkanInstance.logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
        if (result != VK_SUCCESS) {
            std::cerr << string_VkResult(result) << std::endl;
            throw std::runtime_error("[VULKAN] Failed to create Semaphore");
        }
        result = vkCreateSemaphore(vulkanInstance.logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
        if (result != VK_SUCCESS) {
            std::cerr << string_VkResult(result) << std::endl;
            throw std::runtime_error("[VULKAN] Failed to create Semaphore");
        }
        result = vkCreateFence(vulkanInstance.logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]);
        if (result != VK_SUCCESS) {
            std::cerr << string_VkResult(result) << std::endl;
            throw std::runtime_error("[VULKAN] Failed to create Fence");
        }
    }
}

VkShaderModule Swiftcanon::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(vulkanInstance.logicalDevice, &createInfo, nullptr, &shaderModule);
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

void Swiftcanon::loadModel(const char* path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path)) {
        throw std::runtime_error(warn + err);
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.color = {
                attrib.normals[3 * index.vertex_index + 0],
                attrib.normals[3 * index.vertex_index + 1],
                attrib.normals[3 * index.vertex_index + 2]
            };

            vertices.push_back(vertex);
            indices.push_back(indices.size());
        }
    }
}

uint32_t Swiftcanon::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkanInstance.physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("[Vulkan] Failed to find suitable Memory Type");
}

void Swiftcanon::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType        = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size         = size;
    bufferInfo.usage        = usage;
    bufferInfo.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(vulkanInstance.logicalDevice, &bufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkanInstance.logicalDevice, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize    = memRequirements.size;
    allocInfo.memoryTypeIndex   = findMemoryType(memRequirements.memoryTypeBits, properties);

    result = vkAllocateMemory(vulkanInstance.logicalDevice, &allocInfo, nullptr, &bufferMemory);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to allocate Buffer Memory");
    }

    vkBindBufferMemory(vulkanInstance.logicalDevice, buffer, bufferMemory, 0);
}

void Swiftcanon::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType                 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level                 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool           = commandPool;
    allocInfo.commandBufferCount    = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vulkanInstance.logicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType                 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                 = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset            = 0; // Optional
    copyRegion.dstOffset            = 0; // Optional
    copyRegion.size                 = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffer;
    vkQueueSubmit(vulkanInstance.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    
    vkQueueWaitIdle(vulkanInstance.graphicsQueue);
    vkFreeCommandBuffers(vulkanInstance.logicalDevice, commandPool, 1, &commandBuffer);
}

void Swiftcanon::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                            VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                            VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = format;
    imageInfo.tiling        = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = usage;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateImage(vulkanInstance.logicalDevice, &imageInfo, nullptr, &image);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vulkanInstance.logicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize    = memRequirements.size;
    allocInfo.memoryTypeIndex   = findMemoryType(memRequirements.memoryTypeBits, properties);

    result = vkAllocateMemory(vulkanInstance.logicalDevice, &allocInfo, nullptr, &imageMemory);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to allocate Image Memory");
    }

    vkBindImageMemory(vulkanInstance.logicalDevice, image, imageMemory, 0);
}

VkImageView Swiftcanon::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                              = image;
    viewInfo.viewType                           = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                             = format;
    viewInfo.subresourceRange.aspectMask        = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel      = 0;
    viewInfo.subresourceRange.levelCount        = 1;
    viewInfo.subresourceRange.baseArrayLayer    = 0;
    viewInfo.subresourceRange.layerCount        = 1;

    VkImageView imageView;
    VkResult result = vkCreateImageView(vulkanInstance.logicalDevice, &viewInfo, nullptr, &imageView);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create Texture Image View");
    }

    return imageView;
}

void Swiftcanon::mainLoop()
{
    while (window.isOpen()) {
        window.listen();
        drawFrame();
        // vulkanCommandDispatcher.dispatchCommand();
    }
    vkDeviceWaitIdle(vulkanInstance.logicalDevice);
}

void Swiftcanon::drawFrame()
{
    uint32_t imageIndex;
    vkWaitForFences(vulkanInstance.logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    
    VkResult result = vkAcquireNextImageKHR(vulkanInstance.logicalDevice, vulkanSwapchain.swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("[Vulkan] Failed to acquire Swapchain Image");
    }

    vkResetFences(vulkanInstance.logicalDevice, 1, &inFlightFences[currentFrame]);
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
    updateUniformBuffer(currentFrame);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    result = vkQueueSubmit(vulkanInstance.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to submit Draw CommandBuffer");
    }

    VkPresentInfoKHR presentInfo{};
    VkSwapchainKHR swapChains[] = { vulkanSwapchain.swapChain };
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;  // Optional

    result = vkQueuePresentKHR(vulkanInstance.graphicsQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.getFramebufferResized()) {
        logger.INFO(" Recreating Swapchain, VkResult: {0}, FramebufferResized: {1}", string_VkResult(result), window.getFramebufferResized());
        window.setFramebufferResized(false);
        recreateSwapchain();
    }
    else if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to present Swapchain Image");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Swiftcanon::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(24.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(32.0f, 32.0f, 12.0f), glm::vec3(0.0f, 0.0f, 8.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), vulkanSwapchain.getSwapchainExtent().width / (float) vulkanSwapchain.getSwapchainExtent().height, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1;
    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Swiftcanon::cleanup()
{
    cleanupDepthResources();
for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(vulkanInstance.logicalDevice, uniformBuffers[i], nullptr);
    vkFreeMemory(vulkanInstance.logicalDevice, uniformBuffersMemory[i], nullptr);
}
    vkDestroyDescriptorPool(vulkanInstance.logicalDevice, descriptorPool, nullptr);
    vkDestroyBuffer(vulkanInstance.logicalDevice, indexBuffer, nullptr);
    vkFreeMemory(vulkanInstance.logicalDevice, indexBufferMemory, nullptr);
    vkDestroyBuffer(vulkanInstance.logicalDevice, vertexBuffer, nullptr);
    vkFreeMemory(vulkanInstance.logicalDevice, vertexBufferMemory, nullptr);
for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(vulkanInstance.logicalDevice, imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(vulkanInstance.logicalDevice, renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(vulkanInstance.logicalDevice, inFlightFences[i], nullptr);
}
    vkDestroyCommandPool(vulkanInstance.logicalDevice, commandPool, nullptr);
}
