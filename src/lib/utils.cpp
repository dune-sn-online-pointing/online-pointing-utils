#include "global.h"

LoggerInit([]{Logger::getUserHeader() << "[utils]";});





std::vector<std::string> find_input_files(const nlohmann::json& j, const std::string& file_suffix) {
    std::vector<std::string> filenames;
    filenames.reserve(64);
    
    // Helper functions
    auto has_suffix = [&file_suffix](const std::string& path) {
        std::string basename = path.substr(path.find_last_of("/\\") + 1);
        return basename.length() >= file_suffix.length() && 
               basename.substr(basename.length() - file_suffix.length()) == file_suffix;
    };
    
    auto file_exists = [](const std::string& path) { 
        std::ifstream f(path); 
        return f.good(); 
    };

    // Priority 1: JSON inputFile (single root)
    try {
        if (j.contains("inputFile")) {
            auto single = j.at("inputFile").get<std::string>();
            if (!single.empty()) {
                LogInfo << "JSON inputFile: " << single << std::endl;
                if (!file_exists(single)) {
                    LogError << "inputFile does not exist: " << single << std::endl;
                    return filenames; // Return empty vector
                }
                if (!has_suffix(single)) {
                    LogError << "inputFile does not have suffix " << file_suffix << ": " << single << std::endl;
                    return filenames; // Return empty vector
                }
                filenames.push_back(single);
                return filenames;
            }
        }
    } catch (...) { /* ignore */ }

    // Priority 2: JSON inputFolder (gather files with suffix)
    try {
        if (j.contains("inputFolder")) {
            auto folder = j.at("inputFolder").get<std::string>();
            if (!folder.empty()) {
                LogInfo << "JSON inputFolder: " << folder << std::endl;
                std::error_code ec;
                for (auto const& entry : std::filesystem::directory_iterator(folder, ec)) {
                    if (ec) break;
                    if (!entry.is_regular_file()) continue;
                    std::string p = entry.path().string();
                    if (!has_suffix(p)) continue;
                    if (!file_exists(p)) continue;
                    filenames.push_back(p);
                }
                std::sort(filenames.begin(), filenames.end());
                if (!filenames.empty()) return filenames;
            }
        }
    } catch (...) { /* ignore */ }

    // Priority 3: JSON inputList (array of files)
    try {
        if (j.contains("inputList") && j.at("inputList").is_array()) {
            LogInfo << "JSON inputList provided." << std::endl;
            for (const auto& el : j.at("inputList")) {
                if (!el.is_string()) continue;
                std::string p = el.get<std::string>();
                if (p.empty()) continue;
                if (!file_exists(p)) { 
                    LogWarning << "Skipping (missing): " << p << std::endl; 
                    continue; 
                }
                if (!has_suffix(p)) { 
                    LogWarning << "Skipping (not " << file_suffix << "): " << p << std::endl; 
                    continue; 
                }
                filenames.push_back(p);
            }
            if (!filenames.empty()) return filenames;
        }
    } catch (...) { /* ignore */ }

    // Priority 4: JSON inputListFile (file with list)
    try {
        if (j.contains("inputListFile")) {
            auto list_file = j.at("inputListFile").get<std::string>();
            if (!list_file.empty()) {
                LogInfo << "JSON inputListFile: " << list_file << std::endl;
                std::ifstream infile(list_file);
                if (!infile.good()) {
                    LogError << "Cannot open inputListFile: " << list_file << std::endl;
                    return filenames; // Return empty vector
                }
                std::string line;
                while (std::getline(infile, line)) {
                    if (line.size() >= 3 && line.substr(0,3) == "###") break;
                    if (line.empty() || line[0] == '#') continue;
                    if (!file_exists(line)) { 
                        LogWarning << "Skipping (missing): " << line << std::endl; 
                        continue; 
                    }
                    if (!has_suffix(line)) { 
                        LogWarning << "Skipping (not " << file_suffix << "): " << line << std::endl; 
                        continue; 
                    }
                    filenames.push_back(line);
                }
            }
        }
    } catch (...) { /* ignore */ }

    return filenames;
}

// Overloaded version that accepts multiple file suffixes
std::vector<std::string> find_input_files(const nlohmann::json& j, const std::vector<std::string>& file_suffixes) {
    std::vector<std::string> filenames;
    filenames.reserve(64);

    if (file_suffixes.empty()) {
        LogWarning << "No file suffixes provided." << std::endl;
        return filenames;
    }

    // Helper function to check if file has any of the required suffixes
    auto has_any_suffix = [&file_suffixes](const std::string& filename) -> bool {
        for (const auto& suffix : file_suffixes) {
            if (filename.size() >= suffix.size() && 
                filename.substr(filename.size() - suffix.size()) == suffix) {
                return true;
            }
        }
        return false;
    };

    auto file_exists = [](const std::string& filename) -> bool {
        std::ifstream file(filename);
        return file.good();
    };

    try {
        // Priority 1: inputFile (single file)
        if (j.contains("inputFile") && !j["inputFile"].get<std::string>().empty()) {
            std::string file = j["inputFile"].get<std::string>();
            LogInfo << "JSON inputFile: " << file << std::endl;
            if (file_exists(file) && has_any_suffix(file)) {
                filenames.push_back(file);
            } else {
                LogWarning << "inputFile not found or doesn't match suffixes: " << file << std::endl;
            }
        }

        // Priority 2: inputFolder (with optional inputList)
        if (filenames.empty() && j.contains("inputFolder") && !j["inputFolder"].get<std::string>().empty()) {
            std::string folder = j["inputFolder"].get<std::string>();
            LogInfo << "JSON inputFolder: " << folder << std::endl;

            if (j.contains("inputList") && j["inputList"].is_array() && !j["inputList"].empty()) {
                LogInfo << "Using inputList with " << j["inputList"].size() << " entries" << std::endl;
                for (const auto& item : j["inputList"]) {
                    std::string filename = item.get<std::string>();
                    std::string filepath = folder + "/" + filename;
                    if (!file_exists(filepath)) { 
                        LogWarning << "Skipping (missing): " << filepath << std::endl; 
                        continue; 
                    }
                    if (!has_any_suffix(filepath)) { 
                        LogWarning << "Skipping (wrong suffix): " << filepath << std::endl; 
                        continue; 
                    }
                    filenames.push_back(filepath);
                }
            } else {
                LogInfo << "Scanning folder for files with suffixes..." << std::endl;
                try {
                    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
                        if (entry.is_regular_file()) {
                            std::string filepath = entry.path().string();
                            if (has_any_suffix(filepath)) {
                                filenames.push_back(filepath);
                            }
                        }
                    }
                    std::sort(filenames.begin(), filenames.end());
                    LogInfo << "Found " << filenames.size() << " files in folder" << std::endl;
                } catch (const std::filesystem::filesystem_error& e) {
                    LogWarning << "Error scanning directory " << folder << ": " << e.what() << std::endl;
                }
            }
        }

        // Priority 3: filename (single file)
        if (filenames.empty() && j.contains("filename") && !j["filename"].get<std::string>().empty()) {
            std::string file = j["filename"].get<std::string>();
            LogInfo << "JSON filename: " << file << std::endl;
            if (file_exists(file) && has_any_suffix(file)) {
                filenames.push_back(file);
            } else {
                LogWarning << "filename not found or doesn't match suffixes: " << file << std::endl;
            }
        }

        // Priority 4: filelist OR inputListFile (file with list of files)
        if (filenames.empty()) {
            std::string list_file;
            if (j.contains("filelist") && !j["filelist"].get<std::string>().empty()) {
                list_file = j["filelist"].get<std::string>();
                LogInfo << "JSON filelist: " << list_file << std::endl;
            } else if (j.contains("inputListFile") && !j["inputListFile"].get<std::string>().empty()) {
                list_file = j["inputListFile"].get<std::string>();
                LogInfo << "JSON inputListFile: " << list_file << std::endl;
            }

            if (!list_file.empty()) {
                std::ifstream infile(list_file);
                if (!infile.good()) {
                    LogError << "Cannot open list file: " << list_file << std::endl;
                    return filenames; // Return empty vector
                }
                std::string line;
                while (std::getline(infile, line)) {
                    if (line.size() >= 3 && line.substr(0,3) == "###") break;
                    if (line.empty() || line[0] == '#') continue;
                    if (!file_exists(line)) { 
                        LogWarning << "Skipping (missing): " << line << std::endl; 
                        continue; 
                    }
                    if (!has_any_suffix(line)) { 
                        LogWarning << "Skipping (wrong suffixes): " << line << std::endl; 
                        continue; 
                    }
                    filenames.push_back(line);
                }
            }
        }
    } catch (...) { /* ignore */ }

    return filenames;
}