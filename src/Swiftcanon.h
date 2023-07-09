#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>

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
    void createGraphicsPipeline();
    void createCommandPool();
    VkShaderModule createShaderModule(const std::vector<char>& code);

    // Vulkan Pipeline Setup
    VkRenderPass                    renderPass;
    VkPipelineLayout                pipelineLayout;
    VkPipeline                      graphicsPipeline;
    VkCommandPool                   commandPool;

    // UTIL
    std::vector<char> readFile(const std::string& filename);

    // TODO: not best way to to this, should have like a global debug setup
    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif
};