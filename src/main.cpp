#include "ClippyRTXApp.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

int main() {
    ClippyRTXApp app;
    
    std::cout << "==================================" << std::endl;
    std::cout << "   Clippy RTX - Vulkan Ray Tracing" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Initializing Clippy RTX..." << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  SPACE - Toggle RTX On/Off" << std::endl;
    std::cout << "  ESC   - Exit application" << std::endl;
    std::cout << "==================================" << std::endl;
    
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << "Clippy RTX terminated successfully." << std::endl;
    return EXIT_SUCCESS;
}