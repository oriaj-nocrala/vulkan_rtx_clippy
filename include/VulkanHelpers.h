#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <optional>
#include <string>

// Enhanced Uniform Buffer Object with all advanced features
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 viewInverse;
    alignas(16) glm::mat4 projInverse;
    alignas(16) glm::vec3 cameraPos;
    alignas(4) float time;
    alignas(4) float metallic;
    alignas(4) float roughness;
    alignas(4) int rtxEnabled;
    alignas(8) glm::vec2 mousePos;
    alignas(8) glm::vec2 resolution;
    alignas(4) float glowIntensity;
    alignas(4) int frameCount;
    alignas(4) int maxBounces;
    alignas(4) int samplesPerPixel;
    alignas(4) int isBGRFormat; // 1 if BGR format, 0 if RGB
    alignas(4) float volumetricDensity;   // Fog/atmosphere density
    alignas(4) float volumetricScattering; // Light scattering strength
    alignas(4) float glassRefractionIndex; // Glass IOR (1.0 = air, 1.5 = glass)
    alignas(4) float causticsStrength;     // Caustics effect intensity
    alignas(4) float subsurfaceScattering; // SSS strength (0.0 = none, 1.0 = full)
    alignas(4) float subsurfaceRadius;     // SSS penetration distance
};

// Material PBR para Clippy
struct Material {
    glm::vec3 albedo = glm::vec3(1.0f, 0.843f, 0.0f); // Color dorado
    float metallic = 0.95f;
    float roughness = 0.05f;
    float ao = 1.0f;
    glm::vec3 emissive = glm::vec3(0.2f, 0.15f, 0.0f);
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanHelpers {
public:
    static std::vector<char> readFile(const std::string& filename);
    static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
    
    static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
    static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    
    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    static void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, 
                            VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                            VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    
    static void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                          VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);
    static VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, 
                                       VkImageTiling tiling, VkFormatFeatureFlags features);
    
    static void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, 
                           uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, 
                           VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
                           VkImage& image, VkDeviceMemory& imageMemory);
    
    static VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, 
                                      VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    
    static VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice);
    
    static bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions);
    static bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions);
    
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData);
};