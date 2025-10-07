#ifndef PARAMETERS_MANAGER_H
#define PARAMETERS_MANAGER_H

#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <cmath>

class ParametersManager {
public:
    static ParametersManager& getInstance() {
        static ParametersManager instance;
        return instance;
    }

    void loadParameters() {
        std::string paramDir = getParametersDir();
        loadParameterFile(paramDir + "/geometry.dat");
        loadParameterFile(paramDir + "/timing.dat");
        loadParameterFile(paramDir + "/conversion.dat");
        loadParameterFile(paramDir + "/detector.dat");
        loadParameterFile(paramDir + "/analysis.dat");
        
        // Calculate derived parameters
        calculateDerivedParameters();
    }

    double getDouble(const std::string& key) const {
        auto it = parameters_.find(key);
        if (it != parameters_.end()) {
            return std::stod(it->second);
        }
        throw std::runtime_error("Parameter not found: " + key);
    }

    int getInt(const std::string& key) const {
        auto it = parameters_.find(key);
        if (it != parameters_.end()) {
            return std::stoi(it->second);
        }
        throw std::runtime_error("Parameter not found: " + key);
    }

    std::string getString(const std::string& key) const {
        auto it = parameters_.find(key);
        if (it != parameters_.end()) {
            return it->second;
        }
        throw std::runtime_error("Parameter not found: " + key);
    }

    bool hasParameter(const std::string& key) const {
        return parameters_.find(key) != parameters_.end();
    }

private:
    std::map<std::string, std::string> parameters_;

    ParametersManager() = default;
    
    std::string getParametersDir() const {
        const char* paramDir = std::getenv("PARAMETERS_DIR");
        if (paramDir == nullptr) {
            // Fallback to relative path from source directory
            return "parameters";
        }
        return std::string(paramDir);
    }

    void loadParameterFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open parameter file: " << filename << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            parseLine(line);
        }
        file.close();
    }

    void parseLine(const std::string& line) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '*' || line.find_first_not_of(" \t") == std::string::npos) {
            return;
        }

        // Look for pattern: < key = value >
        size_t start = line.find('<');
        size_t end = line.find('>');
        
        if (start != std::string::npos && end != std::string::npos && start < end) {
            std::string content = line.substr(start + 1, end - start - 1);
            
            size_t equalPos = content.find('=');
            if (equalPos != std::string::npos) {
                std::string key = trim(content.substr(0, equalPos));
                std::string value = trim(content.substr(equalPos + 1));
                
                parameters_[key] = value;
            }
        }
    }

    std::string trim(const std::string& str) const {
        size_t start = str.find_first_not_of(" \t");
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(" \t");
        return str.substr(start, end - start + 1);
    }

    void calculateDerivedParameters() {
        // Calculate derived geometry parameters
        if (hasParameter("geometry.apa_angle_deg") && hasParameter("geometry.wire_pitch_induction_diagonal_cm")) {
            double angle_deg = getDouble("geometry.apa_angle_deg");
            double angle_rad = angle_deg * M_PI / 180.0;
            double diagonal_pitch = getDouble("geometry.wire_pitch_induction_diagonal_cm");
            double induction_pitch = diagonal_pitch / sin(angle_rad);
            
            parameters_["geometry.wire_pitch_induction_cm"] = std::to_string(induction_pitch);
            parameters_["geometry.apa_angular_coeff"] = std::to_string(tan(angle_rad));
        }

        // Calculate derived timing parameters
        if (hasParameter("timing.clock_tick_ns") && hasParameter("timing.conversion_tdc_to_tpc")) {
            double clock_tick = getDouble("timing.clock_tick_ns");
            int conversion = getInt("timing.conversion_tdc_to_tpc");
            double tpc_sample_length = clock_tick * conversion;
            
            parameters_["timing.tpc_sample_length_ns"] = std::to_string(tpc_sample_length);
        }
    }
};

// Convenience macros for accessing parameters
#define PARAM_MGR ParametersManager::getInstance()
#define GET_PARAM_DOUBLE(key) PARAM_MGR.getDouble(key)
#define GET_PARAM_INT(key) PARAM_MGR.getInt(key)
#define GET_PARAM_STRING(key) PARAM_MGR.getString(key)

#endif // PARAMETERS_MANAGER_H