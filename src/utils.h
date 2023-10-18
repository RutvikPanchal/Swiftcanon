#pragma once

#include "pch.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <tiny_obj_loader.h>

namespace utils
{
    // Basic Structs

    // Vulkan Structs
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        
        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding      = 0;
            bindingDescription.stride       = sizeof(Vertex);
            bindingDescription.inputRate    = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }
        
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
            attributeDescriptions[0].binding    = 0;
            attributeDescriptions[0].location   = 0;
            attributeDescriptions[0].format     = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset     = offsetof(Vertex, pos);
            attributeDescriptions[1].binding    = 0;
            attributeDescriptions[1].location   = 1;
            attributeDescriptions[1].format     = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset     = offsetof(Vertex, color);
            return attributeDescriptions;
        }
    };

    // Basic Utils
    std::vector<char>   readFile(const std::string& filename);

    // Vulkan Utils
    void                loadModel(const char* path);
    VkFormat            findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
}