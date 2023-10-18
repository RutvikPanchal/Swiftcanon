#pragma once
#include "pch.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Window.h"
#include "VulkanInstance.h"
#include "CommandDispatcher.h"
#include "Swapchain.h"
#include "GraphicsPipeline.h"
#include "Logger.h"

#include "utils.h"

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
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

class Swiftcanon
{
public:
    Swiftcanon();
    void run();
    void init();

private:
    Window              window;
    VulkanInstance      vulkanInstance          = VulkanInstance(&window);
    Swapchain           vulkanSwapchain         = Swapchain(&vulkanInstance);
    GraphicsPipeline    vulkanGraphicsPipeline  = GraphicsPipeline(&vulkanInstance, &vulkanSwapchain);
    // CommandDispatcher   vulkanCommandDispatcher = CommandDispatcher(&vulkanInstance, &vulkanSwapchain, &vulkanGraphicsPipeline);

    Logger          logger          = Logger("SWIFTCANON");

private:
    void initVulkan();
    void mainLoop();
    void cleanup();

    // Vulkan Presentation Setup
    void recreateSwapchain();
    void cleanupDepthResources();

    // Vulkan Pipeline Setup
    void createCommandPool();
    void createDepthResources();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffer();
    void createSyncObjects();
    void drawFrame();
    void updateUniformBuffer(uint32_t currentImage);
    void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index);
    VkShaderModule createShaderModule(const std::vector<char>& code);

    // Vulkan Pipeline Setup
    VkImage                         depthImage;
    VkDeviceMemory                  depthImageMemory;
    VkImageView                     depthImageView;
    VkCommandPool                   commandPool;
    VkDescriptorPool                descriptorPool;
    std::vector<VkDescriptorSet>    descriptorSets;
    std::vector<VkCommandBuffer>    commandBuffers;
    std::vector<VkSemaphore>        imageAvailableSemaphores;
    std::vector<VkSemaphore>        renderFinishedSemaphores;
    std::vector<VkFence>            inFlightFences;
    const int                       MAX_FRAMES_IN_FLIGHT        = 2;
    uint32_t                        currentFrame                = 0;

    // Shaders Setup
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer                        vertexBuffer;
    VkDeviceMemory                  vertexBufferMemory;
    VkBuffer                        indexBuffer;
    VkDeviceMemory                  indexBufferMemory;
    std::vector<VkBuffer>           uniformBuffers;
    std::vector<VkDeviceMemory>     uniformBuffersMemory;
    std::vector<void*>              uniformBuffersMapped;

    // Shaders Setup
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();

    // Vulkan Helper Functions
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    // UTIL
    std::vector<char> readFile(const std::string& filename);
    void loadModel(const char* path);
};
