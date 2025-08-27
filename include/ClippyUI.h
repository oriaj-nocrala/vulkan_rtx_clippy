#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <random>

class ClippyUI {
public:
    ClippyUI(VkDevice device, VkRenderPass renderPass, VkDescriptorPool descriptorPool, 
             uint32_t width, uint32_t height);
    ~ClippyUI();

    enum class MessageType {
        INFORMATIVE,
        HELPFUL, 
        EXCITED,
        THOUGHTFUL,
        WARNING,
        ERROR,
        GREETING,
        TECHNICAL
    };
    
    void showMessage(MessageType type, const std::string& message);
    void update(float deltaTime, double mouseX, double mouseY, bool mousePressed);
    void render(VkCommandBuffer commandBuffer, uint32_t currentFrame);
    
    void cleanup();

private:
    VkDevice device;
    VkRenderPass renderPass;
    VkDescriptorPool descriptorPool;
    uint32_t width, height;
    
    std::mt19937 rng;
    std::uniform_int_distribution<int> dist;
    
    struct UIState {
        MessageType currentMessageType;
        std::string currentMessage;
        float messageTimer;
        float animationPhase;
        bool isVisible;
    } state;
    
    // Animation parameters
    float bubbleAlpha = 0.0f;
    float textScale = 1.0f;
    float messageLifetime = 3.0f;
    
    void updateAnimations(float deltaTime);
    void renderMessageBubble(VkCommandBuffer commandBuffer);
    void renderClippyCharacter(VkCommandBuffer commandBuffer);
};