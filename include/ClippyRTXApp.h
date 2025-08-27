#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <memory>

#include "Vertex.h"
#include "VulkanHelpers.h"
#include "ClippyGeometry.h"
#include "RayTracingPipeline.h"
#include "ClippyUI.h"
#include "PostProcessing.h"

const uint32_t WIDTH = 1920;
const uint32_t HEIGHT = 1080;
const int MAX_FRAMES_IN_FLIGHT = 2;

class ClippyRTXApp {
public:
    void run();

private:
    GLFWwindow* window;
    
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    
    VkCommandPool commandPool;
    
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;
    
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    
    std::vector<VkCommandBuffer> commandBuffers;
    
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
    
    bool framebufferResized = false;
    bool rtxEnabled = true;
    
    // Ray Tracing
    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;
    
    // UI System
    std::unique_ptr<ClippyUI> clippyUI;
    
    // Post Processing
    std::unique_ptr<PostProcessing> postProcessing;
    
    // Material del Clippy
    Material clippyMaterial;
    
    // Timing
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    float totalTime = 0.0f;
    
    // Advanced RTX parameters
    uint32_t frameCount = 0;
    int maxBounces = 3;
    int samplesPerPixel = 4;
    
    // Mouse interaction
    double mouseX = 0.0, mouseY = 0.0;
    bool mousePressed = false;
    
    // Animation modes
    enum class AnimationMode {
        IDLE,
        EXCITED,
        HELPING,
        THINKING,
        QUANTUM,
        PARTY,
        MATRIX
    } currentAnimationMode = AnimationMode::IDLE;

    // Initialization
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
    
    // Window callbacks
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    
    // Animation functions
    void updateAnimationMode();
    void handleMouseInteraction();
    
    // Vulkan setup
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createCommandPool();
    void createColorResources();
    void createDepthResources();
    void createFramebuffers();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();
    
    // Geometry
    void createClippyGeometry();
    
    // Ray Tracing
    bool checkRayTracingSupport();
    void setupRayTracing();
    
    // UI System
    void setupUI();
    void updateUI();
    void renderUI(VkCommandBuffer commandBuffer);
    
    // Post Processing
    void setupPostProcessing();
    void updatePostProcessing();
    
    // Rendering
    void drawFrame();
    void updateUniformBuffer(uint32_t currentImage);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    
    // Swapchain recreation
    void recreateSwapChain();
    void cleanupSwapChain();
    
    // Validation layers
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
};