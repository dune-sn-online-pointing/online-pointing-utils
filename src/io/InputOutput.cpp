#include "Global.h"

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

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

std::string getTpstreamBaseFolder(const nlohmann::json& j) {
    // Priority 1: Explicit tpstream_folder
    if (j.contains("tpstream_folder") && !j["tpstream_folder"].get<std::string>().empty()) {
        std::string folder = j["tpstream_folder"].get<std::string>();
        return std::filesystem::path(folder).lexically_normal().string();
    }
    
    // Priority 2: Auto-generate from main_folder (new structure)
    if (j.contains("main_folder") && !j["main_folder"].get<std::string>().empty()) {
        std::string main_folder = j["main_folder"].get<std::string>();
        std::filesystem::path tpstream_path = std::filesystem::path(main_folder) / "tpstreams";
        return tpstream_path.lexically_normal().string();
    }
    
    // Priority 3: Auto-generate from signal_folder (new structure for samples with signal + bg)
    if (j.contains("signal_folder") && !j["signal_folder"].get<std::string>().empty()) {
        std::string signal_folder = j["signal_folder"].get<std::string>();
        std::filesystem::path tpstream_path = std::filesystem::path(signal_folder) / "tpstreams";
        return tpstream_path.lexically_normal().string();
    }
    
    // Fallback: current directory
    return ".";
}

std::string resolveFolderAgainstTpstream(const nlohmann::json& j, const std::string& folder, bool useBaseOnEmpty) {
    std::string base = getTpstreamBaseFolder(j);

    if (folder.empty()) {
        return useBaseOnEmpty ? base : std::string{};
    }

    std::filesystem::path folder_path(folder);
    if (folder_path.is_absolute()) {
        return folder_path.lexically_normal().string();
    }

    std::filesystem::path resolved = std::filesystem::path(base) / folder_path;
    return resolved.lexically_normal().string();
}


void bindBranch(TTree *tree, const char* name, void* address) {
    if (tree->SetBranchAddress(name, address) < 0) {
        LogWarning << "Failed to bind branch '" << name << "'" << std::endl;
    }
}


std::string getClustersFolder(const nlohmann::json& j) {
    // If explicit clusters_folder provided, use it
    std::string outfolder = j.value("clusters_folder", std::string(""));
    if (!outfolder.empty()) {
        return resolveFolderAgainstTpstream(j, outfolder, true);
    }

    // Get base folder (main_folder or signal_folder or fallback to tpstream)
    if (j.contains("main_folder") && !j["main_folder"].get<std::string>().empty()) {
        outfolder = j["main_folder"].get<std::string>();
    } else if (j.contains("signal_folder") && !j["signal_folder"].get<std::string>().empty()) {
        outfolder = j["signal_folder"].get<std::string>();
    } else {
        outfolder = getTpstreamBaseFolder(j);
    }

    // Get cluster folder prefix (use products_prefix if clusters_folder_prefix not set)
    std::string cluster_prefix = j.value("clusters_folder_prefix", j.value("products_prefix", std::string("")));

    // Extract clustering parameters from JSON, with defaults if missing
    int tick_limit = j.value("tick_limit", 0);
    int channel_limit = j.value("channel_limit", 0);
    int min_tps_to_cluster = j.value("min_tps_to_cluster", 0);
    int tot_cut = j.value("tot_cut", 0);
    float energy_cut = 0.0f;
    if (j.contains("energy_cut")) {
        try {
            energy_cut = j.at("energy_cut").get<float>();
        } catch (const std::exception&) {
            // Fallback: try reading as double and cast to float
            energy_cut = static_cast<float>(j.at("energy_cut").get<double>());
        }
    }

    // Build subfolder name with clustering conditions
    auto sanitize = [](const std::string& s) {
        std::string out = s;
        // keep at most one digit after the decimal point
        auto pos = out.find('.');
        if (pos != std::string::npos) {
            size_t keep = std::min(out.size(), pos + 2); // dot + 1 digit
            out = out.substr(0, keep);
        }
        // make filesystem-friendly (replace '.' with 'p')
        std::replace(out.begin(), out.end(), '.', 'p');
        return out;
    };

    std::string conditions = "_tick" + sanitize(std::to_string(tick_limit))
        + "_ch" + sanitize(std::to_string(channel_limit))
        + "_min" + sanitize(std::to_string(min_tps_to_cluster))
        + "_tot" + sanitize(std::to_string(tot_cut))
        + "_e" + sanitize(std::to_string(energy_cut));

    std::string clusters_subfolder;
    if (!cluster_prefix.empty()) {
        // Pattern: prefix_clusters_conditions
        clusters_subfolder = cluster_prefix + "_clusters" + conditions;
    } else {
        clusters_subfolder = "clusters" + conditions;
    }
    
    std::filesystem::path clusters_folder_path = std::filesystem::path(outfolder) / clusters_subfolder;
    return clusters_folder_path.lexically_normal().string();
}

// Helper: Build conditions string from clustering parameters
std::string getConditionsString(const nlohmann::json& j) {
    auto sanitize = [](const std::string& s) {
        std::string out = s;
        auto pos = out.find('.');
        if (pos != std::string::npos) {
            size_t keep = std::min(out.size(), pos + 2);
            out = out.substr(0, keep);
        }
        std::replace(out.begin(), out.end(), '.', 'p');
        return out;
    };

    int tick_limit = j.value("tick_limit", 0);
    int channel_limit = j.value("channel_limit", 0);
    int min_tps_to_cluster = j.value("min_tps_to_cluster", 0);
    int tot_cut = j.value("tot_cut", 0);
    float energy_cut = 0.0f;
    if (j.contains("energy_cut")) {
        try {
            energy_cut = j.at("energy_cut").get<float>();
        } catch (const std::exception&) {
            energy_cut = static_cast<float>(j.at("energy_cut").get<double>());
        }
    }

    return "tick" + sanitize(std::to_string(tick_limit))
        + "_ch" + sanitize(std::to_string(channel_limit))
        + "_min" + sanitize(std::to_string(min_tps_to_cluster))
        + "_tot" + sanitize(std::to_string(tot_cut))
        + "_e" + sanitize(std::to_string(energy_cut));
}

// Helper: Get output folder with auto-generation logic
// Priority: 1) Explicit JSON key, 2) Auto-generate from main_folder/signal_folder
std::string getOutputFolder(const nlohmann::json& j, const std::string& folder_type, const std::string& json_key) {
    // If explicit path provided in JSON, use it
    if (j.contains(json_key) && !j[json_key].get<std::string>().empty()) {
        return resolveFolderAgainstTpstream(j, j[json_key].get<std::string>(), true);
    }

    // Auto-generate from main_folder or signal_folder
    std::string base_folder;
    if (j.contains("main_folder") && !j["main_folder"].get<std::string>().empty()) {
        base_folder = j["main_folder"].get<std::string>();
    } else if (j.contains("signal_folder") && !j["signal_folder"].get<std::string>().empty()) {
        base_folder = j["signal_folder"].get<std::string>();
    } else {
        // Fallback to tpstream_folder for backward compatibility
        base_folder = getTpstreamBaseFolder(j);
    }
    
    std::filesystem::path base_path(base_folder);

    std::string prefix = j.value("clusters_folder_prefix", j.value("products_prefix", std::string("")));
    std::string conditions = getConditionsString(j);

    if (folder_type == "tps") {
        return (base_path / "tps").lexically_normal().string();
    } else if (folder_type == "tps_bg") {
        return (base_path / "tps_bg").lexically_normal().string();
    } else if (folder_type == "clusters") {
        std::string clusters_name;
        if (!prefix.empty()) {
            // Pattern: prefix_clusters_conditions
            clusters_name = prefix + "_clusters_" + conditions;
        } else {
            clusters_name = "clusters_" + conditions;
        }
        return (base_path / clusters_name).lexically_normal().string();
    } else if (folder_type == "cluster_images") {
        std::string images_name;
        if (!prefix.empty()) {
            // Pattern: prefix_cluster_images_conditions
            images_name = prefix + "_cluster_images_" + conditions;
        } else {
            images_name = "cluster_images_" + conditions;
        }
        return (base_path / images_name).lexically_normal().string();
    } else if (folder_type == "volume_images" || folder_type == "volumes") {
        std::string volumes_name;
        if (!prefix.empty()) {
            // Pattern: prefix_volume_images_conditions
            volumes_name = prefix + "_volume_images_" + conditions;
        } else {
            volumes_name = "volume_images_" + conditions;
        }
        return (base_path / volumes_name).lexically_normal().string();
    } else if (folder_type == "reports") {
        return (base_path / "reports").lexically_normal().string();
    } else if (folder_type == "matched_clusters") {
        std::string matched_name;
        if (!prefix.empty()) {
            // Pattern: prefix_matched_clusters_conditions
            matched_name = prefix + "_matched_clusters_" + conditions;
        } else {
            matched_name = "matched_clusters_" + conditions;
        }
        return (base_path / matched_name).lexically_normal().string();
    }

    // Default: return base folder
    return base_path.lexically_normal().string();
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
        
        // Special handling for tps_bg: match files ending with _bg_tps.root
        if (pattern == "tps_bg") {
            if (base.size() > 12 && base.substr(base.size() - 12) == "_bg_tps.root") {
                if (debugMode) LogDebug << "[find_input_files] '" << base << "' matches '*_bg_tps.root' for pattern 'tps_bg'" << std::endl;
                return true;
            }
            if (debugMode) LogDebug << "[find_input_files] '" << base << "' does not match '*_bg_tps.root'" << std::endl;
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
        
        std::string clusters_folder_path = getClustersFolder(j);

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

    // Auto-generate folder path if not explicitly provided
    // For "bg" pattern, ALWAYS auto-generate (bg_folder is base, we need bg_folder/tps)
    std::string auto_folder;
    bool should_auto_generate = !j.contains(folder_key) || (pattern == "bg" && j.contains("bg_folder"));
    
    if (should_auto_generate) {
        // Get base folder (main_folder or signal_folder or bg_folder)
        std::string base;
        if (pattern == "bg") {
            // For background pattern, use bg_folder as base
            if (j.contains("bg_folder") && !j["bg_folder"].get<std::string>().empty()) {
                base = j["bg_folder"].get<std::string>();
            }
        } else {
            // For other patterns, use main_folder or signal_folder
            if (j.contains("main_folder") && !j["main_folder"].get<std::string>().empty()) {
                base = j["main_folder"].get<std::string>();
            } else if (j.contains("signal_folder") && !j["signal_folder"].get<std::string>().empty()) {
                base = j["signal_folder"].get<std::string>();
            }
        }
        
        // Auto-generate subfolder based on pattern
        if (!base.empty()) {
            if (pattern == "tpstream") {
                auto_folder = (std::filesystem::path(base) / "tpstreams").lexically_normal().string();
            } else if (pattern == "tps" || pattern == "sig") {
                auto_folder = (std::filesystem::path(base) / "tps").lexically_normal().string();
            } else if (pattern == "tps_bg") {
                auto_folder = (std::filesystem::path(base) / "tps_bg").lexically_normal().string();
            } else if (pattern == "bg") {
                // For background, auto-generate to bg_folder/tps
                auto_folder = (std::filesystem::path(base) / "tps").lexically_normal().string();
            }
        }

        if (!auto_folder.empty() && verboseMode) {
            LogInfo << "[find_input_files] Auto-generated '" << folder_key << "': " << auto_folder << std::endl;
        }
    }

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
    if (input_files.empty()) {
        std::string folder;
        // For bg pattern, always use auto_folder (which adds /tps to bg_folder)
        if (pattern == "bg" && !auto_folder.empty()) {
            folder = auto_folder;
        } else if (j.contains(folder_key)) {
            folder = resolveFolderAgainstTpstream(j, j[folder_key].get<std::string>(), pattern == "sig");
        } else if (!auto_folder.empty()) {
            folder = auto_folder;
        }
        
        if (!folder.empty()) {
            if (!std::filesystem::exists(folder)) {
                if (pattern == "sig") {
                    // Try fallback to tps folder
                    std::string fallback;
                    if (j.contains("main_folder")) {
                        fallback = (std::filesystem::path(j["main_folder"].get<std::string>()) / "tps").string();
                    } else if (j.contains("signal_folder")) {
                        fallback = (std::filesystem::path(j["signal_folder"].get<std::string>()) / "tps").string();
                    }
                    if (!fallback.empty() && folder != fallback) {
                        LogWarning << "[find_input_files] Folder '" << folder << "' does not exist. Falling back to tps folder: " << fallback << std::endl;
                        folder = fallback;
                    }
                }
            }
            if (verboseMode) LogInfo << "[find_input_files] Found key '" << folder_key << "' with value: " << folder << std::endl;
            if (verboseMode) LogInfo << "[find_input_files] Scanning folder '" << folder << "' for matching files..." << std::endl;
            
            // OPTIMIZATION: Use system find command for faster directory scanning on EOS
            // Build the find command based on pattern
            std::string find_pattern;
            if (pattern == "sig" || pattern == "bg") {
                find_pattern = "*_tps.root";
            } else if (pattern == "tps_bg") {
                find_pattern = "*_bg_tps.root";
            } else {
                find_pattern = "*" + pattern + "*.root";
            }
            
            std::string find_cmd = "find \"" + folder + "\" -maxdepth 1 -type f -name \"" + find_pattern + "\" 2>/dev/null";
            if (verboseMode) LogInfo << "[find_input_files] Using fast find command: " << find_cmd << std::endl;
            
            FILE* pipe = popen(find_cmd.c_str(), "r");
            if (pipe) {
                char buffer[4096];
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    std::string filepath(buffer);
                    // Remove trailing newline
                    if (!filepath.empty() && filepath.back() == '\n') {
                        filepath.pop_back();
                    }
                    if (!filepath.empty() && matches_pattern(filepath)) {
                        if (verboseMode) LogInfo << "[find_input_files] Adding file from folder: " << filepath << std::endl;
                        input_files.push_back(filepath);
                    }
                }
                pclose(pipe);
                
                if (verboseMode) LogInfo << "[find_input_files] Fast find completed, found " << input_files.size() << " files" << std::endl;
            } else {
                // Fallback to original method if popen fails
                LogWarning << "[find_input_files] Fast find command failed, using fallback method" << std::endl;
                std::error_code ec;
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
            }
        }
    }
    if (input_files.empty()) {
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

std::string extractBasename(const std::string& filepath) {
    /**
     * Extract basename from tpstream file by removing _tpstream.root suffix.
     * This allows consistent file tracking across pipeline stages.
     * 
     * Example:
     *   prodmarley_..._20250826T091337Z_gen_000014_..._tpstream.root
     *   -> prodmarley_..._20250826T091337Z_gen_000014_...
     */
    std::filesystem::path p(filepath);
    std::string filename = p.filename().string();
    
    const std::string suffix = "_tpstream.root";
    if (filename.size() > suffix.size() && 
        filename.substr(filename.size() - suffix.size()) == suffix) {
        return filename.substr(0, filename.size() - suffix.size());
    }
    
    return filename;
}

std::vector<std::string> findFilesMatchingBasenames(
    const std::vector<std::string>& basenames,
    const std::vector<std::string>& candidate_files) {
    /**
     * Filter candidate files to only those matching the given basenames.
     * This ensures skip/max from tpstream list applies to all pipeline stages.
     */
    std::vector<std::string> matched_files;
    
    for (const auto& candidate : candidate_files) {
        std::filesystem::path p(candidate);
        std::string filename = p.filename().string();
        
        // Check if this file matches any of the basenames
        for (const auto& basename : basenames) {
            if (filename.find(basename) != std::string::npos) {
                matched_files.push_back(candidate);
                break;
            }
        }
    }
    
    // Maintain original order
    return matched_files;
}

std::vector<std::string> find_input_files_by_tpstream_basenames(
    const nlohmann::json& j,
    const std::string& pattern,
    int skip_files,
    int max_files) {
    /**
     * Find input files using tpstream file list as source of truth.
     * 
     * This ensures skip/max parameters consistently refer to the same
     * source files across all pipeline stages (backtrack, add_backgrounds,
     * make_clusters, generate_images, create_volumes).
     * 
     * Algorithm:
     *   1. Get list of tpstream files
     *   2. Apply skip/max to tpstream list
     *   3. Extract basenames from selected tpstream files
     *   4. Find files matching those basenames with the requested pattern
     */
    
    // Get tpstream files (the source of truth)
    std::vector<std::string> tpstream_files = find_input_files(j, "tpstream");
    
    if (tpstream_files.empty()) {
        LogWarning << "[find_input_files_by_tpstream_basenames] No tpstream files found" << std::endl;
        return {};
    }
    
    // Apply skip and max to tpstream list
    if (skip_files > 0 && skip_files < (int)tpstream_files.size()) {
        tpstream_files.erase(tpstream_files.begin(), tpstream_files.begin() + skip_files);
    }
    if (max_files > 0 && max_files < (int)tpstream_files.size()) {
        tpstream_files.resize(max_files);
    }
    
    // Extract basenames
    std::vector<std::string> basenames;
    for (const auto& tpstream_file : tpstream_files) {
        basenames.push_back(extractBasename(tpstream_file));
    }
    
    LogInfo << "[find_input_files_by_tpstream_basenames] Using " << basenames.size() 
            << " basenames from tpstream files (skip=" << skip_files 
            << ", max=" << max_files << ")" << std::endl;
    
    // Now find files matching the requested pattern
    std::vector<std::string> all_pattern_files = find_input_files(j, pattern);
    
    // Filter to only files matching our basenames
    std::vector<std::string> matched_files = findFilesMatchingBasenames(basenames, all_pattern_files);
    
    LogInfo << "[find_input_files_by_tpstream_basenames] Found " << matched_files.size() 
            << " files matching basenames for pattern '" << pattern << "'" << std::endl;
    
    return matched_files;
}
