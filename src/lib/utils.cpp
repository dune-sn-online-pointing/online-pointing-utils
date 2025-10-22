#include "global.h"

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

std::string toLower(std::string s){
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
  return s;
}

int toTPCticks(int tdcTicks) {
  return tdcTicks / get_conversion_tdc_to_tpc();
}

int toTDCticks(int tpcTicks) {
  return tpcTicks * get_conversion_tdc_to_tpc();
}

bool ensureDirectoryExists(const std::string& folder) {
    if (folder.empty()) {
        return true;
    }

    std::error_code ec;
    std::filesystem::create_directories(folder, ec);
    if (ec) {
        LogError << "Failed to ensure directory '" << folder << "' exists: " << ec.message() << std::endl;
        return false;
    }
    return true;
}


void bindBranch(TTree *tree, const char* name, void* address) {
    if (tree->SetBranchAddress(name, address) < 0) {
        LogWarning << "Failed to bind branch '" << name << "'" << std::endl;
    }
}

// std::vector<std::string> find_input_files(const nlohmann::json& j, const std::string& file_suffix) {
//     std::vector<std::string> filenames;
//     filenames.reserve(64);
    
//     // Helper functions
//     auto has_suffix = [&file_suffix](const std::string& path) {
//         std::string basename = path.substr(path.find_last_of("/\\") + 1);
//         if (file_suffix == "_clusters") {
//             // Accept any file containing '_clusters' and ending with '.root'
//             return basename.find("_clusters") != std::string::npos &&
//                    basename.size() >= 5 &&
//                    basename.substr(basename.size() - 5) == ".root";
//         } else {
//             return basename.length() >= file_suffix.length() && 
//                    basename.substr(basename.length() - file_suffix.length()) == file_suffix;
//         }
//     };
    
//     auto file_exists = [](const std::string& path) { 
//         std::ifstream f(path); 
//         return f.good(); 
//     };

//     // Priority 1: JSON inputFile (single root)
//     try {
//         if (j.contains("inputFile")) {
//             auto single = j.at("inputFile").get<std::string>();
//             if (!single.empty()) {
//                 LogInfo << "JSON inputFile: " << single << std::endl;
//                 if (!file_exists(single)) {
//                     LogError << "inputFile does not exist: " << single << std::endl;
//                     return filenames; // Return empty vector
//                 }
//                 if (!has_suffix(single)) {
//                     LogError << "inputFile does not have suffix " << file_suffix << ": " << single << std::endl;
//                     return filenames; // Return empty vector
//                 }
//                 filenames.push_back(single);
//                 std::sort(filenames.begin(), filenames.end());
//                 return filenames;
//             }
//         }
//     } catch (...) { /* ignore */ }

//     // Priority 2: JSON inputFolder (gather files with suffix)
//     try {
//         if (j.contains("inputFolder")) {
//             auto folder = j.at("inputFolder").get<std::string>();
//             if (!folder.empty()) {
//                 LogInfo << "JSON inputFolder: " << folder << std::endl;
//                 std::error_code ec;
//                 for (auto const& entry : std::filesystem::directory_iterator(folder, ec)) {
//                     if (ec) break;
//                     if (!entry.is_regular_file()) continue;
//                     std::string p = entry.path().string();
//                     if (!has_suffix(p)) continue;
//                     if (!file_exists(p)) continue;
//                     filenames.push_back(p);
//                 }
//                 std::sort(filenames.begin(), filenames.end());
//                 if (!filenames.empty()) return filenames;
//             }
//         }
//     } catch (...) { /* ignore */ }

//     // Priority 3: JSON inputList (array of files)
//     try {
//         if (j.contains("inputList") && j.at("inputList").is_array()) {
//             LogInfo << "JSON inputList provided." << std::endl;
//             for (const auto& el : j.at("inputList")) {
//                 if (!el.is_string()) continue;
//                 std::string p = el.get<std::string>();
//                 if (p.empty()) continue;
//                 if (!file_exists(p)) { 
//                     LogWarning << "Skipping (missing): " << p << std::endl; 
//                     continue; 
//                 }
//                 if (!has_suffix(p)) { 
//                     LogWarning << "Skipping (not " << file_suffix << "): " << p << std::endl; 
//                     continue; 
//                 }
//                 filenames.push_back(p);
//             }
//             if (!filenames.empty()) {
//                 std::sort(filenames.begin(), filenames.end());
//                 return filenames;
//             }
//         }
//     } catch (...) { /* ignore */ }

//     // Priority 4: JSON inputListFile (file with list)
//     try {
//         if (j.contains("inputListFile")) {
//             auto list_file = j.at("inputListFile").get<std::string>();
//             if (!list_file.empty()) {
//                 LogInfo << "JSON inputListFile: " << list_file << std::endl;
//                 std::ifstream infile(list_file);
//                 if (!infile.good()) {
//                     LogError << "Cannot open inputListFile: " << list_file << std::endl;
//                     return filenames; // Return empty vector
//                 }
//                 std::string line;
//                 while (std::getline(infile, line)) {
//                     if (line.size() >= 3 && line.substr(0,3) == "###") break;
//                     if (line.empty() || line[0] == '#') continue;
//                     if (!file_exists(line)) { 
//                         LogWarning << "Skipping (missing): " << line << std::endl; 
//                         continue; 
//                     }
//                     if (!has_suffix(line)) { 
//                         LogWarning << "Skipping (not " << file_suffix << "): " << line << std::endl; 
//                         continue; 
//                     }
//                     filenames.push_back(line);
//                 }
//                 if (!filenames.empty()) {
//                     std::sort(filenames.begin(), filenames.end());
//                 }
//             }
//         }
//     } catch (...) { /* ignore */ }

//     // After gathering and sorting files, apply skip_files if specified
//     if (!filenames.empty() && j.contains("skip_files")) {
//         try {
//             int skip_count = j.at("skip_files").get<int>();
//             if (skip_count > 0 && skip_count < static_cast<int>(filenames.size())) {
//                 LogInfo << "Skipping first " << skip_count << " files as per skip_files configuration" << std::endl;
//                 filenames.erase(filenames.begin(), filenames.begin() + skip_count);
//             }
//         } catch (...) { /* ignore */ }
//     }

//     return filenames;
// }

template <typename T> bool SetBranchWithFallback(TTree* tree,
                           std::initializer_list<const char*> candidateNames,
                           T* address,
                           const std::string& context) {
    for (const auto* name : candidateNames) {
        if (tree->GetBranch(name) != nullptr) {
            tree->SetBranchAddress(name, address);
            return true;
        }
    }

    std::ostringstream oss;
    oss << "Branches [";
    bool first = true;
    for (const auto* name : candidateNames) {
        if (!first) {
            oss << ", ";
        }
        oss << name;
        first = false;
    }
    oss << "] not found";

    LogWarning << oss.str() << " in " << context << std::endl;
    return false;
}

// Explicit template instantiations for SetBranchWithFallback
template bool SetBranchWithFallback<double>(TTree*, std::initializer_list<const char*>, double*, const std::string&);
template bool SetBranchWithFallback<float>(TTree*, std::initializer_list<const char*>, float*, const std::string&);
template bool SetBranchWithFallback<int>(TTree*, std::initializer_list<const char*>, int*, const std::string&);
template bool SetBranchWithFallback<unsigned int>(TTree*, std::initializer_list<const char*>, unsigned int*, const std::string&);
template bool SetBranchWithFallback<unsigned short>(TTree*, std::initializer_list<const char*>, unsigned short*, const std::string&);

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

    // After gathering files, apply skip_files if specified
    if (!filenames.empty() && j.contains("skip_files")) {
        try {
            int skip_count = j.at("skip_files").get<int>();
            if (skip_count > 0 && skip_count < static_cast<int>(filenames.size())) {
                LogInfo << "Skipping first " << skip_count << " files as per skip_files configuration" << std::endl;
                filenames.erase(filenames.begin(), filenames.begin() + skip_count);
            }
        } catch (...) { /* ignore */ }
    }

    return filenames;
}

// another version to look for a specific pattern in json
std::vector<std::string> find_input_files(const nlohmann::json& j, const std::string& pattern) {
    std::vector<std::string> possible_patterns = {"tpstream", "tps", "tps_bg", "clusters", "sig", "bg"};
    LogInfo << "[find_input_files] Called with pattern: " << pattern << std::endl;

    if (std::find(possible_patterns.begin(), possible_patterns.end(), pattern) == possible_patterns.end()) {
        LogError << "[find_input_files] Pattern '" << pattern << "' not recognized. Valid patterns are: ";
        for (const auto& p : possible_patterns) {
            LogError << p << " ";
        }
        LogError << std::endl;
        return {};
    }

    // Accept files named *pattern.root or *pattern_*.root
    // For sig and bg patterns, we look for *_tps.root files (since they contain TPs)
    auto matches_pattern = [&pattern](const std::string& filename) -> bool {
        std::string base = std::filesystem::path(filename).filename().string();
        if (debugMode) LogDebug << "[find_input_files] Checking if '" << base << "' matches pattern '" << pattern << "'" << std::endl;
        
        // Special handling for sig and bg: they contain tps files
        if (pattern == "sig" || pattern == "bg") {
            // Match files ending with _tps.root
            if (base.size() > 9 && base.substr(base.size() - 9) == "_tps.root") {
                if (debugMode) LogDebug << "[find_input_files] '" << base << "' matches '*_tps.root' for pattern '" << pattern << "'" << std::endl;
                return true;
            }
            if (debugMode) LogDebug << "[find_input_files] '" << base << "' does not match '*_tps.root'" << std::endl;
            return false;
        }
        
        // For other patterns (tpstream, tps, tps_bg, clusters), match the pattern directly
        if (base.size() >= pattern.size() + 5 && base.substr(base.size() - 5) == ".root") {
            // Matches *pattern.root
            if (base.size() >= pattern.size() + 5 &&
                base.substr(base.size() - pattern.size() - 5, pattern.size()) == pattern) {
                if (debugMode) LogDebug << "[find_input_files] '" << base << "' matches '*" << pattern << ".root'" << std::endl;
                return true;
            }
            // Matches *pattern_*.root
            std::string pat_prefix = pattern + "_";
            if (base.size() > pat_prefix.size() + 5 &&
                base.substr(base.size() - 5) == ".root" &&
                base.find(pat_prefix) != std::string::npos) {
                if (debugMode) LogDebug << "[find_input_files] '" << base << "' matches '*" << pattern << "_*.root'" << std::endl;
                return true;
            }
        }
        if (debugMode) LogDebug << "[find_input_files] '" << base << "' does not match pattern" << std::endl;
        return false;
    };
        
    auto file_exists = [](const std::string& filename) -> bool {
        std::ifstream file(filename);
        bool exists = file.good();
        if (debugMode) LogDebug << "[find_input_files] Checking existence of '" << filename << "': " << (exists ? "exists" : "missing") << std::endl;
        return exists;
    };

    std::vector<std::string> input_files;

    // Special handling for clusters pattern
    if (pattern == "clusters") {
        // Get clustering parameters from JSON (with defaults if missing)
        std::string cluster_prefix = j.value("clusters_folder_prefix", "");
        int tick_limit = j.value("tick_limit", 0);
        int channel_limit = j.value("channel_limit", 0);
        int min_tps_to_cluster = j.value("min_tps_to_cluster", 0);
        int tot_cut = j.value("tot_cut", 0);
        int energy_cut = j.value("energy_cut", 0);
        std::string outfolder = j.value("clusters_folder", ".");

        // Build subfolder name with clustering conditions
        std::string clusters_subfolder = "clusters_" + cluster_prefix
            + "_tick" + std::to_string(tick_limit)
            + "_ch" + std::to_string(channel_limit)
            + "_min" + std::to_string(min_tps_to_cluster)
            + "_tot" + std::to_string(tot_cut)
            + "_e" + std::to_string(energy_cut);

        std::string clusters_folder_path = outfolder + "/" + clusters_subfolder;

        LogInfo << "[find_input_files] clusters_folder_path: " << clusters_folder_path << std::endl;

        // Scan the clusters_folder_path for matching files
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(clusters_folder_path, ec)) {
            if (ec) {
                LogError << "[find_input_files] Error iterating directory '" << clusters_folder_path << "': " << ec.message() << std::endl;
                break;
            }
            if (!entry.is_regular_file()) continue;
            std::string filepath = entry.path().string();
            if (file_exists(filepath) && matches_pattern(filepath)) {
                input_files.push_back(filepath);
            }
        }
        std::sort(input_files.begin(), input_files.end());
        return input_files;
    }

    // Standard pattern handling
    std::string folder_key      = pattern + "_folder";
    std::string input_file_key  = pattern + "_input_file";
    std::string input_list_key  = pattern + "_input_list";

    if (debugMode) LogDebug << "[find_input_files] Keys: folder_key='" << folder_key << "', input_file_key='" << input_file_key << "', input_list_key='" << input_list_key << "'" << std::endl;

    // Priority 1: pattern_input_file (single file)
    if (j.contains(input_file_key)) {
        std::string file = j[input_file_key].get<std::string>();
        LogInfo << "[find_input_files] Found key '" << input_file_key << "' with value: " << file << std::endl;
        if (!file.empty() && file_exists(file) && matches_pattern(file)) {
            LogInfo << "[find_input_files] Adding file from '" << input_file_key << "': " << file << std::endl;
            input_files.push_back(file);
        } else {
            LogWarning << "[find_input_files] File '" << file << "' is empty, missing, or does not match pattern" << std::endl;
        }
    } else {
        if (debugMode) LogDebug << "[find_input_files] Key '" << input_file_key << "' not found in JSON" << std::endl;
    }

    // Priority 2: pattern_folder (scan folder for files with pattern)
    if (input_files.empty() && j.contains(folder_key)) {
        std::string folder = j[folder_key].get<std::string>();
        if (verboseMode) LogInfo << "[find_input_files] Found key '" << folder_key << "' with value: " << folder << std::endl;
        if (!folder.empty()) {
            std::error_code ec;
            if (verboseMode) LogInfo << "[find_input_files] Scanning folder '" << folder << "' for matching files..." << std::endl;
            for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
                if (ec) {
                    LogError << "[find_input_files] Error iterating directory '" << folder << "': " << ec.message() << std::endl;
                    break;
                }
                if (!entry.is_regular_file()) {
                    if (debugMode) LogDebug << "[find_input_files] Skipping non-regular file: " << entry.path().string() << std::endl;
                    continue;
                }
                std::string filepath = entry.path().string();
                if (debugMode) LogDebug << "[find_input_files] Found file: " << filepath << std::endl;
                if (file_exists(filepath) && matches_pattern(filepath)) {
                    if (verboseMode) LogInfo << "[find_input_files] Adding file from folder: " << filepath << std::endl;
                    input_files.push_back(filepath);
                } else {
                    if (debugMode) LogDebug << "[find_input_files] File '" << filepath << "' does not exist or does not match pattern" << std::endl;
                }
            }
        } else {
            LogWarning << "[find_input_files] Folder value for key '" << folder_key << "' is empty" << std::endl;
        }
    } else if (input_files.empty()) {
        if (debugMode) LogDebug << "[find_input_files] Key '" << folder_key << "' not found in JSON or input_files already found" << std::endl;
    }

    // Priority 3: pattern_input_list (array of files)
    if (input_files.empty() && j.contains(input_list_key) && j[input_list_key].is_array()) {
        if (verboseMode) LogInfo << "[find_input_files] Found key '" << input_list_key << "' with array value" << std::endl;
        for (const auto& el : j[input_list_key]) {
            if (!el.is_string()) {
                if (debugMode) LogDebug << "[find_input_files] Skipping non-string entry in '" << input_list_key << "'" << std::endl;
                continue;
            }
            std::string file = el.get<std::string>();
            if (debugMode) LogDebug << "[find_input_files] Checking file from list: " << file << std::endl;
            if (!file.empty() && file_exists(file) && matches_pattern(file)) {
                if (verboseMode) LogInfo << "[find_input_files] Adding file from list: " << file << std::endl;
                input_files.push_back(file);
            } else {
                LogWarning << "[find_input_files] File '" << file << "' is empty, missing, or does not match pattern" << std::endl;
            }
        }
    } else if (input_files.empty()) {
        if (debugMode) LogDebug << "[find_input_files] Key '" << input_list_key << "' not found in JSON or input_files already found" << std::endl;
    }

    if (verboseMode) LogInfo << "[find_input_files] Sorting " << input_files.size() << " input files" << std::endl;
    std::sort(input_files.begin(), input_files.end());

    if (verboseMode) LogInfo << "[find_input_files] Final input files:" << std::endl;
    for (const auto& f : input_files) {
        if (verboseMode) LogInfo << "  " << f << std::endl;
    }

    return input_files;
}
