#include "PostProcessing.h"
#include "VulkanHelpers.h"
#include <stdexcept>
#include <fstream>
#include <array>
#include <cstring>
#include <iostream>

PostProcessing::PostProcessing(VkDevice device, VkPhysicalDevice physicalDevice, VkRenderPass renderPass, VkExtent2D extent)
    : device(device), physicalDevice(physicalDevice), renderPass(renderPass), extent(extent) {
    
    // Initialize default uniforms
    uniforms.time = 0.0f;
    uniforms.exposure = 1.0f;
    uniforms.gamma = 2.2f;
    uniforms.contrast = 1.0f;
    uniforms.saturation = 1.0f;
    uniforms.vignetteStrength = 0.5f;
    uniforms.chromaticAberration = 0.002f;
    uniforms.filmGrain = 0.05f;
    uniforms.bloomIntensity = 0.3f;
    uniforms.bloomRadius = 1.0f;
    uniforms.resolution = glm::vec2(extent.width, extent.height);
    uniforms.enableTonemap = 1;
    uniforms.enableBloom = 1;
    uniforms.enableVignette = 1;
    uniforms.enableChromaticAberration = 1;
    uniforms.enableFilmGrain = 1;
    
    createPostProcessResources();
    createPostProcessPipeline();
}

PostProcessing::~PostProcessing() {
    cleanup();
}

void PostProcessing::cleanup() {
    // Clean up bloom resources
    for (size_t i = 0; i < bloomFramebuffers.size(); i++) {
        vkDestroyFramebuffer(device, bloomFramebuffers[i], nullptr);
        vkDestroyImageView(device, bloomImageViews[i], nullptr);
        vkDestroyImage(device, bloomImages[i], nullptr);
        vkFreeMemory(device, bloomImageMemories[i], nullptr);
    }
    bloomFramebuffers.clear();
    bloomImageViews.clear();
    bloomImages.clear();
    bloomImageMemories.clear();
    
    // Clean up post-process resources
    if (postProcessFramebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device, postProcessFramebuffer, nullptr);
        postProcessFramebuffer = VK_NULL_HANDLE;
    }
    
    if (postProcessImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, postProcessImageView, nullptr);
        postProcessImageView = VK_NULL_HANDLE;
    }
    
    if (postProcessImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, postProcessImage, nullptr);
        vkFreeMemory(device, postProcessImageMemory, nullptr);
        postProcessImage = VK_NULL_HANDLE;
    }
    
    // Clean up uniform buffers
    for (size_t i = 0; i < uniformBuffers.size(); i++) {
        if (uniformBuffersMapped[i]) {
            vkUnmapMemory(device, uniformBuffersMemory[i]);
        }
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }
    uniformBuffers.clear();
    uniformBuffersMemory.clear();
    uniformBuffersMapped.clear();
    
    // Clean up samplers
    if (colorSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, colorSampler, nullptr);
        colorSampler = VK_NULL_HANDLE;
    }
    
    if (bloomSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, bloomSampler, nullptr);
        bloomSampler = VK_NULL_HANDLE;
    }
    
    // Clean up pipeline resources
    if (postProcessPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, postProcessPipeline, nullptr);
        postProcessPipeline = VK_NULL_HANDLE;
    }
    
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
    
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
    
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
}

void PostProcessing::createPostProcessResources() {
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSets();
    createUniformBuffers();
    createSamplers();
    createBloomResources();
}

void PostProcessing::createDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
    
    // Color texture binding
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].pImmutableSamplers = nullptr;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    // Bloom texture binding
    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].pImmutableSamplers = nullptr;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    // Uniform buffer binding
    bindings[2].binding = 2;
    bindings[2].descriptorCount = 1;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].pImmutableSamplers = nullptr;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create post-process descriptor set layout!");
    }
}

void PostProcessing::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = 4; // 2 samplers * 2 frames
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = 2; // 2 frames
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 2; // 2 frames
    
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create post-process descriptor pool!");
    }
}

void PostProcessing::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(2, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();
    
    descriptorSets.resize(2);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate post-process descriptor sets!");
    }
}

void PostProcessing::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(PostProcessUniforms);
    
    uniformBuffers.resize(2);
    uniformBuffersMemory.resize(2);
    uniformBuffersMapped.resize(2);
    
    for (size_t i = 0; i < 2; i++) {
        VulkanHelpers::createBuffer(device, physicalDevice, bufferSize,
                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    uniformBuffers[i], uniformBuffersMemory[i]);
        
        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void PostProcessing::createSamplers() {
    // Color sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    
    if (vkCreateSampler(device, &samplerInfo, nullptr, &colorSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create color sampler!");
    }
    
    // Bloom sampler
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    
    if (vkCreateSampler(device, &samplerInfo, nullptr, &bloomSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create bloom sampler!");
    }
}

void PostProcessing::createBloomResources() {
    // Create multiple bloom textures for blur passes
    bloomImages.resize(2);
    bloomImageMemories.resize(2);
    bloomImageViews.resize(2);
    bloomFramebuffers.resize(2);
    
    for (int i = 0; i < 2; i++) {
        VkExtent2D bloomExtent = { extent.width / (2 + i), extent.height / (2 + i) };
        
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = bloomExtent.width;
        imageInfo.extent.height = bloomExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        if (vkCreateImage(device, &imageInfo, nullptr, &bloomImages[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create bloom image!");
        }
        
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, bloomImages[i], &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(physicalDevice, memRequirements.memoryTypeBits,
                                                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        if (vkAllocateMemory(device, &allocInfo, nullptr, &bloomImageMemories[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate bloom image memory!");
        }
        
        vkBindImageMemory(device, bloomImages[i], bloomImageMemories[i], 0);
        
        // Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = bloomImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(device, &viewInfo, nullptr, &bloomImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create bloom image view!");
        }
    }
}

void PostProcessing::createPostProcessPipeline() {
    auto vertShaderCode = readFile("shaders/postprocess.vert.spv");
    auto fragShaderCode = readFile("shaders/postprocess.frag.spv");
    
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
    
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
    
    // Vertex input (full screen quad)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) extent.width;
    viewport.height = (float) extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    
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
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
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
        throw std::runtime_error("failed to create post-process pipeline layout!");
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
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &postProcessPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create post-process graphics pipeline!");
    }
    
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void PostProcessing::render(VkCommandBuffer commandBuffer, VkImageView sourceImage, uint32_t currentFrame) {
    updateDescriptorSet(currentFrame, sourceImage);
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postProcessPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, 
                           &descriptorSets[currentFrame], 0, nullptr);
    
    // Draw full screen quad (no vertex data needed, generated in vertex shader)
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void PostProcessing::updateUniforms(uint32_t currentFrame, float time) {
    uniforms.time = time;
    uniforms.resolution = glm::vec2(extent.width, extent.height);
    
    memcpy(uniformBuffersMapped[currentFrame], &uniforms, sizeof(uniforms));
}

void PostProcessing::updateDescriptorSet(uint32_t currentFrame, VkImageView sourceImageView) {
    std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
    
    // Color texture
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = sourceImageView;
    imageInfo.sampler = colorSampler;
    
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSets[currentFrame];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfo;
    
    // Bloom texture (use first bloom image for now)
    VkDescriptorImageInfo bloomImageInfo{};
    bloomImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    bloomImageInfo.imageView = bloomImageViews[0];
    bloomImageInfo.sampler = bloomSampler;
    
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSets[currentFrame];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &bloomImageInfo;
    
    // Uniform buffer
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[currentFrame];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(PostProcessUniforms);
    
    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = descriptorSets[currentFrame];
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &bufferInfo;
    
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void PostProcessing::recreateResources(VkExtent2D newExtent) {
    extent = newExtent;
    uniforms.resolution = glm::vec2(extent.width, extent.height);
    
    // Cleanup old bloom resources
    for (size_t i = 0; i < bloomFramebuffers.size(); i++) {
        vkDestroyFramebuffer(device, bloomFramebuffers[i], nullptr);
        vkDestroyImageView(device, bloomImageViews[i], nullptr);
        vkDestroyImage(device, bloomImages[i], nullptr);
        vkFreeMemory(device, bloomImageMemories[i], nullptr);
    }
    bloomFramebuffers.clear();
    bloomImageViews.clear();
    bloomImages.clear();
    bloomImageMemories.clear();
    
    // Recreate bloom resources with new size
    createBloomResources();
}

VkShaderModule PostProcessing::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    
    return shaderModule;
}

std::vector<char> PostProcessing::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }
    
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    
    return buffer;
}