#include "ParametersManager.h"

// Global initialization function to load parameters
namespace {
    struct ParametersInitializer {
        ParametersInitializer() {
            try {
                ParametersManager::getInstance().loadParameters();
                // std::cout << "Parameters loaded successfully from PARAMETERS_DIR" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to load some parameters: " << e.what() << std::endl;
            }
        }
    };
    
    // This will be called when the library is loaded
    static ParametersInitializer init;
}