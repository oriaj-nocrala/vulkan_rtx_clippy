#include "RayTracingPipeline.h"
#include "VulkanHelpers.h" 
#include "Vertex.h"
#include <stdexcept>
#include <iostream>
#include <cstring>

RayTracingPipeline::RayTracingPipeline(VkDevice device, VkPhysicalDevice physicalDevice)
    : device(device), physicalDevice(physicalDevice), pipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE),
      bottomLevelAS(VK_NULL_HANDLE), topLevelAS(VK_NULL_HANDLE),
      bottomLevelASBuffer(VK_NULL_HANDLE), bottomLevelASMemory(VK_NULL_HANDLE),
      topLevelASBuffer(VK_NULL_HANDLE), topLevelASMemory(VK_NULL_HANDLE),
      shaderBindingTableBuffer(VK_NULL_HANDLE), shaderBindingTableMemory(VK_NULL_HANDLE) {
    loadRayTracingFunctions();
}

RayTracingPipeline::~RayTracingPipeline() {
    if (device != VK_NULL_HANDLE) {
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
        if (bottomLevelAS != VK_NULL_HANDLE) {
            vkDestroyAccelerationStructureKHR(device, bottomLevelAS, nullptr);
        }
        if (topLevelAS != VK_NULL_HANDLE) {
            vkDestroyAccelerationStructureKHR(device, topLevelAS, nullptr);
        }
        if (bottomLevelASBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, bottomLevelASBuffer, nullptr);
        }
        if (bottomLevelASMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, bottomLevelASMemory, nullptr);
        }
        if (topLevelASBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, topLevelASBuffer, nullptr);
        }
        if (topLevelASMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, topLevelASMemory, nullptr);
        }
        if (shaderBindingTableBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, shaderBindingTableBuffer, nullptr);
        }
        if (shaderBindingTableMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, shaderBindingTableMemory, nullptr);
        }
    }
}

void RayTracingPipeline::loadRayTracingFunctions() {
    vkGetAccelerationStructureBuildSizesKHR = 
        reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
            vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
    
    vkCreateAccelerationStructureKHR = 
        reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
            vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
    
    vkCmdBuildAccelerationStructuresKHR = 
        reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
            vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
    
    vkCmdTraceRaysKHR = 
        reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
            vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
    
    vkGetRayTracingShaderGroupHandlesKHR = 
        reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
            vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
    
    vkCreateRayTracingPipelinesKHR = 
        reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
            vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
    
    vkDestroyAccelerationStructureKHR = 
        reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
            vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
    
    vkGetAccelerationStructureDeviceAddressKHR = 
        reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
            vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
    
    if (!vkGetAccelerationStructureBuildSizesKHR || !vkCreateAccelerationStructureKHR ||
        !vkCmdBuildAccelerationStructuresKHR || !vkCmdTraceRaysKHR ||
        !vkGetRayTracingShaderGroupHandlesKHR || !vkCreateRayTracingPipelinesKHR ||
        !vkDestroyAccelerationStructureKHR || !vkGetAccelerationStructureDeviceAddressKHR) {
        throw std::runtime_error("Failed to load ray tracing function pointers!");
    }
}

VkDeviceAddress RayTracingPipeline::getBufferDeviceAddress(VkBuffer buffer) {
    VkBufferDeviceAddressInfo addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.buffer = buffer;
    return vkGetBufferDeviceAddress(device, &addressInfo);
}

void RayTracingPipeline::createPipeline(VkDescriptorSetLayout descriptorSetLayout) {
    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create ray tracing pipeline layout!");
    }
    
    // Load shaders
    auto raygenShaderCode = readFile("shaders/raygen.rgen.spv");
    auto missShaderCode = readFile("shaders/miss.rmiss.spv");
    auto closestHitShaderCode = readFile("shaders/closesthit.rchit.spv");
    auto shadowMissShaderCode = readFile("shaders/shadow.rmiss.spv");
    
    VkShaderModule raygenShaderModule = createShaderModule(raygenShaderCode);
    VkShaderModule missShaderModule = createShaderModule(missShaderCode);
    VkShaderModule closestHitShaderModule = createShaderModule(closestHitShaderCode);
    VkShaderModule shadowMissShaderModule = createShaderModule(shadowMissShaderCode);
    
    // Shader stages
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    
    // Raygen
    VkPipelineShaderStageCreateInfo raygenStage{};
    raygenStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    raygenStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    raygenStage.module = raygenShaderModule;
    raygenStage.pName = "main";
    shaderStages.push_back(raygenStage);
    
    // Miss
    VkPipelineShaderStageCreateInfo missStage{};
    missStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    missStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    missStage.module = missShaderModule;
    missStage.pName = "main";
    shaderStages.push_back(missStage);
    
    // Shadow miss
    VkPipelineShaderStageCreateInfo shadowMissStage{};
    shadowMissStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shadowMissStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    shadowMissStage.module = shadowMissShaderModule;
    shadowMissStage.pName = "main";
    shaderStages.push_back(shadowMissStage);
    
    // Closest hit
    VkPipelineShaderStageCreateInfo closestHitStage{};
    closestHitStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    closestHitStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    closestHitStage.module = closestHitShaderModule;
    closestHitStage.pName = "main";
    shaderStages.push_back(closestHitStage);
    
    // Shader groups
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
    
    // Raygen group (index 0)
    VkRayTracingShaderGroupCreateInfoKHR raygenGroup{};
    raygenGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    raygenGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    raygenGroup.generalShader = 0;
    raygenGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    raygenGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    raygenGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroups.push_back(raygenGroup);
    
    // Miss group (index 1)
    VkRayTracingShaderGroupCreateInfoKHR missGroup{};
    missGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    missGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    missGroup.generalShader = 1;
    missGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    missGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    missGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroups.push_back(missGroup);
    
    // Shadow miss group (index 2)
    VkRayTracingShaderGroupCreateInfoKHR shadowMissGroup{};
    shadowMissGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shadowMissGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shadowMissGroup.generalShader = 2;
    shadowMissGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    shadowMissGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    shadowMissGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroups.push_back(shadowMissGroup);
    
    // Hit group (index 3)
    VkRayTracingShaderGroupCreateInfoKHR hitGroup{};
    hitGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    hitGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    hitGroup.generalShader = VK_SHADER_UNUSED_KHR;
    hitGroup.closestHitShader = 3;
    hitGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    hitGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroups.push_back(hitGroup);
    
    // Create ray tracing pipeline
    VkRayTracingPipelineCreateInfoKHR rtPipelineInfo{};
    rtPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    rtPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    rtPipelineInfo.pStages = shaderStages.data();
    rtPipelineInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
    rtPipelineInfo.pGroups = shaderGroups.data();
    rtPipelineInfo.maxPipelineRayRecursionDepth = 2;
    rtPipelineInfo.layout = pipelineLayout;
    
    if (vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, 
                                       &rtPipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create ray tracing pipeline!");
    }
    
    // Cleanup shader modules
    vkDestroyShaderModule(device, raygenShaderModule, nullptr);
    vkDestroyShaderModule(device, missShaderModule, nullptr);
    vkDestroyShaderModule(device, closestHitShaderModule, nullptr);
    vkDestroyShaderModule(device, shadowMissShaderModule, nullptr);
    
    std::cout << "Ray Tracing Pipeline created successfully" << std::endl;
}

void RayTracingPipeline::createAccelerationStructures(VkBuffer vertexBuffer, VkBuffer indexBuffer,
                                                      uint32_t vertexCount, uint32_t indexCount) {
    // Simple placeholder for now
    std::cout << "Acceleration structures setup (simple placeholder)" << std::endl;
}

void RayTracingPipeline::createShaderBindingTable() {
    // Simple placeholder for now
    raygenRegion = {};
    missRegion = {};  
    hitRegion = {};
    callableRegion = {};
    
    std::cout << "Shader Binding Table created (simple placeholder)" << std::endl;
}

void RayTracingPipeline::traceRays(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height) {
    // Safe placeholder
    std::cout << "RTX Ray Tracing executed (safe placeholder)" << std::endl;
}

VkShaderModule RayTracingPipeline::createShaderModule(const std::vector<char>& code) {
    return VulkanHelpers::createShaderModule(device, code);
}

std::vector<char> RayTracingPipeline::readFile(const std::string& filename) {
    return VulkanHelpers::readFile(filename);
}