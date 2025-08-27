#include "RayTracingPipeline.h"
#include "VulkanHelpers.h" 
#include "Vertex.h"
#include <stdexcept>
#include <iostream>
#include <cstring>

RayTracingPipeline::RayTracingPipeline(VkDevice device, VkPhysicalDevice physicalDevice, 
                                       VkCommandPool commandPool, VkQueue graphicsQueue)
    : device(device), physicalDevice(physicalDevice), commandPool(commandPool), graphicsQueue(graphicsQueue), 
      pipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE),
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
    
    std::cout << "Step 1: Creating BLAS (Bottom Level Acceleration Structure)" << std::endl;
    
    // BLAS geometry setup
    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.geometry.triangles.vertexData.deviceAddress = getBufferDeviceAddress(vertexBuffer);
    geometry.geometry.triangles.vertexStride = sizeof(Vertex);
    geometry.geometry.triangles.maxVertex = vertexCount - 1;
    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    geometry.geometry.triangles.indexData.deviceAddress = getBufferDeviceAddress(indexBuffer);
    
    // Build info
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometry;
    
    uint32_t primitiveCount = indexCount / 3;
    VkAccelerationStructureBuildSizesInfoKHR blasSizeInfo{};
    blasSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, 
                                           &buildInfo, &primitiveCount, &blasSizeInfo);
    
    std::cout << "✅ BLAS size calculated: " << blasSizeInfo.accelerationStructureSize << " bytes" << std::endl;
    std::cout << "   - Vertices: " << vertexCount << ", Triangles: " << primitiveCount << std::endl;
    
    // Step 2: Create BLAS buffer and memory
    std::cout << "Step 2: Creating BLAS buffer and memory allocation" << std::endl;
    
    VulkanHelpers::createBuffer(device, physicalDevice, 
                               blasSizeInfo.accelerationStructureSize,
                               VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               bottomLevelASBuffer, bottomLevelASMemory);
    
    std::cout << "✅ BLAS buffer created: " << blasSizeInfo.accelerationStructureSize << " bytes allocated" << std::endl;
    
    // Step 3: Create actual acceleration structure
    std::cout << "Step 3: Creating BLAS acceleration structure" << std::endl;
    
    VkAccelerationStructureCreateInfoKHR asCreateInfo{};
    asCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    asCreateInfo.buffer = bottomLevelASBuffer;
    asCreateInfo.size = blasSizeInfo.accelerationStructureSize;
    asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    
    VkResult result = vkCreateAccelerationStructureKHR(device, &asCreateInfo, nullptr, &bottomLevelAS);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create bottom level acceleration structure");
    }
    
    std::cout << "✅ BLAS acceleration structure created successfully" << std::endl;
    
    // Step 4: Build the BLAS with command buffer (complex step)
    std::cout << "Step 4: Building BLAS with command buffer" << std::endl;
    
    // Need scratch buffer for building
    VkBuffer scratchBuffer;
    VkDeviceMemory scratchMemory;
    VulkanHelpers::createBuffer(device, physicalDevice,
                               blasSizeInfo.buildScratchSize,
                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               scratchBuffer, scratchMemory);
    
    // Update build info with destination and scratch addresses
    buildInfo.dstAccelerationStructure = bottomLevelAS;
    buildInfo.scratchData.deviceAddress = getBufferDeviceAddress(scratchBuffer);
    
    // Build range info
    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.primitiveCount = primitiveCount;
    buildRangeInfo.primitiveOffset = 0;
    buildRangeInfo.firstVertex = 0;
    buildRangeInfo.transformOffset = 0;
    
    const VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfo = &buildRangeInfo;
    
    // Real command buffer building
    std::cout << "   - Scratch buffer created: " << blasSizeInfo.buildScratchSize << " bytes" << std::endl;
    std::cout << "   - Build info configured with addresses" << std::endl;
    
    VkCommandBuffer buildCommandBuffer = beginSingleTimeCommands();
    
    std::cout << "   - Executing vkCmdBuildAccelerationStructuresKHR..." << std::endl;
    vkCmdBuildAccelerationStructuresKHR(buildCommandBuffer, 1, &buildInfo, &pBuildRangeInfo);
    
    endSingleTimeCommands(buildCommandBuffer);
    std::cout << "   - Command buffer executed and submitted to GPU" << std::endl;
    
    // Cleanup scratch buffer
    vkDestroyBuffer(device, scratchBuffer, nullptr);
    vkFreeMemory(device, scratchMemory, nullptr);
    
    std::cout << "✅ BLAS built successfully with " << primitiveCount << " triangles" << std::endl;
    std::cout << "Acceleration structures setup (step 4: BLAS fully built!)" << std::endl;
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

VkCommandBuffer RayTracingPipeline::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void RayTracingPipeline::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

VkShaderModule RayTracingPipeline::createShaderModule(const std::vector<char>& code) {
    return VulkanHelpers::createShaderModule(device, code);
}

std::vector<char> RayTracingPipeline::readFile(const std::string& filename) {
    return VulkanHelpers::readFile(filename);
}