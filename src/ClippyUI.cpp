#include "ClippyUI.h"
#include <iostream>

ClippyUI::ClippyUI(VkDevice device, VkRenderPass renderPass, VkDescriptorPool descriptorPool, 
                   uint32_t width, uint32_t height) 
    : device(device), renderPass(renderPass), descriptorPool(descriptorPool), 
      width(width), height(height), rng(std::random_device{}()), dist(0, 100) {
    
    // Initialize UI state
    state.currentMessageType = MessageType::GREETING;
    state.currentMessage = "¡Hola! Soy Clippy RTX";
    state.messageTimer = 0.0f;
    state.animationPhase = 0.0f;
    state.isVisible = true;
    
    // TODO: Initialize Vulkan resources for UI rendering
    std::cout << "ClippyUI initialized with resolution " << width << "x" << height << std::endl;
}

ClippyUI::~ClippyUI() {
    cleanup();
}

void ClippyUI::showMessage(MessageType type, const std::string& message) {
    state.currentMessageType = type;
    state.currentMessage = message;
    state.messageTimer = 0.0f;
    state.isVisible = true;
    
    // Log message for debugging
    std::cout << "[ClippyUI] " << message << std::endl;
}

void ClippyUI::update(float deltaTime, double mouseX, double mouseY, bool mousePressed) {
    state.messageTimer += deltaTime;
    state.animationPhase += deltaTime * 2.0f;
    
    // Simple animation updates
    if (state.messageTimer > messageLifetime) {
        state.isVisible = false;
    }
    
    updateAnimations(deltaTime);
}

void ClippyUI::render(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
    if (!state.isVisible) return;
    
    // For now, just a placeholder - no actual Vulkan rendering
    // TODO: Implement proper UI rendering with Vulkan
    renderMessageBubble(commandBuffer);
    renderClippyCharacter(commandBuffer);
}

void ClippyUI::cleanup() {
    // TODO: Clean up Vulkan resources
    std::cout << "ClippyUI cleaned up" << std::endl;
}

void ClippyUI::updateAnimations(float deltaTime) {
    // Simple bobbing animation
    bubbleAlpha = 0.8f + 0.2f * sin(state.animationPhase);
    textScale = 1.0f + 0.1f * sin(state.animationPhase * 1.5f);
}

void ClippyUI::renderMessageBubble(VkCommandBuffer commandBuffer) {
    // Placeholder for message bubble rendering
    // In a full implementation, this would render a speech bubble with text
}

void ClippyUI::renderClippyCharacter(VkCommandBuffer commandBuffer) {
    // Placeholder for Clippy character rendering
    // In a full implementation, this would render the 3D Clippy character
}