// Complete Vulkan Pipeline Implementations - Part of ClippyRTXApp
// This file contains the remaining pipeline implementations

#include "ClippyRTXApp.h"
#include <array>
#include <stdexcept>
#include <cstring>
#include <iostream>

// Descriptor Set Layout Implementation
void ClippyRTXApp::createDescriptorSetLayout() {
    std::cout << "Creating descriptor set layout with TLAS support..." << std::endl;
    
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    
    // Binding 0: TLAS for ray tracing (NEW!)
    VkDescriptorSetLayoutBinding tlasLayoutBinding{};
    tlasLayoutBinding.binding = 0;
    tlasLayoutBinding.descriptorCount = 1;
    tlasLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    tlasLayoutBinding.pImmutableSamplers = nullptr;
    tlasLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    bindings.push_back(tlasLayoutBinding);
    
    // Binding 1: Ray tracing output image (NEW!)
    VkDescriptorSetLayoutBinding imageLayoutBinding{};
    imageLayoutBinding.binding = 1;
    imageLayoutBinding.descriptorCount = 1;
    imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageLayoutBinding.pImmutableSamplers = nullptr;
    imageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    bindings.push_back(imageLayoutBinding);
    
    // Binding 2: Accumulation buffer (NEW!)
    VkDescriptorSetLayoutBinding accumulationLayoutBinding{};
    accumulationLayoutBinding.binding = 2;
    accumulationLayoutBinding.descriptorCount = 1;
    accumulationLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    accumulationLayoutBinding.pImmutableSamplers = nullptr;
    accumulationLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    bindings.push_back(accumulationLayoutBinding);
    
    // Binding 3: Camera uniform buffer (MOVED from binding 0)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 3;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | 
                                 VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    bindings.push_back(uboLayoutBinding);
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
    
    std::cout << "✅ Descriptor set layout created with " << bindings.size() << " bindings:" << std::endl;
    std::cout << "   - Binding 0: TLAS (acceleration structure)" << std::endl;
    std::cout << "   - Binding 1: Ray tracing output image" << std::endl;
    std::cout << "   - Binding 2: Accumulation buffer" << std::endl;
    std::cout << "   - Binding 3: Camera uniform buffer" << std::endl;
}

// Graphics Pipeline Implementation
void ClippyRTXApp::createGraphicsPipeline() {
    auto vertShaderCode = VulkanHelpers::readFile("shaders/vertex_basic.vert.spv");
    auto fragShaderCode = VulkanHelpers::readFile("shaders/fragment_basic.frag.spv");
    
    VkShaderModule vertShaderModule = VulkanHelpers::createShaderModule(device, vertShaderCode);
    VkShaderModule fragShaderModule = VulkanHelpers::createShaderModule(device, fragShaderCode);
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // Disable culling for debugging
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.rasterizationSamples = msaaSamples;
    multisampling.minSampleShading = .2f;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
    
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

// Command Pool Implementation
void ClippyRTXApp::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = VulkanHelpers::findQueueFamilies(physicalDevice, surface);
    
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

// Color Resources (MSAA) Implementation
void ClippyRTXApp::createColorResources() {
    VkFormat colorFormat = swapChainImageFormat;
    
    VulkanHelpers::createImage(device, physicalDevice, swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, 
                              colorFormat, VK_IMAGE_TILING_OPTIMAL, 
                              VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
    
    colorImageView = VulkanHelpers::createImageView(device, colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

// Depth Resources Implementation
void ClippyRTXApp::createDepthResources() {
    VkFormat depthFormat = VulkanHelpers::findDepthFormat(physicalDevice);
    
    VulkanHelpers::createImage(device, physicalDevice, swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, 
                              depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    
    depthImageView = VulkanHelpers::createImageView(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

// Framebuffers Implementation
void ClippyRTXApp::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());
    
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            colorImageView,
            depthImageView,
            swapChainImageViews[i]
        };
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;
        
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void ClippyRTXApp::createUIFramebuffers() {
    uiFramebuffers.resize(swapChainImageViews.size());
    
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            swapChainImageViews[i]  // Direct to swapchain image
        };
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = uiRenderPass;  // Use UI render pass
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;
        
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &uiFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create UI framebuffer!");
        }
    }
    
    std::cout << "UI overlay framebuffers created (preserves RTX content)" << std::endl;
}

// Vertex Buffer Implementation
void ClippyRTXApp::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VulkanHelpers::createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                               stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
    
    VulkanHelpers::createBuffer(device, physicalDevice, bufferSize, 
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    
    VulkanHelpers::copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, vertexBuffer, bufferSize);
    
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

// Index Buffer Implementation
void ClippyRTXApp::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VulkanHelpers::createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                               stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
    
    VulkanHelpers::createBuffer(device, physicalDevice, bufferSize, 
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    
    VulkanHelpers::copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, indexBuffer, bufferSize);
    
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

// Uniform Buffers Implementation
void ClippyRTXApp::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VulkanHelpers::createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                                   uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

// Descriptor Pool Implementation
void ClippyRTXApp::createDescriptorPool() {
    std::cout << "Creating descriptor pool with RTX support..." << std::endl;
    
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    
    // Acceleration structure (TLAS) - binding 0
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    
    // Storage images (bindings 1 and 2) - output and accumulation
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2);  // 2 images per frame
    
    // Uniform buffer (binding 3) - camera data
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
    
    std::cout << "✅ Descriptor pool created with:" << std::endl;
    std::cout << "   - " << poolSizes[0].descriptorCount << " acceleration structures" << std::endl;
    std::cout << "   - " << poolSizes[1].descriptorCount << " storage images" << std::endl;
    std::cout << "   - " << poolSizes[2].descriptorCount << " uniform buffers" << std::endl;
}

// Descriptor Sets Implementation
void ClippyRTXApp::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();
    
    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);
        
        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
        
        // Only bind uniform buffer to binding 3 now (for ray tracing compatibility)
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 3;  // Changed from 0 to 3 for ray tracing
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
    
    std::cout << "✅ Descriptor sets created (uniform buffer only - TLAS will be added later)" << std::endl;
}

// Update Descriptor Sets with TLAS and Storage Images for Ray Tracing
void ClippyRTXApp::updateDescriptorSetsWithTLAS() {
    if (!rayTracingPipeline) {
        std::cout << "❌ Cannot update descriptor sets - no ray tracing pipeline" << std::endl;
        return;
    }
    
    std::cout << "Updating descriptor sets with TLAS and storage images..." << std::endl;
    VkAccelerationStructureKHR tlas = rayTracingPipeline->getTopLevelAS();
    
    if (tlas == VK_NULL_HANDLE) {
        std::cout << "❌ TLAS is null - cannot bind to descriptor set" << std::endl;
        return;
    }
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        
        // Binding 0: TLAS (Acceleration Structure)
        VkWriteDescriptorSetAccelerationStructureKHR tlasDescriptor{};
        tlasDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        tlasDescriptor.accelerationStructureCount = 1;
        tlasDescriptor.pAccelerationStructures = &tlas;
        
        VkWriteDescriptorSet tlasWrite{};
        tlasWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        tlasWrite.pNext = &tlasDescriptor;
        tlasWrite.dstSet = descriptorSets[i];
        tlasWrite.dstBinding = 0;
        tlasWrite.dstArrayElement = 0;
        tlasWrite.descriptorCount = 1;
        tlasWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        descriptorWrites.push_back(tlasWrite);
        
        // Binding 1: RT Output Image (Storage Image)
        VkDescriptorImageInfo rtOutputImageInfo{};
        rtOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        rtOutputImageInfo.imageView = rtOutputImageView;
        rtOutputImageInfo.sampler = VK_NULL_HANDLE;  // No sampler needed for storage images
        
        VkWriteDescriptorSet rtOutputWrite{};
        rtOutputWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        rtOutputWrite.dstSet = descriptorSets[i];
        rtOutputWrite.dstBinding = 1;
        rtOutputWrite.dstArrayElement = 0;
        rtOutputWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        rtOutputWrite.descriptorCount = 1;
        rtOutputWrite.pImageInfo = &rtOutputImageInfo;
        descriptorWrites.push_back(rtOutputWrite);
        
        // Binding 2: Accumulation Image (Storage Image)
        VkDescriptorImageInfo rtAccumImageInfo{};
        rtAccumImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        rtAccumImageInfo.imageView = rtAccumulationImageView;
        rtAccumImageInfo.sampler = VK_NULL_HANDLE;  // No sampler needed for storage images
        
        VkWriteDescriptorSet rtAccumWrite{};
        rtAccumWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        rtAccumWrite.dstSet = descriptorSets[i];
        rtAccumWrite.dstBinding = 2;
        rtAccumWrite.dstArrayElement = 0;
        rtAccumWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        rtAccumWrite.descriptorCount = 1;
        rtAccumWrite.pImageInfo = &rtAccumImageInfo;
        descriptorWrites.push_back(rtAccumWrite);
        
        // Update all descriptor sets at once
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), 
                              descriptorWrites.data(), 0, nullptr);
    }
    
    std::cout << "✅ Descriptor sets updated with complete RTX bindings for " << MAX_FRAMES_IN_FLIGHT << " frames:" << std::endl;
    std::cout << "   - Binding 0: TLAS (acceleration structure)" << std::endl;
    std::cout << "   - Binding 1: RT output image (" << swapChainExtent.width << "x" << swapChainExtent.height << ")" << std::endl;
    std::cout << "   - Binding 2: Accumulation image (" << swapChainExtent.width << "x" << swapChainExtent.height << ")" << std::endl;
    std::cout << "   - Binding 3: Uniform buffer (already bound)" << std::endl;
}

// Command Buffers Implementation
void ClippyRTXApp::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();
    
    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

// Synchronization Objects Implementation
void ClippyRTXApp::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

// Swapchain Recreation Implementation
void ClippyRTXApp::recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    
    vkDeviceWaitIdle(device);
    
    cleanupSwapChain();
    
    createSwapChain();
    createImageViews();
    createColorResources();
    createDepthResources();
    createRayTracingStorageImages();  // Recreate RT storage images with new size
    createFramebuffers();
    
    // Update descriptor sets with new storage images
    if (rayTracingPipeline) {
        updateDescriptorSetsWithTLAS();
    }
}

// Ray Tracing Storage Images Implementation
void ClippyRTXApp::createRayTracingStorageImages() {
    std::cout << "Creating ray tracing storage images..." << std::endl;
    
    // Create RT output image (for final ray traced result)
    VulkanHelpers::createImage(
        device, physicalDevice,
        swapChainExtent.width, swapChainExtent.height, 1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_R8G8B8A8_UNORM,  // Standard RGBA format
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        rtOutputImage, rtOutputImageMemory
    );
    
    rtOutputImageView = VulkanHelpers::createImageView(
        device, rtOutputImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1
    );
    
    // Create accumulation image (for progressive rendering)
    VulkanHelpers::createImage(
        device, physicalDevice,
        swapChainExtent.width, swapChainExtent.height, 1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_R32G32B32A32_SFLOAT,  // High precision float for accumulation
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        rtAccumulationImage, rtAccumulationImageMemory
    );
    
    rtAccumulationImageView = VulkanHelpers::createImageView(
        device, rtAccumulationImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1
    );
    
    std::cout << "✅ Ray tracing storage images created:" << std::endl;
    std::cout << "   - RT Output: " << swapChainExtent.width << "x" << swapChainExtent.height << " RGBA8" << std::endl;
    std::cout << "   - Accumulation: " << swapChainExtent.width << "x" << swapChainExtent.height << " RGBA32F" << std::endl;
}

// Swapchain Cleanup Implementation
void ClippyRTXApp::cleanupSwapChain() {
    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);
    
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
    
    // Cleanup ray tracing storage images
    if (rtOutputImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, rtOutputImageView, nullptr);
        rtOutputImageView = VK_NULL_HANDLE;
    }
    if (rtOutputImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, rtOutputImage, nullptr);
        rtOutputImage = VK_NULL_HANDLE;
    }
    if (rtOutputImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, rtOutputImageMemory, nullptr);
        rtOutputImageMemory = VK_NULL_HANDLE;
    }
    
    if (rtAccumulationImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, rtAccumulationImageView, nullptr);
        rtAccumulationImageView = VK_NULL_HANDLE;
    }
    if (rtAccumulationImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, rtAccumulationImage, nullptr);
        rtAccumulationImage = VK_NULL_HANDLE;
    }
    if (rtAccumulationImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, rtAccumulationImageMemory, nullptr);
        rtAccumulationImageMemory = VK_NULL_HANDLE;
    }
    
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    
    // Cleanup UI framebuffers
    for (auto framebuffer : uiFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    
    // Cleanup render passes
    if (uiRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, uiRenderPass, nullptr);
        uiRenderPass = VK_NULL_HANDLE;
    }
    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }
    
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    
    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

// Copy RT Output Image to Swapchain for Display (WORKING VERSION!)
void ClippyRTXApp::copyRTOutputToSwapchain(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    std::cout << "🖼️ Copying RT output to swapchain image " << imageIndex << std::endl;
    
    // Transition RT output image to transfer src layout
    VkImageMemoryBarrier rtImageBarrier{};
    rtImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    rtImageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    rtImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    rtImageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    rtImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    rtImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    rtImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    rtImageBarrier.image = rtOutputImage;
    rtImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    rtImageBarrier.subresourceRange.baseMipLevel = 0;
    rtImageBarrier.subresourceRange.levelCount = 1;
    rtImageBarrier.subresourceRange.baseArrayLayer = 0;
    rtImageBarrier.subresourceRange.layerCount = 1;
    
    // Transition swapchain image to transfer dst layout
    VkImageMemoryBarrier swapImageBarrier{};
    swapImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swapImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    swapImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    swapImageBarrier.srcAccessMask = 0;
    swapImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    swapImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swapImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swapImageBarrier.image = swapChainImages[imageIndex];
    swapImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    swapImageBarrier.subresourceRange.baseMipLevel = 0;
    swapImageBarrier.subresourceRange.levelCount = 1;
    swapImageBarrier.subresourceRange.baseArrayLayer = 0;
    swapImageBarrier.subresourceRange.layerCount = 1;
    
    VkImageMemoryBarrier barriers[] = {rtImageBarrier, swapImageBarrier};
    
    vkCmdPipelineBarrier(commandBuffer,
                        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0, 0, nullptr, 0, nullptr, 2, barriers);
    
    // Copy RT output image to swapchain image
    VkImageCopy copyRegion{};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcOffset = {0, 0, 0};
    
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.dstOffset = {0, 0, 0};
    
    copyRegion.extent.width = swapChainExtent.width;
    copyRegion.extent.height = swapChainExtent.height;
    copyRegion.extent.depth = 1;
    
    vkCmdCopyImage(commandBuffer,
                   rtOutputImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &copyRegion);
    
    // Transition swapchain image to present layout
    VkImageMemoryBarrier presentBarrier{};
    presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    presentBarrier.dstAccessMask = 0;
    presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    presentBarrier.image = swapChainImages[imageIndex];
    presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    presentBarrier.subresourceRange.baseMipLevel = 0;
    presentBarrier.subresourceRange.levelCount = 1;
    presentBarrier.subresourceRange.baseArrayLayer = 0;
    presentBarrier.subresourceRange.layerCount = 1;
    
    // Transition RT output image back to general layout for next frame
    VkImageMemoryBarrier rtBackBarrier{};
    rtBackBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    rtBackBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    rtBackBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    rtBackBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    rtBackBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    rtBackBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    rtBackBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    rtBackBarrier.image = rtOutputImage;
    rtBackBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    rtBackBarrier.subresourceRange.baseMipLevel = 0;
    rtBackBarrier.subresourceRange.levelCount = 1;
    rtBackBarrier.subresourceRange.baseArrayLayer = 0;
    rtBackBarrier.subresourceRange.layerCount = 1;
    
    VkImageMemoryBarrier finalBarriers[] = {presentBarrier, rtBackBarrier};
    
    vkCmdPipelineBarrier(commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                        0, 0, nullptr, 0, nullptr, 2, finalBarriers);
    
    std::cout << "✅ RT output copied to swapchain - ready for display!" << std::endl;
}

