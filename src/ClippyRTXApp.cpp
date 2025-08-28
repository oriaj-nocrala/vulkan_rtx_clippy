#include "ClippyRTXApp.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <set>
#include <algorithm>
#include <array>
#include <chrono>

// Validation layers para debug
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// Extensions necesarias para RTX
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
};

#ifdef ENABLE_VALIDATION_LAYERS
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif

// Extension functions
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
                                      const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, 
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void ClippyRTXApp::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void ClippyRTXApp::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    
    window = glfwCreateWindow(WIDTH, HEIGHT, "Clippy RTX - Vulkan Ray Tracing", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
}

void ClippyRTXApp::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<ClippyRTXApp*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void ClippyRTXApp::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto app = reinterpret_cast<ClippyRTXApp*>(glfwGetWindowUserPointer(window));
    
    if (action == GLFW_PRESS) {
        switch(key) {
            case GLFW_KEY_SPACE:
                app->rtxEnabled = !app->rtxEnabled;
                std::cout << "RTX " << (app->rtxEnabled ? "ON" : "OFF") << std::endl;
                break;
            case GLFW_KEY_1:
                app->currentAnimationMode = AnimationMode::EXCITED;
                std::cout << "Mode: EXCITED" << std::endl;
                break;
            case GLFW_KEY_2:
                app->currentAnimationMode = AnimationMode::HELPING;
                std::cout << "Mode: HELPING" << std::endl;
                break;
            case GLFW_KEY_3:
                app->currentAnimationMode = AnimationMode::QUANTUM;
                std::cout << "Mode: QUANTUM" << std::endl;
                break;
            case GLFW_KEY_4:
                app->currentAnimationMode = AnimationMode::PARTY;
                std::cout << "Mode: PARTY" << std::endl;
                break;
            case GLFW_KEY_5:
                app->currentAnimationMode = AnimationMode::MATRIX;
                std::cout << "Mode: MATRIX" << std::endl;
                break;
            case GLFW_KEY_R:
                app->currentAnimationMode = AnimationMode::IDLE;
                std::cout << "Mode: RESET TO IDLE" << std::endl;
                break;
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
        }
    }
}

void ClippyRTXApp::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto app = reinterpret_cast<ClippyRTXApp*>(glfwGetWindowUserPointer(window));
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            app->mousePressed = true;
            app->currentAnimationMode = AnimationMode::EXCITED;
            std::cout << "Clippy clicked! Mode: EXCITED" << std::endl;
        } else if (action == GLFW_RELEASE) {
            app->mousePressed = false;
        }
    }
}

void ClippyRTXApp::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    auto app = reinterpret_cast<ClippyRTXApp*>(glfwGetWindowUserPointer(window));
    
    app->mouseX = xpos;
    app->mouseY = ypos;
}

void ClippyRTXApp::updateAnimationMode() {
    // Auto-return to IDLE after some time for most modes
    static float modeTimer = 0.0f;
    modeTimer += deltaTime;
    
    switch(currentAnimationMode) {
        case AnimationMode::EXCITED:
            if (modeTimer > 3.0f) {
                currentAnimationMode = AnimationMode::IDLE;
                modeTimer = 0.0f;
            }
            break;
        case AnimationMode::HELPING:
            if (modeTimer > 4.0f) {
                currentAnimationMode = AnimationMode::IDLE;
                modeTimer = 0.0f;
            }
            break;
        case AnimationMode::THINKING:
            if (modeTimer > 5.0f) {
                currentAnimationMode = AnimationMode::IDLE;
                modeTimer = 0.0f;
            }
            break;
        case AnimationMode::IDLE:
            modeTimer = 0.0f;
            break;
        default:
            // QUANTUM, PARTY, MATRIX modes stay until manually changed
            break;
    }
}

void ClippyRTXApp::handleMouseInteraction() {
    // Update mouse position in normalized coordinates
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    
    // Normalize mouse coordinates to 0-1 range
    float normalizedX = static_cast<float>(mouseX) / static_cast<float>(windowWidth);
    float normalizedY = 1.0f - static_cast<float>(mouseY) / static_cast<float>(windowHeight); // Flip Y
    
    // Store for shader usage
    // This will be used in updateUniformBuffer
}

void ClippyRTXApp::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createColorResources();
    createDepthResources();
    createRayTracingStorageImages();
    createFramebuffers();
    createClippyGeometry();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
    
    // Si la GPU soporta RTX, crear estructuras de aceleración
    if (checkRayTracingSupport()) {
        setupRayTracing();
    } else {
        std::cout << "Ray Tracing not supported - falling back to rasterization" << std::endl;
        rtxEnabled = false;
    }
    
    // Setup UI system
    setupUI();
    
    // DISABLED POST-PROCESSING TO DEBUG GOLD COLOR
    // setupPostProcessing();
    
    std::cout << "All initialization completed, ready to start main loop!" << std::endl;
}

bool ClippyRTXApp::checkRayTracingSupport() {
    // Verificar extensiones
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
    
    bool hasRayTracingExtensions = true;
    std::vector<const char*> rtExtensions = {
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
    };
    
    for (const char* requiredExt : rtExtensions) {
        bool found = false;
        for (const auto& availableExt : availableExtensions) {
            if (strcmp(requiredExt, availableExt.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            hasRayTracingExtensions = false;
            std::cout << "Missing RT extension: " << requiredExt << std::endl;
            break;
        }
    }
    
    if (!hasRayTracingExtensions) {
        return false;
    }
    
    // Verificar features
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures{};
    rtFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    
    VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{};
    asFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    asFeatures.pNext = &rtFeatures;
    
    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &asFeatures;
    
    vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures);
    
    return rtFeatures.rayTracingPipeline && asFeatures.accelerationStructure;
}

void ClippyRTXApp::setupRayTracing() {
    rayTracingPipeline = std::make_unique<RayTracingPipeline>(device, physicalDevice, commandPool, graphicsQueue);
    rayTracingPipeline->createPipeline(descriptorSetLayout);
    rayTracingPipeline->createAccelerationStructures(vertexBuffer, indexBuffer, 
                                                     static_cast<uint32_t>(vertices.size()),
                                                     static_cast<uint32_t>(indices.size()));
    rayTracingPipeline->createShaderBindingTable();
    
    // Update descriptor sets with TLAS for ray tracing
    updateDescriptorSetsWithTLAS();
    
    std::cout << "Ray Tracing pipeline initialized successfully!" << std::endl;
}

void ClippyRTXApp::createClippyGeometry() {
    // Now restore the full Clippy geometry
    ClippyGeometry::generateClippy(vertices, indices);
    std::cout << "Clippy geometry restored: " << vertices.size() 
              << " vertices, " << indices.size() << " indices" << std::endl;
}

void ClippyRTXApp::mainLoop() {
    std::cout << "Entering main loop..." << std::endl;
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Calcular delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        totalTime += deltaTime;
        frameCount++;
        
        // Update animations and interactions
        updateAnimationMode();
        handleMouseInteraction();
        updateUI();
        updatePostProcessing();
        
        drawFrame();
    }
    
    vkDeviceWaitIdle(device);
}

void ClippyRTXApp::drawFrame() {
    // Debug output removed
    
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, 
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    
    updateUniformBuffer(currentFrame);
    
    // Proper fence synchronization for RTX performance
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];
    
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    
    // Create fresh command buffer for each frame
    
    // For now, let's just submit an empty command buffer to test the pipeline
    VkCommandBuffer tempCmdBuffer;
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    
    if (vkAllocateCommandBuffers(device, &allocInfo, &tempCmdBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate temporary command buffer!");
    }
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    if (vkBeginCommandBuffer(tempCmdBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording temporary command buffer!");
    }
    
    // 🔥 PURE RTX RENDERING - REAL RAY TRACING! 🔥
    if (rtxEnabled && rayTracingPipeline) {
        std::cout << "🔥 EXECUTING REAL RAY TRACING DISPATCH WITH TLAS! 🔥" << std::endl;
        std::cout << "   - Resolution: " << swapChainExtent.width << "x" << swapChainExtent.height << std::endl;
        
        // Step 1: Execute ray tracing OUTSIDE render pass (writes to storage images)
        rayTracingPipeline->traceRays(tempCmdBuffer, swapChainExtent.width, swapChainExtent.height, 
                                     descriptorSets[currentFrame]);
        
        // Step 2: Copy RT output image to swapchain image for display
        copyRTOutputToSwapchain(tempCmdBuffer, imageIndex);
        
        // End the command buffer for RTX path
        if (vkEndCommandBuffer(tempCmdBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to end RTX command buffer!");
        }
        std::cout << "✅ RTX command buffer completed successfully!" << std::endl;
    } else {
        // Fallback Rasterization Path
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.2f, 1.0f}}; // Dark blue for rasterization
        clearValues[1].depthStencil = {1.0f, 0};
        
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(tempCmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        // Traditional rasterization
        vkCmdBindPipeline(tempCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(tempCmdBuffer, 0, 1, &viewport);
        
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(tempCmdBuffer, 0, 1, &scissor);
        
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(tempCmdBuffer, 0, 1, vertexBuffers, offsets);
        
        vkCmdBindIndexBuffer(tempCmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        vkCmdBindDescriptorSets(tempCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, 
                               &descriptorSets[currentFrame], 0, nullptr);
        
        vkCmdDrawIndexed(tempCmdBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        
        vkCmdEndRenderPass(tempCmdBuffer);
        
        // End command buffer for rasterization path
        if (vkEndCommandBuffer(tempCmdBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record rasterization command buffer!");
        }
        std::cout << "✅ Rasterization command buffer completed successfully!" << std::endl;
    }
    
    // Command buffer ready
    
    submitInfo.commandBufferCount = 1;
    // Use the temporary command buffer instead of the problematic one
    submitInfo.pCommandBuffers = &tempCmdBuffer;
    
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    
    // 🔧 ENHANCED GPU HANG DETECTION WITH TIMEOUT MONITORING
    #ifdef ENABLE_RAY_TRACING_DEBUG
    auto submitStartTime = std::chrono::high_resolution_clock::now();
    std::cout << "⏱️  Submitting command buffer with timeout monitoring..." << std::endl;
    #endif
    
    VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
    if (submitResult != VK_SUCCESS) {
        #ifdef ENABLE_RAY_TRACING_DEBUG
        std::cerr << "💥 QUEUE SUBMIT FAILED with result: " << submitResult << std::endl;
        if (submitResult == VK_ERROR_DEVICE_LOST) {
            std::cerr << "🚨 DEVICE LOST - Possible GPU hang or driver crash!" << std::endl;
        }
        #endif
        throw std::runtime_error("failed to submit draw command buffer!");
    }
    
    #ifdef ENABLE_RAY_TRACING_DEBUG
    auto submitEndTime = std::chrono::high_resolution_clock::now();
    auto submitDuration = std::chrono::duration_cast<std::chrono::milliseconds>(submitEndTime - submitStartTime);
    std::cout << "✅ Command buffer submitted in " << submitDuration.count() << "ms" << std::endl;
    
    // Monitor fence wait time with timeout
    auto fenceWaitStart = std::chrono::high_resolution_clock::now();
    VkResult fenceResult = vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, 2000000000); // 2 second timeout
    auto fenceWaitEnd = std::chrono::high_resolution_clock::now();
    auto fenceWaitDuration = std::chrono::duration_cast<std::chrono::milliseconds>(fenceWaitEnd - fenceWaitStart);
    
    if (fenceResult == VK_TIMEOUT) {
        std::cerr << "⚠️  FENCE TIMEOUT after 2 seconds - Possible GPU hang!" << std::endl;
        std::cerr << "   Ray tracing dispatch might be stuck in infinite loop" << std::endl;
        std::cerr << "   Check shader recursion limits and termination conditions" << std::endl;
        // Don't throw - let it continue and see validation layer output
    } else if (fenceResult == VK_SUCCESS) {
        std::cout << "🔄 GPU work completed in " << fenceWaitDuration.count() << "ms" << std::endl;
        if (fenceWaitDuration.count() > 500) {
            std::cout << "⚠️  High GPU execution time - monitoring for hangs" << std::endl;
        }
    } else {
        std::cerr << "💥 FENCE WAIT FAILED with result: " << fenceResult << std::endl;
    }
    #endif
    
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    
    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
    
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void ClippyRTXApp::updateUniformBuffer(uint32_t currentImage) {
    UniformBufferObject ubo{};
    
    // Enhanced animation based on current mode
    float rotationSpeed = 0.5f;
    float bounceHeight = 0.1f;
    float bounceSpeed = 2.0f;
    
    switch(currentAnimationMode) {
        case AnimationMode::EXCITED:
            rotationSpeed = 2.0f;
            bounceHeight = 0.3f;
            bounceSpeed = 8.0f;
            clippyMaterial.albedo = glm::vec3(1.0f, 0.9f, 0.2f); // Bright gold
            break;
        case AnimationMode::HELPING:
            rotationSpeed = 1.0f;
            bounceHeight = 0.05f;
            bounceSpeed = 1.0f;
            clippyMaterial.albedo = glm::vec3(0.2f, 0.8f, 1.0f); // Helpful blue
            break;
        case AnimationMode::QUANTUM:
            rotationSpeed = 3.0f;
            bounceHeight = 0.2f;
            bounceSpeed = 5.0f;
            clippyMaterial.albedo = glm::vec3(0.8f, 0.2f, 1.0f); // Quantum purple
            clippyMaterial.metallic = 1.0f;
            clippyMaterial.roughness = 0.0f;
            break;
        case AnimationMode::PARTY: {
            rotationSpeed = 4.0f;
            bounceHeight = 0.4f;
            bounceSpeed = 12.0f;
            // Animated color in party mode
            float hue = fmod(totalTime * 2.0f, 2.0f * glm::pi<float>());
            clippyMaterial.albedo = glm::vec3(
                0.5f + 0.5f * sin(hue),
                0.5f + 0.5f * sin(hue + 2.09f), // 120 degrees
                0.5f + 0.5f * sin(hue + 4.19f)  // 240 degrees
            );
            break;
        }
        case AnimationMode::MATRIX:
            rotationSpeed = 0.3f;
            bounceHeight = 0.0f;
            bounceSpeed = 0.0f;
            clippyMaterial.albedo = glm::vec3(0.0f, 1.0f, 0.0f); // Matrix green
            break;
        default: // IDLE
            rotationSpeed = 0.5f;
            bounceHeight = 0.1f;
            bounceSpeed = 2.0f;
            clippyMaterial.albedo = glm::vec3(1.0f, 0.843f, 0.0f); // Classic gold
            break;
    }
    
    // Restore animation
    ubo.model = glm::rotate(glm::mat4(1.0f), totalTime * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.model = glm::translate(ubo.model, glm::vec3(0.0f, sin(totalTime * bounceSpeed) * bounceHeight, 0.0f));
    
    // Enhanced camera system restored
    float cameraRadius = 5.0f;
    float cameraHeight = 2.0f;
    float cameraSpeed = 0.3f;
    
    // Matrix mode special camera movement
    if (currentAnimationMode == AnimationMode::MATRIX) {
        cameraHeight += sin(totalTime * 0.5f) * 2.0f;
        cameraRadius += cos(totalTime * 0.3f) * 1.0f;
        cameraSpeed = 0.1f;
    }
    
    glm::vec3 cameraPos = glm::vec3(
        sin(totalTime * cameraSpeed) * cameraRadius,
        cameraHeight,
        cos(totalTime * cameraSpeed) * cameraRadius
    );
    
    ubo.view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(60.0f), 
        swapChainExtent.width / static_cast<float>(swapChainExtent.height), 0.1f, 100.0f);
    ubo.proj[1][1] *= -1;
    
    ubo.viewInverse = glm::inverse(ubo.view);
    ubo.projInverse = glm::inverse(ubo.proj);
    ubo.cameraPos = cameraPos;
    ubo.time = totalTime;
    ubo.metallic = clippyMaterial.metallic;
    ubo.roughness = clippyMaterial.roughness;
    ubo.rtxEnabled = rtxEnabled ? 1 : 0;
    
    // New advanced fields
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    
    ubo.mousePos = glm::vec2(
        static_cast<float>(mouseX) / static_cast<float>(windowWidth),
        1.0f - static_cast<float>(mouseY) / static_cast<float>(windowHeight)
    );
    
    ubo.resolution = glm::vec2(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
    ubo.glowIntensity = 1.0f + sin(totalTime * 3.0f) * 0.3f; // Animated glow
    ubo.frameCount = static_cast<int>(frameCount);
    ubo.maxBounces = maxBounces;
    ubo.samplesPerPixel = samplesPerPixel;
    
    // Set BGR format flag for shader correction
    ubo.isBGRFormat = (swapChainImageFormat == VK_FORMAT_B8G8R8A8_SRGB || 
                       swapChainImageFormat == VK_FORMAT_B8G8R8A8_UNORM) ? 1 : 0;
    
    // PERFORMANCE: Reduced RTX parameters for Global Illumination
    ubo.maxBounces = 1;  // REDUCED: Only 1 bounce for performance
    ubo.samplesPerPixel = 1; // REDUCED: 1 sample per pixel
    
    // Dynamic RTX parameters based on animation mode (REDUCED)
    if (currentAnimationMode == AnimationMode::QUANTUM) {
        ubo.maxBounces = 1; // REDUCED from 5
        ubo.samplesPerPixel = 1; // REDUCED from 8
        ubo.glowIntensity = 2.0f;
    } else if (currentAnimationMode == AnimationMode::PARTY) {
        ubo.maxBounces = 1; // REDUCED from 2
        ubo.samplesPerPixel = 1; // REDUCED from 6
        ubo.glowIntensity = 3.0f + sin(totalTime * 10.0f) * 0.5f;
    }
    
    void* data;
    vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
}

void ClippyRTXApp::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    std::cout << "Starting recordCommandBuffer..." << std::endl;
    
    // Check if command buffer is valid
    if (commandBuffer == VK_NULL_HANDLE) {
        std::cout << "ERROR: Command buffer is null!" << std::endl;
        return;
    }
    std::cout << "Command buffer is valid (not null)" << std::endl;
    
    // Don't reset - use ONE_TIME_SUBMIT instead
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    std::cout << "About to begin command buffer..." << std::endl;
    
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    
    std::cout << "Command buffer begin successful!" << std::endl;
    
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;
    
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{1.0f, 0.0f, 0.0f, 1.0f}}; // Bright red to confirm render pipeline works
    clearValues[1].depthStencil = {1.0f, 0};
    
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Debug: Print render area
    static bool debugRenderArea = false;
    if (!debugRenderArea) {
        std::cout << "Render area: " << renderPassInfo.renderArea.extent.width 
                  << "x" << renderPassInfo.renderArea.extent.height << std::endl;
        debugRenderArea = true;
    }
    
    // Use ray tracing if enabled and available
    if (rtxEnabled && rayTracingPipeline) {
        // Ray tracing rendering would go here
        // For now, fall back to rasterization
    }
    
    std::cout << "About to bind graphics pipeline..." << std::endl;
    
    // Rasterization rendering
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, 
                           &descriptorSets[currentFrame], 0, nullptr);
    
    // Debug output
    static bool debugPrinted = false;
    if (!debugPrinted) {
        std::cout << "Drawing " << indices.size() << " indices..." << std::endl;
        debugPrinted = true;
    }
    
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    
    // Render UI overlay - disabled for debugging
    // renderUI(commandBuffer);
    
    vkCmdEndRenderPass(commandBuffer);
    
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

// Vulkan setup methods implementation
void ClippyRTXApp::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Clippy RTX";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Clippy Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;
    
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }
    
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

bool ClippyRTXApp::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        
        if (!layerFound) {
            return false;
        }
    }
    
    return true;
}

std::vector<const char*> ClippyRTXApp::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    return extensions;
}

void ClippyRTXApp::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    
    // Enhanced debug verbosity for ray tracing debugging
    #ifdef ENABLE_RAY_TRACING_DEBUG
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    std::cout << "🔧 Enhanced Ray Tracing Debug Mode ENABLED" << std::endl;
    #else
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    #endif
    
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = VulkanHelpers::debugCallback;
}

void ClippyRTXApp::setupDebugMessenger() {
    if (!enableValidationLayers) return;
    
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    
    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void ClippyRTXApp::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void ClippyRTXApp::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    
    for (const auto& device : devices) {
        if (VulkanHelpers::isDeviceSuitable(device, surface, deviceExtensions)) {
            physicalDevice = device;
            msaaSamples = VulkanHelpers::getMaxUsableSampleCount(physicalDevice);
            break;
        }
    }
    
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void ClippyRTXApp::createLogicalDevice() {
    QueueFamilyIndices indices = VulkanHelpers::findQueueFamilies(physicalDevice, surface);
    
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE;
    
    // Ray tracing features
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures{};
    rtFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rtFeatures.rayTracingPipeline = VK_TRUE;
    
    VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{};
    asFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    asFeatures.accelerationStructure = VK_TRUE;
    asFeatures.pNext = &rtFeatures;
    
    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR bufferDeviceAddressFeatures{};
    bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    bufferDeviceAddressFeatures.pNext = &asFeatures;
    
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &bufferDeviceAddressFeatures;
    
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    
    createInfo.pEnabledFeatures = &deviceFeatures;
    
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

// Placeholder implementations for remaining methods
void ClippyRTXApp::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = VulkanHelpers::querySwapChainSupport(physicalDevice, surface);
    
    // FIXED: Prefer RGB over BGR for correct color display across platforms
    VkSurfaceFormatKHR surfaceFormat = swapChainSupport.formats[0]; // fallback
    
    // PRIORITY 1: Look for RGB SRGB (ideal for cross-platform)
    for (const auto& availableFormat : swapChainSupport.formats) {
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            std::cout << "✅ Selected RGB format (cross-platform compatible)" << std::endl;
            break;
        }
    }
    
    // PRIORITY 2: Look for RGB UNORM if SRGB not available  
    for (const auto& availableFormat : swapChainSupport.formats) {
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            std::cout << "✅ Selected RGB UNORM format" << std::endl;
            break;
        }
    }
    
    // FALLBACK: Use BGR only if no RGB available (Linux X11 compatibility)
    bool foundRGB = (surfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB || 
                     surfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM);
    if (!foundRGB) {
        for (const auto& availableFormat : swapChainSupport.formats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                surfaceFormat = availableFormat;
                std::cout << "⚠️  Fallback to BGR format (will need color correction)" << std::endl;
                break;
            }
        }
    }
    
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availablePresentMode : swapChainSupport.presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = availablePresentMode;
        }
    }
    
    VkExtent2D extent = swapChainSupport.capabilities.currentExtent;
    if (extent.width == UINT32_MAX) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        
        extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        
        extent.width = std::clamp(extent.width, 
                                 swapChainSupport.capabilities.minImageExtent.width, 
                                 swapChainSupport.capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, 
                                  swapChainSupport.capabilities.minImageExtent.height, 
                                  swapChainSupport.capabilities.maxImageExtent.height);
    }
    
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && 
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    QueueFamilyIndices indices = VulkanHelpers::findQueueFamilies(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
    
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
    
    // DEBUG: Show what format we're using
    std::cout << "🎨 Swapchain format: " << swapChainImageFormat << std::endl;
    if (swapChainImageFormat == VK_FORMAT_B8G8R8A8_SRGB || swapChainImageFormat == VK_FORMAT_B8G8R8A8_UNORM) {
        std::cout << "   -> BGR format detected - will need color correction in shaders" << std::endl;
    } else if (swapChainImageFormat == VK_FORMAT_R8G8B8A8_SRGB || swapChainImageFormat == VK_FORMAT_R8G8B8A8_UNORM) {
        std::cout << "   -> RGB format detected - colors will display correctly" << std::endl;
    }
}

void ClippyRTXApp::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = VulkanHelpers::createImageView(device, swapChainImages[i], 
                                                               swapChainImageFormat, 
                                                               VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void ClippyRTXApp::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VulkanHelpers::findDepthFormat(physicalDevice);
    depthAttachment.samples = msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;
    
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}
// All Vulkan pipeline implementations are now in VulkanPipelineImplementations.cpp

// UI System Implementation
void ClippyRTXApp::setupUI() {
    clippyUI = std::make_unique<ClippyUI>(device, renderPass, descriptorPool, 
                                         swapChainExtent.width, swapChainExtent.height);
    
    // Initialize with welcome message
    clippyUI->showMessage(ClippyUI::MessageType::GREETING, "¡Hola! Soy Clippy RTX");
}

void ClippyRTXApp::updateUI() {
    if (!clippyUI) return;
    
    // Update UI based on current animation mode
    switch(currentAnimationMode) {
        case AnimationMode::EXCITED:
            clippyUI->showMessage(ClippyUI::MessageType::EXCITED, 
                "¡Esto está genial! ¡Mira este RTX!");
            break;
        case AnimationMode::HELPING:
            clippyUI->showMessage(ClippyUI::MessageType::HELPFUL, 
                "¿Te puedo ayudar con algo? Tengo muchas funciones RTX");
            break;
        case AnimationMode::THINKING:
            clippyUI->showMessage(ClippyUI::MessageType::THOUGHTFUL, 
                "Hmm... calculando trazado de rayos...");
            break;
        case AnimationMode::QUANTUM:
            clippyUI->showMessage(ClippyUI::MessageType::TECHNICAL, 
                "Modo cuántico activado. Superposición de estados RTX");
            break;
        case AnimationMode::PARTY:
            clippyUI->showMessage(ClippyUI::MessageType::EXCITED, 
                "¡FIESTA RTX! ¡Mira estos efectos!");
            break;
        case AnimationMode::MATRIX:
            clippyUI->showMessage(ClippyUI::MessageType::TECHNICAL, 
                "Acceso a la Matrix RTX. Neo... es hora");
            break;
        case AnimationMode::IDLE:
        default:
            if (rtxEnabled) {
                clippyUI->showMessage(ClippyUI::MessageType::INFORMATIVE, 
                    "RTX ON - Ray Tracing activo");
            } else {
                clippyUI->showMessage(ClippyUI::MessageType::INFORMATIVE, 
                    "RTX OFF - Modo clásico");
            }
            break;
    }
    
    clippyUI->update(deltaTime, mouseX, mouseY, mousePressed);
}

void ClippyRTXApp::renderUI(VkCommandBuffer commandBuffer) {
    if (clippyUI) {
        clippyUI->render(commandBuffer, currentFrame);
    }
}

// Post Processing Implementation
void ClippyRTXApp::setupPostProcessing() {
    std::cout << "Initializing PostProcessing..." << std::endl;
    
    try {
        postProcessing = std::make_unique<PostProcessing>(device, physicalDevice, renderPass, swapChainExtent);
        
        // ULTRA PRO EFFECTS ACTIVATED! 🔥
        postProcessing->enableTonemap(false); // DISABLED TO DEBUG GOLD COLOR
        postProcessing->enableBloom(true);
        postProcessing->enableVignette(true);
        postProcessing->enableChromaticAberration(true);
        postProcessing->enableFilmGrain(true);
        
        // RTX-inspired ultra pro settings
        postProcessing->setExposure(1.3f);      // Enhanced exposure
        postProcessing->setGamma(2.2f);
        postProcessing->setContrast(1.15f);     // Increased contrast
        postProcessing->setSaturation(1.1f);    // Boosted saturation
        postProcessing->setBloomIntensity(0.4f); // Beautiful bloom
        postProcessing->setBloomRadius(1.2f);
        postProcessing->setVignetteStrength(0.3f); // Cinematic vignette
        postProcessing->setChromaticAberration(0.003f); // Subtle CA
        postProcessing->setFilmGrain(0.08f);    // Film grain texture
        
        std::cout << "PostProcessing initialized successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "PostProcessing failed to initialize: " << e.what() << std::endl;
        std::cout << "Continuing without post-processing effects..." << std::endl;
        postProcessing = nullptr;
    }
}

void ClippyRTXApp::updatePostProcessing() {
    if (!postProcessing) return;
    
    // Dynamic post-processing based on animation mode
    switch(currentAnimationMode) {
        case AnimationMode::EXCITED:
            postProcessing->setBloomIntensity(0.5f + sin(totalTime * 3.0f) * 0.2f);
            postProcessing->setSaturation(1.3f + sin(totalTime * 2.0f) * 0.1f);
            postProcessing->setChromaticAberration(0.003f + sin(totalTime * 4.0f) * 0.001f);
            break;
            
        case AnimationMode::QUANTUM:
            postProcessing->setBloomIntensity(0.8f);
            postProcessing->setVignetteStrength(0.3f);
            postProcessing->setChromaticAberration(0.005f);
            postProcessing->setFilmGrain(0.02f);
            break;
            
        case AnimationMode::PARTY:
            postProcessing->setBloomIntensity(1.0f + sin(totalTime * 5.0f) * 0.3f);
            postProcessing->setSaturation(1.5f + sin(totalTime * 3.0f) * 0.2f);
            postProcessing->setContrast(1.3f + sin(totalTime * 2.0f) * 0.1f);
            postProcessing->setChromaticAberration(0.004f + sin(totalTime * 6.0f) * 0.002f);
            break;
            
        case AnimationMode::MATRIX:
            postProcessing->setBloomIntensity(0.6f);
            postProcessing->setSaturation(0.7f);  // Desaturated for matrix look
            postProcessing->setVignetteStrength(0.8f);
            postProcessing->setChromaticAberration(0.001f);
            postProcessing->setFilmGrain(0.08f);
            break;
            
        case AnimationMode::HELPING:
            postProcessing->setBloomIntensity(0.25f);
            postProcessing->setSaturation(1.1f);
            postProcessing->setVignetteStrength(0.4f);
            postProcessing->setChromaticAberration(0.001f);
            break;
            
        case AnimationMode::THINKING:
            postProcessing->setBloomIntensity(0.2f + sin(totalTime * 1.0f) * 0.05f);
            postProcessing->setSaturation(0.9f);
            postProcessing->setVignetteStrength(0.6f);
            break;
            
        case AnimationMode::IDLE:
        default:
            postProcessing->setBloomIntensity(0.3f);
            postProcessing->setSaturation(1.05f);
            postProcessing->setVignetteStrength(0.5f);
            postProcessing->setChromaticAberration(0.002f);
            postProcessing->setFilmGrain(0.05f);
            break;
    }
    
    // RTX mode affects post-processing
    if (rtxEnabled) {
        postProcessing->setExposure(1.4f);
        postProcessing->setContrast(1.15f);
    } else {
        postProcessing->setExposure(1.0f);
        postProcessing->setContrast(1.0f);
    }
    
    postProcessing->updateUniforms(currentFrame, totalTime);
}

void ClippyRTXApp::cleanup() {
    cleanupSwapChain();
    
    clippyUI.reset();
    postProcessing.reset();
    rayTracingPipeline.reset();
    
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
    
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
    
    vkDestroyCommandPool(device, commandPool, nullptr);
    
    vkDestroyDevice(device, nullptr);
    
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    
    glfwDestroyWindow(window);
    glfwTerminate();
}