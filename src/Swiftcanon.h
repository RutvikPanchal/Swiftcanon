#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <vector>
#include <string>
#include <optional>

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct Vertex {
    glm::vec2 pos;
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
        attributeDescriptions[0].format     = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset     = offsetof(Vertex, pos);
        attributeDescriptions[1].binding    = 0;
        attributeDescriptions[1].location   = 1;
        attributeDescriptions[1].format     = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset     = offsetof(Vertex, color);
        return attributeDescriptions;
    }
};

struct DeviceDetails {
    const char* name;
    int         deviceIndex;
    int         score;
    uint32_t    extensionCount;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class Swiftcanon
{
public:
    Swiftcanon();
    void run();
    void init();

    // TODO: This doesn't seem like a good implementation
    bool framebufferResized = false;

private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    // Vulkan Compute Setup
    void addVulkanValidationLayers();
    void addVulkanInstanceExtensions();
    void createVulkanInstance();
    void pickPhysicalGraphicsDevice();
    void createVulkanLogicalDevice();
    void ratePhysicalGraphicsDevices(VkPhysicalDevice device, int deviceIndex);

    // Vulkan Compute Setup
    std::vector<const char*> const  requiredValidationLayers;
    std::vector<const char*>        requiredVulkanExtensions;
    VkInstance                      vkInstance;
    std::vector<DeviceDetails>      allDeviceDetails;
    std::vector<QueueFamilyIndices> allDeviceIndices;
    DeviceDetails                   physicalDeviceDetails;
    QueueFamilyIndices              physicalDeviceIndices;
    VkPhysicalDevice                physicalDevice = VK_NULL_HANDLE;
    std::vector<const char*>        requiredDeviceExtensions;
    VkDevice                        device;
    VkQueue                         graphicsQueue;

    // Vulkan Presentation Setup
    void createSurface();
    void createSwapChain();
    void recreateSwapChain();
    void cleanupSwapChain();
    void createImageViews();
    void createFramebuffers();

    // Vulkan Presentation Setup
    GLFWwindow*                     window;
    VkSurfaceKHR                    surface;
    VkQueue                         presentQueue;
    VkSwapchainKHR                  swapChain;
    std::vector<VkImage>            swapChainImages;
    VkFormat                        swapChainImageFormat;
    VkExtent2D                      swapChainExtent;
    std::vector<VkImageView>        swapChainImageViews;
    std::vector<VkFramebuffer>      swapChainFramebuffers;

    // Vulkan Pipeline Setup
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createCommandPool();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffer();
    void createSyncObjects();
    void drawFrame();
    void updateUniformBuffer(uint32_t currentImage);
    void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index);
    VkShaderModule createShaderModule(const std::vector<char>& code);

    // Vulkan Pipeline Setup
    VkRenderPass                    renderPass;
    VkDescriptorSetLayout           descriptorSetLayout;
    VkPipelineLayout                pipelineLayout;
    VkPipeline                      graphicsPipeline;
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
    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };
    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };
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

    // UTIL
    std::vector<char> readFile(const std::string& filename);

    // TODO: not best way to to this, should have like a global debug setup
    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif
};