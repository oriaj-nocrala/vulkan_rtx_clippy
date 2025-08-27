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

VkDeviceAddress RayTracingPipeline::getAccelerationStructureDeviceAddress(VkAccelerationStructureKHR as) {
    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addressInfo.accelerationStructure = as;
    return vkGetAccelerationStructureDeviceAddressKHR(device, &addressInfo);
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
    
    // === TOP LEVEL ACCELERATION STRUCTURE (TLAS) ===
    std::cout << "\nStep 5: Creating TLAS (Top Level Acceleration Structure)" << std::endl;
    
    // Step 5a: Create instance data for Clippy
    VkAccelerationStructureInstanceKHR instance{};
    
    // Identity transformation matrix (Clippy at origin)
    instance.transform.matrix[0][0] = 1.0f; instance.transform.matrix[0][1] = 0.0f; instance.transform.matrix[0][2] = 0.0f; instance.transform.matrix[0][3] = 0.0f;
    instance.transform.matrix[1][0] = 0.0f; instance.transform.matrix[1][1] = 1.0f; instance.transform.matrix[1][2] = 0.0f; instance.transform.matrix[1][3] = 0.0f;
    instance.transform.matrix[2][0] = 0.0f; instance.transform.matrix[2][1] = 0.0f; instance.transform.matrix[2][2] = 1.0f; instance.transform.matrix[2][3] = 0.0f;
    
    instance.instanceCustomIndex = 0;  // Custom index for shader access
    instance.mask = 0xFF;              // Visibility mask
    instance.instanceShaderBindingTableRecordOffset = 0; // Hit group offset
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instance.accelerationStructureReference = getAccelerationStructureDeviceAddress(bottomLevelAS);
    
    std::cout << "✅ TLAS instance created with identity transform" << std::endl;
    std::cout << "   - BLAS address: 0x" << std::hex << instance.accelerationStructureReference << std::dec << std::endl;
    
    // Step 5b: Create instance buffer and upload data
    std::cout << "Step 5b: Creating TLAS instance buffer" << std::endl;
    
    VkBuffer instanceBuffer;
    VkDeviceMemory instanceBufferMemory;
    VkDeviceSize instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR);
    
    VulkanHelpers::createBuffer(device, physicalDevice,
                               instanceBufferSize,
                               VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               instanceBuffer, instanceBufferMemory);
    
    // Upload instance data to buffer
    void* mappedData;
    vkMapMemory(device, instanceBufferMemory, 0, instanceBufferSize, 0, &mappedData);
    memcpy(mappedData, &instance, sizeof(VkAccelerationStructureInstanceKHR));
    vkUnmapMemory(device, instanceBufferMemory);
    
    VkDeviceAddress instanceBufferAddress = getBufferDeviceAddress(instanceBuffer);
    
    std::cout << "✅ TLAS instance buffer created and uploaded" << std::endl;
    std::cout << "   - Instance buffer size: " << instanceBufferSize << " bytes" << std::endl;
    std::cout << "   - Instance buffer address: 0x" << std::hex << instanceBufferAddress << std::dec << std::endl;
    
    // Step 5c: Create TLAS geometry and build info
    std::cout << "Step 5c: Creating TLAS geometry and build info" << std::endl;
    
    // TLAS geometry setup (different from BLAS - uses instances)
    VkAccelerationStructureGeometryKHR tlasGeometry{};
    tlasGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    
    tlasGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    tlasGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    tlasGeometry.geometry.instances.data.deviceAddress = instanceBufferAddress;
    
    // TLAS build info
    VkAccelerationStructureBuildGeometryInfoKHR tlasBuildInfo{};
    tlasBuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    tlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    tlasBuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    tlasBuildInfo.geometryCount = 1;
    tlasBuildInfo.pGeometries = &tlasGeometry;
    
    uint32_t instanceCount = 1; // One Clippy instance
    VkAccelerationStructureBuildSizesInfoKHR tlasSizeInfo{};
    tlasSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                           &tlasBuildInfo, &instanceCount, &tlasSizeInfo);
    
    std::cout << "✅ TLAS geometry and build info created" << std::endl;
    std::cout << "   - TLAS size: " << tlasSizeInfo.accelerationStructureSize << " bytes" << std::endl;
    std::cout << "   - TLAS scratch size: " << tlasSizeInfo.buildScratchSize << " bytes" << std::endl;
    std::cout << "   - Instance count: " << instanceCount << std::endl;
    
    // Step 5d: Build complete TLAS - FINAL STEP!
    std::cout << "Step 5d: Building complete TLAS with command buffer (FINAL STEP!)" << std::endl;
    
    // Create TLAS buffer
    VulkanHelpers::createBuffer(device, physicalDevice,
                               tlasSizeInfo.accelerationStructureSize,
                               VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               topLevelASBuffer, topLevelASMemory);
    
    // Create TLAS acceleration structure
    VkAccelerationStructureCreateInfoKHR tlasCreateInfo{};
    tlasCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    tlasCreateInfo.buffer = topLevelASBuffer;
    tlasCreateInfo.size = tlasSizeInfo.accelerationStructureSize;
    tlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    
    VkResult tlasResult = vkCreateAccelerationStructureKHR(device, &tlasCreateInfo, nullptr, &topLevelAS);
    if (tlasResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to create top level acceleration structure");
    }
    
    std::cout << "   ✅ TLAS buffer and acceleration structure created" << std::endl;
    
    // Create TLAS scratch buffer
    VkBuffer tlasScratchBuffer;
    VkDeviceMemory tlasScratchMemory;
    VulkanHelpers::createBuffer(device, physicalDevice,
                               tlasSizeInfo.buildScratchSize,
                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               tlasScratchBuffer, tlasScratchMemory);
    
    // Update TLAS build info with destination and scratch addresses
    tlasBuildInfo.dstAccelerationStructure = topLevelAS;
    tlasBuildInfo.scratchData.deviceAddress = getBufferDeviceAddress(tlasScratchBuffer);
    
    // TLAS build range info
    VkAccelerationStructureBuildRangeInfoKHR tlasBuildRangeInfo{};
    tlasBuildRangeInfo.primitiveCount = instanceCount;
    tlasBuildRangeInfo.primitiveOffset = 0;
    tlasBuildRangeInfo.firstVertex = 0;
    tlasBuildRangeInfo.transformOffset = 0;
    
    const VkAccelerationStructureBuildRangeInfoKHR* pTlasBuildRangeInfo = &tlasBuildRangeInfo;
    
    // Build TLAS with command buffer
    std::cout << "   - Executing TLAS build with command buffer..." << std::endl;
    VkCommandBuffer tlasBuildCommandBuffer = beginSingleTimeCommands();
    
    vkCmdBuildAccelerationStructuresKHR(tlasBuildCommandBuffer, 1, &tlasBuildInfo, &pTlasBuildRangeInfo);
    
    endSingleTimeCommands(tlasBuildCommandBuffer);
    
    // Cleanup TLAS scratch buffer
    vkDestroyBuffer(device, tlasScratchBuffer, nullptr);
    vkFreeMemory(device, tlasScratchMemory, nullptr);
    
    // Cleanup instance buffer (no longer needed)
    vkDestroyBuffer(device, instanceBuffer, nullptr);
    vkFreeMemory(device, instanceBufferMemory, nullptr);
    
    std::cout << "🎉 ✅ COMPLETE ACCELERATION STRUCTURE HIERARCHY BUILT!" << std::endl;
    std::cout << "   - BLAS: " << primitiveCount << " triangles (" << blasSizeInfo.accelerationStructureSize << " bytes)" << std::endl;
    std::cout << "   - TLAS: " << instanceCount << " instance (" << tlasSizeInfo.accelerationStructureSize << " bytes)" << std::endl;
    std::cout << "   - Total hierarchy: TLAS → BLAS → " << primitiveCount << " triangles" << std::endl;
    std::cout << "🚀 RTX RAY TRACING INFRASTRUCTURE READY!" << std::endl;
}

void RayTracingPipeline::createShaderBindingTable() {
    std::cout << "Creating REAL Shader Binding Table with actual handles!" << std::endl;
    
    // Get RT pipeline properties
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProps{};
    rtPipelineProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    
    VkPhysicalDeviceProperties2 deviceProps{};
    deviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProps.pNext = &rtPipelineProps;
    vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps);
    
    const uint32_t handleSize = rtPipelineProps.shaderGroupHandleSize;
    const uint32_t handleAlignment = rtPipelineProps.shaderGroupHandleAlignment;
    const uint32_t numGroups = 4; // raygen, miss, shadowMiss, hitGroup
    
    std::cout << "   - Handle size: " << handleSize << " bytes" << std::endl;
    std::cout << "   - Handle alignment: " << handleAlignment << " bytes" << std::endl;
    std::cout << "   - Number of groups: " << numGroups << std::endl;
    
    // Get shader handles from pipeline
    const uint32_t sbtSize = numGroups * handleSize;
    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    VkResult result = vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, numGroups, 
                                                           sbtSize, shaderHandleStorage.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to get ray tracing shader group handles");
    }
    
    std::cout << "✅ Retrieved " << numGroups << " real shader handles (" << sbtSize << " bytes total)" << std::endl;
    
    // Calculate aligned sizes for SBT regions
    const uint32_t alignedHandleSize = (handleSize + handleAlignment - 1) & ~(handleAlignment - 1);
    
    // Create SBT buffer  
    const uint32_t totalSbtSize = alignedHandleSize * numGroups;
    VulkanHelpers::createBuffer(device, physicalDevice, totalSbtSize,
                               VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               shaderBindingTableBuffer, shaderBindingTableMemory);
    
    // Upload shader handles to SBT buffer
    void* mappedSBT;
    vkMapMemory(device, shaderBindingTableMemory, 0, totalSbtSize, 0, &mappedSBT);
    
    uint8_t* sbtData = reinterpret_cast<uint8_t*>(mappedSBT);
    for (uint32_t i = 0; i < numGroups; i++) {
        memcpy(sbtData + i * alignedHandleSize, 
               shaderHandleStorage.data() + i * handleSize, handleSize);
    }
    
    vkUnmapMemory(device, shaderBindingTableMemory);
    
    // Setup SBT regions with real addresses
    VkDeviceAddress sbtAddress = getBufferDeviceAddress(shaderBindingTableBuffer);
    
    // Raygen region (Group 0)
    raygenRegion.deviceAddress = sbtAddress;
    raygenRegion.stride = alignedHandleSize;
    raygenRegion.size = alignedHandleSize;
    
    // Miss region (Groups 1-2: miss + shadowMiss)
    missRegion.deviceAddress = sbtAddress + alignedHandleSize;
    missRegion.stride = alignedHandleSize;
    missRegion.size = alignedHandleSize * 2;
    
    // Hit region (Group 3)
    hitRegion.deviceAddress = sbtAddress + alignedHandleSize * 3;
    hitRegion.stride = alignedHandleSize;
    hitRegion.size = alignedHandleSize;
    
    // Callable region (unused for now)
    callableRegion = {};
    
    std::cout << "🚀 ✅ REAL SHADER BINDING TABLE CREATED!" << std::endl;
    std::cout << "   - SBT buffer: " << totalSbtSize << " bytes at 0x" << std::hex << sbtAddress << std::dec << std::endl;
    std::cout << "   - Raygen region: 0x" << std::hex << raygenRegion.deviceAddress << std::dec << std::endl;
    std::cout << "   - Miss region: 0x" << std::hex << missRegion.deviceAddress << std::dec << std::endl;
    std::cout << "   - Hit region: 0x" << std::hex << hitRegion.deviceAddress << std::dec << std::endl;
}

void RayTracingPipeline::traceRays(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height, VkDescriptorSet descriptorSet) {
    std::cout << "🔥 EXECUTING REAL RAY TRACING DISPATCH WITH TLAS! 🔥" << std::endl;
    std::cout << "   - Resolution: " << width << "x" << height << std::endl;
    std::cout << "   - Using TLAS: 0x" << std::hex << getAccelerationStructureDeviceAddress(topLevelAS) << std::dec << std::endl;
    
    // Bind ray tracing pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
    
    // CRITICAL: Bind descriptor set with TLAS!
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 
                           pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    
    std::cout << "✅ Descriptor set bound with TLAS!" << std::endl;
    
    // REAL RAY TRACING DISPATCH WITH OUR ACCELERATION STRUCTURES!
    vkCmdTraceRaysKHR(commandBuffer,
                      &raygenRegion,   // Raygen shader region
                      &missRegion,     // Miss shader region  
                      &hitRegion,      // Hit group region
                      &callableRegion, // Callable region (unused)
                      width, height, 1);
    
    std::cout << "⚡ vkCmdTraceRaysKHR dispatched with real TLAS and descriptor set!" << std::endl;
    std::cout << "🎯 Tracing " << (width * height) << " rays through Clippy geometry!" << std::endl;
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