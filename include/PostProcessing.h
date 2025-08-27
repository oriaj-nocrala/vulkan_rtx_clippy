#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

class PostProcessing {
public:
    struct PostProcessUniforms {
        float time;
        float exposure;
        float gamma;
        float contrast;
        float saturation;
        float vignetteStrength;
        float chromaticAberration;
        float filmGrain;
        float bloomIntensity;
        float bloomRadius;
        glm::vec2 resolution;
        int enableTonemap;
        int enableBloom;
        int enableVignette;
        int enableChromaticAberration;
        int enableFilmGrain;
    };

    PostProcessing(VkDevice device, VkPhysicalDevice physicalDevice, VkRenderPass renderPass, VkExtent2D extent);
    ~PostProcessing();

    void cleanup();
    void createPostProcessPipeline();
    void createPostProcessResources();
    void recreateResources(VkExtent2D newExtent);
    
    void render(VkCommandBuffer commandBuffer, VkImageView sourceImage, uint32_t currentFrame);
    void updateUniforms(uint32_t currentFrame, float time);
    
    // Effect controls
    void setExposure(float exposure) { uniforms.exposure = exposure; }
    void setGamma(float gamma) { uniforms.gamma = gamma; }
    void setContrast(float contrast) { uniforms.contrast = contrast; }
    void setSaturation(float saturation) { uniforms.saturation = saturation; }
    void setVignetteStrength(float strength) { uniforms.vignetteStrength = strength; }
    void setChromaticAberration(float amount) { uniforms.chromaticAberration = amount; }
    void setFilmGrain(float amount) { uniforms.filmGrain = amount; }
    void setBloomIntensity(float intensity) { uniforms.bloomIntensity = intensity; }
    void setBloomRadius(float radius) { uniforms.bloomRadius = radius; }
    
    void enableTonemap(bool enable) { uniforms.enableTonemap = enable ? 1 : 0; }
    void enableBloom(bool enable) { uniforms.enableBloom = enable ? 1 : 0; }
    void enableVignette(bool enable) { uniforms.enableVignette = enable ? 1 : 0; }
    void enableChromaticAberration(bool enable) { uniforms.enableChromaticAberration = enable ? 1 : 0; }
    void enableFilmGrain(bool enable) { uniforms.enableFilmGrain = enable ? 1 : 0; }

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkRenderPass renderPass;
    VkExtent2D extent;
    
    PostProcessUniforms uniforms;
    
    // Pipeline resources
    VkPipeline postProcessPipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    
    // Uniform buffers
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    
    // Post-process render targets
    VkImage postProcessImage;
    VkDeviceMemory postProcessImageMemory;
    VkImageView postProcessImageView;
    VkFramebuffer postProcessFramebuffer;
    
    // Bloom resources
    std::vector<VkImage> bloomImages;
    std::vector<VkDeviceMemory> bloomImageMemories;
    std::vector<VkImageView> bloomImageViews;
    std::vector<VkFramebuffer> bloomFramebuffers;
    
    // Samplers
    VkSampler colorSampler;
    VkSampler bloomSampler;
    
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();
    void createUniformBuffers();
    void createSamplers();
    void createBloomResources();
    void updateDescriptorSet(uint32_t currentFrame, VkImageView sourceImageView);
    
    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::vector<char> readFile(const std::string& filename);
};