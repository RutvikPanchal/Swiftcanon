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
    bool isComplete() {
        return graphicsFamily.has_value();
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

    void addVulkanValidationLayers();
    void addVulkanInstanceExtensions();
    void createVulkanInstance();
    void pickPhysicalGraphicsDevice();
    void createVulkanLogicalDevice();
    
    void ratePhysicalGraphicsDevices(VkPhysicalDevice device, int deviceIndex);

    GLFWwindow*                     window;
    std::vector<const char*> const  requiredValidationLayers;
    std::vector<const char*>        requiredVkInstanceExtensions;
    VkInstance                      vkInstance;
    std::vector<DeviceDetails>      allDeviceDetails;
    VkPhysicalDevice                physicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices              indices;
    std::vector<const char*>        requiredDeviceInstanceExtensions;
    VkDevice                        device;
    VkQueue                         graphicsQueue;

    // TODO: not best way to to this, should have like a global debug setup
    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif
};