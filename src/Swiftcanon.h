#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>

struct DeviceDetails {
    const char* name;
    int deviceIndex;
    int score;
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
    VkPhysicalDevice                physicalDevice = VK_NULL_HANDLE;
    std::vector<QueueFamilyIndices> indices;
    std::vector<const char*>        requiredDeviceExtensions;
    VkDevice                        device;
    VkQueue                         graphicsQueue;

    // Vulkan Presentation Setup
    void createSurface();

    // Vulkan Presentation Setup
    GLFWwindow*                     window;
    VkSurfaceKHR                    surface;
    VkQueue                         presentQueue;

    // TODO: not best way to to this, should have like a global debug setup
    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif
};