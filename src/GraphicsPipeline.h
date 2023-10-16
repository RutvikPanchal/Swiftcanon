#pragma once
#include "pch.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "VulkanInstance.h"
#include "Swapchain.h"
#include "Logger.h"
#include "utils.h"

class GraphicsPipeline
{
public:
    GraphicsPipeline(VulkanInstance* vkInstance, Swapchain* swapchain);
   ~GraphicsPipeline();

private:
    VkShaderModule createShaderModule(const std::vector<char>& code);

private:
    static int              id;
    VkRenderPass            renderPass;
    VkDescriptorSetLayout   descriptorSetLayout;
    VkPipelineLayout        pipelineLayout;
    VkPipeline              graphicsPipeline;

private:
    VulkanInstance* vkInstance;
    Swapchain*      swapchain;

private:
    Logger logger = Logger((std::string("GRAPHICS PIPELINE ").append(std::to_string(id))).c_str());
};