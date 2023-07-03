#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

struct DeviceDetails {
    const char* name;
    int deviceIndex;
    int score;
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
    void addVulkanExtensions();
    void createVulkanInstance();
    void pickPhysicalGraphicsDevice();
    
    void ratePhysicalGraphicsDevices(VkPhysicalDevice device, int deviceIndex);

    GLFWwindow*                     window;
    std::vector<const char*>        requiredExtensions;
    std::vector<const char*> const  requiredValidationLayers;
    VkInstance                      vkInstance;
    VkPhysicalDevice                physicalDevice = VK_NULL_HANDLE;
    std::vector<DeviceDetails>      allDeviceDetails;

    // TODO: not best way to to this, should have like a global debug setup
    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif
};