#include "CommandDispatcher.h"

int CommandDispatcher::id;

CommandDispatcher::CommandDispatcher(VulkanInstance* vkInstance, Swapchain* swapchain, GraphicsPipeline* graphicsPipeline)
    :vkInstance(vkInstance),
    swapchain(swapchain),
    graphicsPipeline(graphicsPipeline)
{
    id += 1;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags              = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex   = vkInstance->physicalDeviceIndices.graphicsFamily.value();

    VkResult result = vkCreateCommandPool(vkInstance->logicalDevice, &poolInfo, nullptr, &commandPool);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create CommandPool");
    }

    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType                 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool           = commandPool;
    allocInfo.level                 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount    = static_cast<uint32_t>(commandBuffers.size());

    result = vkAllocateCommandBuffers(vkInstance->logicalDevice, &allocInfo, commandBuffers.data());
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to create CommandBuffer");
    }

    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkResult result = vkCreateSemaphore(vkInstance->logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
        if (result != VK_SUCCESS) {
            std::cerr << string_VkResult(result) << std::endl;
            throw std::runtime_error("[VULKAN] Failed to create Semaphore");
        }
        result = vkCreateSemaphore(vkInstance->logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
        if (result != VK_SUCCESS) {
            std::cerr << string_VkResult(result) << std::endl;
            throw std::runtime_error("[VULKAN] Failed to create Semaphore");
        }
        result = vkCreateFence(vkInstance->logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]);
        if (result != VK_SUCCESS) {
            std::cerr << string_VkResult(result) << std::endl;
            throw std::runtime_error("[VULKAN] Failed to create Fence");
        }
    }
}

CommandDispatcher::~CommandDispatcher()
{
logger.INFO("(MEMORY) Cleaning up Command Dispatcher");
for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(vkInstance->logicalDevice, imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(vkInstance->logicalDevice, renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(vkInstance->logicalDevice, inFlightFences[i], nullptr);
}
    logger.TRACE("  Destroying commandPool...");
    vkDestroyCommandPool(vkInstance->logicalDevice, commandPool, nullptr);
}

void CommandDispatcher::dispatchCommand()
{
    uint32_t imageIndex;
    vkWaitForFences(vkInstance->logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(vkInstance->logicalDevice, 1, &inFlightFences[currentFrame]);
    
    VkResult result = vkAcquireNextImageKHR(vkInstance->logicalDevice, swapchain->swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    // if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    //     // recreateSwapchain();
    //     return;
    // }
    // else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    //     throw std::runtime_error("[Vulkan] Failed to acquire Swapchain Image");
    // }

    // vkResetFences(vkInstance->logicalDevice, 1, &inFlightFences[currentFrame]);
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
    // updateUniformBuffer(currentFrame);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[]        = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[]   = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount       = 1;
    submitInfo.pWaitSemaphores          = waitSemaphores;
    submitInfo.pWaitDstStageMask        = waitStages;
    submitInfo.commandBufferCount       = 1;
    submitInfo.pCommandBuffers          = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[]      = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount     = 1;
    submitInfo.pSignalSemaphores        = signalSemaphores;

    result = vkQueueSubmit(vkInstance->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to submit Draw CommandBuffer");
    }

    VkPresentInfoKHR presentInfo{};
    VkSwapchainKHR swapChains[]         = { swapchain->swapChain };
    presentInfo.sType                   = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount      = 1;
    presentInfo.pWaitSemaphores         = signalSemaphores;
    presentInfo.swapchainCount          = 1;
    presentInfo.pSwapchains             = swapChains;
    presentInfo.pImageIndices           = &imageIndex;
    presentInfo.pResults                = nullptr;  // Optional

    result = vkQueuePresentKHR(vkInstance->graphicsQueue, &presentInfo);
    // if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vkInstance->window->getFramebufferResized()) {
    //     logger.INFO(" Recreating Swapchain, VkResult: {0}, FramebufferResized: {1}", string_VkResult(result), vkInstance->window->getFramebufferResized());
    //     vkInstance->window->setFramebufferResized(false);
    //     // recreateSwapchain();
    // }
    // else if (result != VK_SUCCESS) {
    //     std::cerr << string_VkResult(result) << std::endl;
    //     throw std::runtime_error("[VULKAN] Failed to present Swapchain Image");
    // }
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to present Swapchain Image");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void CommandDispatcher::recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType             = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags             = 0;        // Optional
    beginInfo.pInheritanceInfo  = nullptr;  // Optional

    VkRenderPassBeginInfo renderPassInfo{};
    // std::array<VkClearValue, 2> clearValues{};
    std::array<VkClearValue, 1> clearValues{};
        clearValues[0].color            = {{0.0f, 0.0f, 0.0f, 1.0f}};
        // clearValues[1].depthStencil     = {1.0f, 0};
    renderPassInfo.sType                = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass           = graphicsPipeline->getRenderPass();
    renderPassInfo.framebuffer          = swapchain->swapChainFramebuffers[image_index];
    renderPassInfo.renderArea.offset    = {0, 0};
    renderPassInfo.renderArea.extent    = swapchain->swapChainExtent;
    renderPassInfo.clearValueCount      = static_cast<uint32_t>(clearValues.size());;
    renderPassInfo.pClearValues         = clearValues.data();

    VkViewport viewport{};
    viewport.x          = 0.0f;
    viewport.y          = 0.0f;
    viewport.width      = static_cast<float>(swapchain->swapChainExtent.width);
    viewport.height     = static_cast<float>(swapchain->swapChainExtent.height);
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;

    VkRect2D scissor{};
    scissor.offset      = {0, 0};
    scissor.extent      = swapchain->getSwapchainExtent();

    VkResult result = vkBeginCommandBuffer(command_buffer, &beginInfo);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to initialize recording CommandBuffer");
    }

    // VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = {0};
    vkCmdBeginRenderPass        (command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline           (command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->getGraphicsPipeline());
    // vkCmdBindVertexBuffers      (command_buffer, 0, 1, vertexBuffers, offsets);
    // vkCmdBindIndexBuffer        (command_buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    // vkCmdBindDescriptorSets     (command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
    vkCmdSetViewport            (command_buffer, 0, 1, &viewport);
    vkCmdSetScissor             (command_buffer, 0, 1, &scissor);
    // vkCmdDrawIndexed            (command_buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    vkCmdDraw                   (command_buffer, 3, 1, 0, 0);
    vkCmdEndRenderPass          (command_buffer);
    result = vkEndCommandBuffer (command_buffer);
    if (result != VK_SUCCESS) {
        std::cerr << string_VkResult(result) << std::endl;
        throw std::runtime_error("[VULKAN] Failed to record CommandBuffer");
    }
}
