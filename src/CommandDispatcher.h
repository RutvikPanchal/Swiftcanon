#pragma once
#include "pch.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "VulkanInstance.h"
#include "Swapchain.h"
#include "GraphicsPipeline.h"
#include "Logger.h"

class CommandDispatcher
{
public:
    CommandDispatcher(VulkanInstance* vkInstance, Swapchain* swapchain, GraphicsPipeline* graphicsPipeline);
   ~CommandDispatcher();

    void dispatchCommand();

private:
    void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index);

private:
    static int                      id;
    VkCommandPool                   commandPool;
    std::vector<VkCommandBuffer>    commandBuffers;
    std::vector<VkSemaphore>        imageAvailableSemaphores;
    std::vector<VkSemaphore>        renderFinishedSemaphores;
    std::vector<VkFence>            inFlightFences;
    const int                       MAX_FRAMES_IN_FLIGHT    = 2;
    uint32_t                        currentFrame            = 0;

private:
    VulkanInstance* vkInstance;
    Swapchain* swapchain;
    GraphicsPipeline* graphicsPipeline;

private:
    Logger logger = Logger((std::string("COMMAND DISPATCHER ").append(std::to_string(id))).c_str());
};