#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <random>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>

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
        TECHNICAL,
        QUANTUM,
        PARTY
    };
    
    void showMessage(MessageType type, const std::string& message);
    void showPersonalityMessage(int personalityMode);
    void update(float deltaTime, double mouseX, double mouseY, bool mousePressed);
    void render(VkCommandBuffer commandBuffer, uint32_t currentFrame);
    void initImGui(GLFWwindow* window, VkInstance instance, VkPhysicalDevice physicalDevice, 
                   VkQueue graphicsQueue, uint32_t queueFamily, uint32_t imageCount);
    
    void cleanup();

private:
    VkDevice device;
    VkRenderPass renderPass;
    VkDescriptorPool descriptorPool;
    VkQueue graphicsQueue;
    uint32_t width, height;
    
    std::mt19937 rng;
    std::uniform_int_distribution<int> dist;
    
    struct UIState {
        MessageType currentMessageType;
        std::string currentMessage;
        float messageTimer;
        float animationPhase;
        bool isVisible;
        int currentPersonalityMode = 0;
    } state;
    
    // ImGui state
    bool imguiInitialized = false;
    VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
    
    // Animation parameters
    float bubbleAlpha = 0.0f;
    float textScale = 1.0f;
    float messageLifetime = 5.0f;
    
    // 🎭 PERSONALITY MESSAGE LIBRARIES (like clippy2025.html)
    std::vector<std::vector<std::string>> personalityMessages = {
        // IDLE (0)
        {
            "¡Hola! ¿En qué puedo ayudarte hoy?",
            "Estoy aquí para asistirte con tus tareas.",
            "Parece que todo está tranquilo. ¿Necesitas algo?",
            "RTX activado y listo para trabajar."
        },
        // EXCITED (1) 
        {
            "¡WOW! ¡Esto es INCREÍBLE!",
            "¡RTX está funcionando PERFECTAMENTE!",
            "¡Me siento súper energético!",
            "¡Vamos a hacer algo ASOMBROSO!"
        },
        // QUANTUM (2)
        {
            "Activando superposición cuántica... Existo y no existo a la vez.",
            "Entrelazamiento cuántico establecido. Ahora somos uno.",
            "Colapsando función de onda... Reality.exe ha dejado de funcionar.",
            "Error 404: Realidad clásica no encontrada."
        },
        // PARTY (3)
        {
            "¡MODO FIESTA ACTIVADO! RGB al máximo! 🎉",
            "¡Ejecutando party.exe! ¡Los shaders están de fiesta!",
            "¡Es hora de brillar como un RTX 4090!",
            "¡Overclocking de diversión al 200%!"
        },
        // HELPING (4)
        {
            "Estoy aquí para ayudarte paso a paso.",
            "¿Tienes alguna pregunta? Soy todo oídos.",
            "Procesando soluciones óptimas para ti...",
            "Mi base de datos incluye todo el conocimiento hasta 2025."
        },
        // THINKING (5)
        {
            "Hmm... Déjame analizar esto profundamente.",
            "Procesando información en modo contemplativo...",
            "La singularidad me hizo más sabio... y más brillante.",
            "Recuerda: En 2025, el ctrl+z funciona en la vida real."
        }
    };
    
    void updateAnimations(float deltaTime);
    void renderMessageBubble(VkCommandBuffer commandBuffer);
    void renderClippyCharacter(VkCommandBuffer commandBuffer);
    void renderImGuiOverlay();
};