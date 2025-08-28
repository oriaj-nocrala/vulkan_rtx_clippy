#include "ClippyUI.h"
#include <iostream>
#include <algorithm>

ClippyUI::ClippyUI(VkDevice device, VkRenderPass renderPass, VkDescriptorPool descriptorPool, 
                   uint32_t width, uint32_t height) 
    : device(device), renderPass(renderPass), descriptorPool(descriptorPool), 
      width(width), height(height), rng(std::random_device{}()), dist(0, 100) {
    
    // Initialize UI state
    state.currentMessageType = MessageType::GREETING;
    state.currentMessage = "¡Hola! Soy Clippy RTX con personalidad";
    state.messageTimer = 0.0f;
    state.animationPhase = 0.0f;
    state.isVisible = true;
    state.currentPersonalityMode = 0;
    
    std::cout << "ClippyUI initialized with resolution " << width << "x" << height << std::endl;
}

ClippyUI::~ClippyUI() {
    cleanup();
}

void ClippyUI::initImGui(GLFWwindow* window, VkInstance instance, VkPhysicalDevice physicalDevice, 
                         VkQueue graphicsQueue, uint32_t queueFamily, uint32_t imageCount) {
    // Store the graphics queue for later use
    this->graphicsQueue = graphicsQueue;
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup ImGui style - futuristic theme
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    style.FrameRounding = 5.0f;
    style.WindowBorderSize = 2.0f;
    
    // Golden/cyan color scheme for Clippy
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
    colors[ImGuiCol_Border] = ImVec4(1.0f, 0.843f, 0.0f, 0.8f); // Gold border
    colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.5f, 0.8f, 0.8f);  // Cyan title
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.7f, 1.0f, 1.0f);
    
    // Create ImGui-specific descriptor pool with COMBINED_IMAGE_SAMPLER support
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 }
    };
    
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    
    if (vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiDescriptorPool) != VK_SUCCESS) {
        std::cout << "[ClippyUI] Failed to create ImGui descriptor pool!" << std::endl;
        return;
    }
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physicalDevice;
    init_info.Device = device;
    init_info.QueueFamily = queueFamily;
    init_info.Queue = graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = imguiDescriptorPool;  // Use ImGui-specific pool
    init_info.Subpass = 0;
    init_info.MinImageCount = imageCount;
    init_info.ImageCount = imageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    
    ImGui_ImplVulkan_Init(&init_info, renderPass);
    
    // Upload fonts to GPU - this needs to be done after ImGui_ImplVulkan_Init
    // Create a temporary command buffer to upload fonts
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamily;
    
    VkCommandPool tempCommandPool;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &tempCommandPool) == VK_SUCCESS) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = tempCommandPool;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer tempCommandBuffer;
        if (vkAllocateCommandBuffers(device, &allocInfo, &tempCommandBuffer) == VK_SUCCESS) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            
            vkBeginCommandBuffer(tempCommandBuffer, &beginInfo);
            ImGui_ImplVulkan_CreateFontsTexture();
            vkEndCommandBuffer(tempCommandBuffer);
            
            // Submit the command buffer
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &tempCommandBuffer;
            
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphicsQueue);
        }
        
        vkDestroyCommandPool(device, tempCommandPool, nullptr);
    }
    
    // Clear font data from CPU memory
    ImGui_ImplVulkan_DestroyFontsTexture();
    
    imguiInitialized = true;
    std::cout << "[ClippyUI] ImGui initialized successfully with fonts uploaded" << std::endl;
}

void ClippyUI::showMessage(MessageType type, const std::string& message) {
    state.currentMessageType = type;
    state.currentMessage = message;
    state.messageTimer = 0.0f;
    state.isVisible = true;
    
    // Log message for debugging
    std::cout << "[ClippyUI] " << message << std::endl;
}

void ClippyUI::showPersonalityMessage(int personalityMode) {
    if (personalityMode < 0 || personalityMode >= personalityMessages.size()) return;
    
    state.currentPersonalityMode = personalityMode;
    const auto& messages = personalityMessages[personalityMode];
    if (messages.empty()) return;
    
    // Pick random message for this personality mode
    int randomIndex = dist(rng) % messages.size();
    showMessage(static_cast<MessageType>(personalityMode), messages[randomIndex]);
}

void ClippyUI::update(float deltaTime, double mouseX, double mouseY, bool mousePressed) {
    state.messageTimer += deltaTime;
    state.animationPhase += deltaTime * 2.0f;
    
    // Messages last longer now
    if (state.messageTimer > messageLifetime) {
        state.isVisible = false;
    }
    
    updateAnimations(deltaTime);
}

void ClippyUI::render(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
    if (imguiInitialized) {
        // Start new ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Render ImGui content
        renderImGuiOverlay();
        
        // End frame and render
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }
    renderMessageBubble(commandBuffer);
    renderClippyCharacter(commandBuffer);
}

void ClippyUI::cleanup() {
    if (imguiInitialized) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        imguiInitialized = false;
    }
    
    if (imguiDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);
        imguiDescriptorPool = VK_NULL_HANDLE;
    }
    
    std::cout << "ClippyUI cleaned up" << std::endl;
}

void ClippyUI::updateAnimations(float deltaTime) {
    // Simple bobbing animation
    bubbleAlpha = 0.8f + 0.2f * sin(state.animationPhase);
    textScale = 1.0f + 0.1f * sin(state.animationPhase * 1.5f);
}

void ClippyUI::renderImGuiOverlay() {
    // 🎭 CLIPPY SPEECH BUBBLE WINDOW
    if (state.isVisible) {
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_Always);
        
        // Window flags for speech bubble
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | 
                                ImGuiWindowFlags_NoMove | 
                                ImGuiWindowFlags_NoCollapse |
                                ImGuiWindowFlags_AlwaysAutoResize;
        
        // Dynamic title based on personality mode
        std::string windowTitle;
        switch (state.currentPersonalityMode) {
            case 0: windowTitle = "💬 Clippy (Modo IDLE)"; break;
            case 1: windowTitle = "⚡ Clippy (Modo EXCITED)"; break;
            case 2: windowTitle = "🔮 Clippy (Modo QUANTUM)"; break;
            case 3: windowTitle = "🎉 Clippy (Modo PARTY)"; break;
            case 4: windowTitle = "🤝 Clippy (Modo HELPING)"; break;
            case 5: windowTitle = "🤔 Clippy (Modo THINKING)"; break;
            default: windowTitle = "💬 Clippy RTX"; break;
        }
        
        ImGui::Begin(windowTitle.c_str(), nullptr, flags);
        
        // Animated text size based on personality
        float textSize = 16.0f * textScale;
        
        // Color based on personality mode
        ImVec4 textColor;
        switch (state.currentPersonalityMode) {
            case 0: textColor = ImVec4(1.0f, 0.843f, 0.0f, 1.0f); break; // Gold
            case 1: textColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break;   // Yellow
            case 2: textColor = ImVec4(0.0f, 1.0f, 1.0f, 1.0f); break;   // Cyan
            case 3: 
                // Rainbow effect for party mode
                {
                    float hue = fmod(state.animationPhase * 0.5f, 6.28318f);
                    textColor = ImVec4(
                        0.5f + 0.5f * sin(hue),
                        0.5f + 0.5f * sin(hue + 2.09f),
                        0.5f + 0.5f * sin(hue + 4.19f),
                        1.0f
                    );
                }
                break;
            case 4: textColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break;   // Green
            case 5: textColor = ImVec4(0.5f, 0.0f, 1.0f, 1.0f); break;   // Purple
            default: textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
        }
        
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        ImGui::TextWrapped("%s", state.currentMessage.c_str());
        ImGui::PopStyleColor();
        
        // Timer bar
        float progress = 1.0f - (state.messageTimer / messageLifetime);
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "");
        
        ImGui::End();
    }
    
    // 🎮 CONTROLS WINDOW
    ImGui::SetNextWindowPos(ImVec2(width - 300, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(280, 200), ImGuiCond_Always);
    
    ImGuiWindowFlags controlFlags = ImGuiWindowFlags_NoResize | 
                                   ImGuiWindowFlags_NoMove | 
                                   ImGuiWindowFlags_NoCollapse;
    
    ImGui::Begin("🎭 Controles de Clippy", nullptr, controlFlags);
    
    ImGui::Text("Modo Actual: %d", state.currentPersonalityMode);
    
    if (ImGui::Button("SALUDAR (1)")) {
        showPersonalityMessage(0);
    }
    if (ImGui::Button("EMOCIONADO (2)")) {
        showPersonalityMessage(1);
    }
    if (ImGui::Button("MODO QUANTUM (3)")) {
        showPersonalityMessage(2);
    }
    if (ImGui::Button("PARTY MODE (4)")) {
        showPersonalityMessage(3);
    }
    if (ImGui::Button("AYUDA (5)")) {
        showPersonalityMessage(4);
    }
    if (ImGui::Button("SABIDURÍA (6)")) {
        showPersonalityMessage(5);
    }
    
    ImGui::Separator();
    ImGui::Text("Controles de teclado:");
    ImGui::Text("1-6: Cambiar personalidad");
    ImGui::Text("SPACE: Toggle RTX");
    ImGui::Text("ESC: Salir");
    
    ImGui::End();
}

void ClippyUI::renderMessageBubble(VkCommandBuffer commandBuffer) {
    // ImGui handles this now
}

void ClippyUI::renderClippyCharacter(VkCommandBuffer commandBuffer) {
    // The 3D Clippy is rendered by RTX pipeline
}