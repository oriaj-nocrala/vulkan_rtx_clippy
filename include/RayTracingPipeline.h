#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class RayTracingPipeline {
public:
    RayTracingPipeline(VkDevice device, VkPhysicalDevice physicalDevice, 
                       VkCommandPool commandPool, VkQueue graphicsQueue);
    ~RayTracingPipeline();
    
    void createPipeline(VkDescriptorSetLayout descriptorSetLayout);
    void createShaderBindingTable();
    void createAccelerationStructures(VkBuffer vertexBuffer, VkBuffer indexBuffer,
                                     uint32_t vertexCount, uint32_t indexCount);
    
    VkPipeline getPipeline() const { return pipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    
    VkAccelerationStructureKHR getTopLevelAS() const { return topLevelAS; }
    
    void traceRays(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height);
    
private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    
    // Acceleration structures
    VkAccelerationStructureKHR bottomLevelAS;
    VkAccelerationStructureKHR topLevelAS;
    VkBuffer bottomLevelASBuffer;
    VkDeviceMemory bottomLevelASMemory;
    VkBuffer topLevelASBuffer;
    VkDeviceMemory topLevelASMemory;
    
    // Shader binding table
    VkBuffer shaderBindingTableBuffer;
    VkDeviceMemory shaderBindingTableMemory;
    VkStridedDeviceAddressRegionKHR raygenRegion{};
    VkStridedDeviceAddressRegionKHR missRegion{};
    VkStridedDeviceAddressRegionKHR hitRegion{};
    VkStridedDeviceAddressRegionKHR callableRegion{};
    
    // Function pointers for ray tracing
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    
    void loadRayTracingFunctions();
    VkDeviceAddress getBufferDeviceAddress(VkBuffer buffer);
    
    // Command buffer helpers for building BLAS
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    
    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::vector<char> readFile(const std::string& filename);
};