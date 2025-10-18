#include "Backtracking.h"
#include "Clustering.h"

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

// Helper function to find files in a folder matching a pattern
std::vector<std::string> find_files_in_folder(const std::string& folder, const std::string& pattern = "", const std::string& suffix = ".root") {
    std::vector<std::string> files;
    
    if (!std::filesystem::exists(folder) || !std::filesystem::is_directory(folder)) {
        LogWarning << "Folder does not exist or is not a directory: " << folder << std::endl;
        return files;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().string();
            
            // Check if filename contains the pattern (if specified)
            if (!pattern.empty() && filename.find(pattern) == std::string::npos) {
                continue;
            }
            
            // Check if filename ends with the suffix
            if (!suffix.empty() && filename.find(suffix) == std::string::npos) {
                continue;
            }
            
            files.push_back(filename);
        }
    }
    
    // Sort files for consistent ordering
    std::sort(files.begin(), files.end());
    
    return files;
}

int main(int argc, char* argv[]) {
    CmdLineParser clp;
    clp.getDescription() << "> add_backgrounds app - Merge signal TPs with random background events, writing *_tps_bg.root files." << std::endl;
    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");
    clp.addTriggerOption("verboseMode", {"-v", "--verbose"}, "Run in verbose mode");
    clp.addTriggerOption("debugMode", {"-d", "--debug"}, "Run in debug mode (more detailed than verbose)");
    clp.addTriggerOption("override", {"-o", "--override"}, "Override existing output files");
    clp.addDummyOption();
    
    LogInfo << clp.getDescription().str() << std::endl;
    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;
    
    clp.parseCmdLine(argc, argv);
    LogThrowIf( clp.isNoOptionTriggered(), "No option was provided." );

    bool verboseMode = clp.isOptionTriggered("verboseMode");
    bool debugMode = clp.isOptionTriggered("debugMode");
    bool overrideMode = clp.isOptionTriggered("override");
    if (debugMode) verboseMode = true;
    
    // Load parameters
    ParametersManager::getInstance().loadParameters();

    std::string json = clp.getOptionVal<std::string>("json");
    std::ifstream i(json); 
    LogThrowIf(!i.good(), "Failed to open JSON config: " << json);
    nlohmann::json j; 
    i >> j;

    // Parse JSON configuration
    std::string signal_type = j.value("signal_type", std::string("cc")); // "cc" or "es", just in case...
    bool around_vertex_only = j.value("around_vertex_only", false);
    double vertex_radius = j.value("vertex_radius", 100.0); // cm, used if around_vertex_only=true
    int max_files = j.value("max_files", -1); // -1 means no limit
    std::string bkg_folder = j.value("bkg_folder", std::string(""));
    LogThrowIf(bkg_folder.empty(), "bkg_folder is not specified in JSON config.");
    
    // Read input_folder from JSON
    std::string input_folder = j.value("inputFolder", std::string(""));
    LogThrowIf(input_folder.empty(), "input_folder is not specified in JSON config.");

    LogInfo << "Configuration:" << std::endl;
    LogInfo << " - Signal type: " << signal_type << std::endl;
    LogInfo << " - Signal folder: " << input_folder << std::endl;
    LogInfo << " - Background folder: " << bkg_folder << std::endl;
    LogInfo << " - Add backgrounds around vertex only: " << (around_vertex_only ? "YES" : "NO") << std::endl;
    if (around_vertex_only) {
        LogInfo << " - Vertex radius: " << vertex_radius << " cm" << std::endl;
    }
    if (max_files > 0) {
        LogInfo << " - Max files to process: " << max_files << std::endl;
    } else {
        LogInfo << " - Max files to process: unlimited" << std::endl;
    }

    // Find signal files using utility function
    std::vector<std::string> signal_files = find_input_files(j, "_tps.root");
    LogInfo << "Found " << signal_files.size() << " signal files" << std::endl;
    LogThrowIf(signal_files.empty(), "No signal files found.");

    // Find background files using helper function (looking for BG_*_tps.root pattern)
    std::vector<std::string> bkg_files = find_files_in_folder(bkg_folder, "BG_", "_tps.root");
    LogInfo << "Found " << bkg_files.size() << " background files" << std::endl;
    LogThrowIf(bkg_files.empty(), "No background files found.");

    // Initialize for cycling through background files and events
    size_t bkg_file_idx = 0;
    size_t bkg_event_idx = 0;
    
    // Cache for current background file to avoid repeated reads
    std::string cached_bkg_file = "";
    std::map<int, std::vector<TriggerPrimitive>> cached_bkg_tps;
    std::map<int, std::vector<TrueParticle>> cached_bkg_true;
    std::map<int, std::vector<Neutrino>> cached_bkg_nu;
    std::vector<int> cached_bkg_event_ids;

    // Process signal files
    int file_count = 0;
    std::vector<std::string> output_files;
    
    for (const auto& signal_file : signal_files) {
        // Check if we've reached max_files limit
        if (max_files > 0 && file_count >= max_files) {
            LogInfo << "Reached max_files limit (" << max_files << "), stopping." << std::endl;
            break;
        }
        file_count++;
        if (!verboseMode) {
            GenericToolbox::displayProgressBar(file_count, std::min((int)signal_files.size(), max_files > 0 ? max_files : (int)signal_files.size()), "Adding backgrounds...");
        }
        if (verboseMode) LogInfo << "\nProcessing signal file: " << signal_file << std::endl;
        // Read signal TPs
        std::map<int, std::vector<TriggerPrimitive>> signal_tps_by_event;
        std::map<int, std::vector<TrueParticle>> signal_true_by_event;
        std::map<int, std::vector<Neutrino>> signal_nu_by_event;
        read_tps(signal_file, signal_tps_by_event, signal_true_by_event, signal_nu_by_event);
        // Prepare output file name
        std::filesystem::path signal_path(signal_file);
        std::string outputFolder = j.value("outputFolder", "") ;
        LogThrowIf(outputFolder.empty(), "outputFolder is not specified in JSON config.");
        
        // Get base filename without _tps.root suffix
        std::string base_name = signal_path.stem().string();
        if (base_name.size() > 4 && base_name.substr(base_name.size() - 4) == "_tps") {
            base_name = base_name.substr(0, base_name.size() - 4);
        }
        
        std::string output_filename = outputFolder + "/" + base_name;
        if (around_vertex_only) {
            output_filename += "_bg_vtx" + std::to_string((int)vertex_radius) + "_tps.root";
        } else {
            output_filename += "_bg_tps.root";
        }
        if (verboseMode) LogInfo << "Output file: " << output_filename << std::endl;
        // Check if output file already exists
        if (std::filesystem::exists(output_filename) && !overrideMode) {
            file_count--;
            if (verboseMode) LogInfo << "Output file already exists, skipping (use --override to overwrite)" << std::endl;
            continue;
        }
        // Prepare merged data structures
        std::vector<std::vector<TriggerPrimitive>> merged_tps_vec;
        std::vector<std::vector<TrueParticle>> merged_true_vec;
        std::vector<std::vector<Neutrino>> merged_nu_vec;
        // Persistent storage for background truth data (must live until write_tps is called)
        // Use a list instead of vector to avoid pointer invalidation on reallocation
        std::list<std::vector<TrueParticle>> persistent_bkg_true;
        // Track signal TP counts for each event (for re-linking later)
        std::vector<int> signal_tp_counts;
        // Process each event in the signal file
        for (const auto& kv : signal_tps_by_event) {
            int event_id = kv.first;
            const auto& signal_tps = kv.second;
            
            // Get neutrino vertex position if around_vertex_only is enabled
            TVector3 vertex_pos(0, 0, 0);
            bool has_vertex = false;
            if (around_vertex_only && signal_nu_by_event.count(event_id) > 0) {
                const auto& neutrinos = signal_nu_by_event.at(event_id);
                if (!neutrinos.empty()) {
                    vertex_pos.SetXYZ(neutrinos[0].GetX(), neutrinos[0].GetY(), neutrinos[0].GetZ());
                    has_vertex = true;
                    if (debugMode) {
                        LogInfo << "Event " << event_id << " vertex at (" 
                                << vertex_pos.X() << ", " << vertex_pos.Y() << ", " << vertex_pos.Z() << ")" << std::endl;
                    }
                }
            }
            
            // Start with signal TPs
            std::vector<TriggerPrimitive> merged_tps = signal_tps;
            
            // Vectors to hold merged truth info for this event
            std::vector<TrueParticle> merged_true;
            std::vector<Neutrino> merged_nu;
            
            // Load background file if needed (with caching)
            const std::string& current_bkg_file = bkg_files[bkg_file_idx];
            if (cached_bkg_file != current_bkg_file) {
                if (verboseMode) LogInfo << "Loading background file: " << current_bkg_file << std::endl;
                cached_bkg_tps.clear();
                cached_bkg_true.clear();
                cached_bkg_nu.clear();
                cached_bkg_event_ids.clear();
                read_tps(current_bkg_file, cached_bkg_tps, cached_bkg_true, cached_bkg_nu);
                for (const auto& bkg_kv : cached_bkg_tps) {
                    cached_bkg_event_ids.push_back(bkg_kv.first);
                }
                cached_bkg_file = current_bkg_file;
            }
            
            if (!cached_bkg_event_ids.empty()) {
                // Wrap around if we've gone past the end of events in this file
                if (bkg_event_idx >= cached_bkg_event_ids.size()) {
                    bkg_event_idx = 0;
                    bkg_file_idx = (bkg_file_idx + 1) % bkg_files.size();
                    // Reload the new file
                    const std::string& next_bkg_file = bkg_files[bkg_file_idx];
                    if (verboseMode) LogInfo << "Moving to next background file: " << next_bkg_file << std::endl;
                    cached_bkg_tps.clear();
                    cached_bkg_true.clear();
                    cached_bkg_nu.clear();
                    cached_bkg_event_ids.clear();
                    read_tps(next_bkg_file, cached_bkg_tps, cached_bkg_true, cached_bkg_nu);
                    for (const auto& bkg_kv : cached_bkg_tps) {
                        cached_bkg_event_ids.push_back(bkg_kv.first);
                    }
                    cached_bkg_file = next_bkg_file;
                    bkg_event_idx = 0;
                }
                
                int bkg_event_id = cached_bkg_event_ids[bkg_event_idx];
                const auto& bkg_tps = cached_bkg_tps.at(bkg_event_id);
                const auto& bkg_true_by_event = cached_bkg_true;
                
                if (verboseMode) {
                    LogInfo << "Signal event " << event_id << ": merging with background file "
                            << std::filesystem::path(cached_bkg_file).filename().string()
                            << " event " << bkg_event_id << " (event index " << bkg_event_idx << ")"
                            << " (" << bkg_tps.size() << " TPs)" << std::endl;
                }
                
                // Deep copy background truth data to persistent storage
                std::vector<TrueParticle> bkg_true_copy;
                if (bkg_true_by_event.count(bkg_event_id) > 0) {
                    bkg_true_copy = bkg_true_by_event.at(bkg_event_id);
                    // Update event numbers for all background truth particles (CRITICAL BUG FIX)
                    for (auto& true_particle : bkg_true_copy) {
                        true_particle.SetEvent(event_id);
                    }
                }
                persistent_bkg_true.push_back(bkg_true_copy);
                std::vector<TrueParticle>& persistent_bkg = persistent_bkg_true.back();
                
                int bkg_added = 0;
                int bkg_truth_linked = 0;
                
                // Add background TPs, updating their truth pointers to the persistent copied data
                for (const auto& bkg_tp : bkg_tps) {
                    TriggerPrimitive tp = bkg_tp;
                    
                    // Update the event number to match the signal event (CRITICAL BUG FIX)
                    tp.SetEvent(event_id);
                    
                    // Update truth pointer to point to our persistent copy
                    const TrueParticle* orig_ptr = bkg_tp.GetTrueParticle();
                    if (orig_ptr != nullptr && !persistent_bkg.empty()) {
                        int truth_id = orig_ptr->GetTruthId();
                        int track_id = orig_ptr->GetTrackId();
                        bool found = false;
                        for (auto& copied_true : persistent_bkg) {
                            if (copied_true.GetTruthId() == truth_id && copied_true.GetTrackId() == track_id) {
                                tp.SetTrueParticle(&copied_true);
                                if (tp.GetTrueParticle() != nullptr) {
                                    bkg_truth_linked++;
                                } else if (debugMode) {
                                    LogWarning << "SetTrueParticle failed - pointer is still null!" << std::endl;
                                }
                                found = true;
                                break;
                            }
                        }
                        if (!found && debugMode) {
                            LogWarning << "Could not find matching TrueParticle for bkg TP (truth_id=" 
                                      << truth_id << ", track_id=" << track_id << ")" << std::endl;
                        }
                    }
                    merged_tps.push_back(tp);
                    bkg_added++;
                }
                
                if (around_vertex_only && has_vertex) {
                    LogWarning << "around_vertex_only is enabled but TP position filtering not yet implemented. Adding all background TPs." << std::endl;
                }
                
                if (verboseMode) {
                    LogInfo << "Signal event " << event_id << ": " << signal_tps.size() << " signal TPs + "
                            << bkg_added << " background TPs (truth linked: " << bkg_truth_linked << ") = "
                            << merged_tps.size() << " total TPs" << std::endl;
                }
                
                // Merge truth information from signal and copied background
                if (signal_true_by_event.count(event_id) > 0) {
                    merged_true = signal_true_by_event.at(event_id);
                }
                merged_true.insert(merged_true.end(), persistent_bkg.begin(), persistent_bkg.end());
                if (signal_nu_by_event.count(event_id) > 0) {
                    merged_nu = signal_nu_by_event.at(event_id);
                }
                
                // Move to next event (and potentially next file)
                bkg_event_idx++;
                
            } else {
                // No background TPs available, just use signal truth
                if (signal_true_by_event.count(event_id) > 0) {
                    merged_true = signal_true_by_event.at(event_id);
                }
                if (signal_nu_by_event.count(event_id) > 0) {
                    merged_nu = signal_nu_by_event.at(event_id);
                }
            }
            
            merged_tps_vec.push_back(merged_tps);
            merged_true_vec.push_back(merged_true);
            merged_nu_vec.push_back(merged_nu);
            signal_tp_counts.push_back(signal_tps.size());
        }
        
        // CRITICAL: Update background TP pointers to point to TrueParticles in merged_true_vec
        // This must be done AFTER all events are added to merged_true_vec
        int relinked_count = 0;
        for (size_t ev_idx = 0; ev_idx < merged_tps_vec.size(); ev_idx++) {
            auto& event_tps = merged_tps_vec[ev_idx];
            auto& event_true = merged_true_vec[ev_idx];
            int signal_tp_count = signal_tp_counts[ev_idx];
            
            // Re-link ALL TPs (both signal and background) to TrueParticles in merged_true_vec
            for (size_t tp_idx = 0; tp_idx < event_tps.size(); tp_idx++) {
                auto& tp = event_tps[tp_idx];
                const auto* old_ptr = tp.GetTrueParticle();
                if (old_ptr != nullptr) {
                    // Find matching TrueParticle in merged_true_vec by truth_id and track_id
                    for (auto& true_particle : event_true) {
                        if (true_particle.GetTruthId() == old_ptr->GetTruthId() &&
                            true_particle.GetTrackId() == old_ptr->GetTrackId()) {
                            tp.SetTrueParticle(&true_particle);
                            if ((int)tp_idx >= signal_tp_count) {  // Background TP
                                relinked_count++;
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (verboseMode) LogInfo << "Re-linked " << relinked_count << " background TP pointers to merged_true_vec" << std::endl;
        
        // Debug: Verify pointers right before write
        if (verboseMode && !merged_tps_vec.empty() && !merged_tps_vec[0].empty()) {
            int bkg_tps_checked = 0;
            for (const auto& tp : merged_tps_vec[0]) {
                if (bkg_tps_checked >= signal_tps_by_event[0].size()) { // Skip signal TPs
                    const auto* true_ptr = tp.GetTrueParticle();
                    if (true_ptr != nullptr) {
                        LogInfo << "  PRE-WRITE: TP has generator: " << true_ptr->GetGeneratorName() << std::endl;
                        break;
                    }
                }
                bkg_tps_checked++;
            }
        }
        
        // Write output file with merged TPs
        // Background truth pointers remain valid since all background data is pre-loaded
        write_tps(output_filename, merged_tps_vec, merged_true_vec, merged_nu_vec);
        output_files.push_back(output_filename);
        
        if (verboseMode) LogInfo << "Wrote: " << output_filename << std::endl;
    }
    
    LogInfo << "\n\nProcessed " << file_count << " files successfully." << std::endl;
    LogInfo << "Output files are in the same directories as input files with '_bkg' suffix." << std::endl;
    LogInfo << "\nNote: In the output files, background TPs have their original generator labels," << std::endl;
    LogInfo << "      allowing you to distinguish signal (MARLEY) from background (e.g., radiological)." << std::endl;
    
    // Print summary of output files
    LogInfo << "\nOutput files (" << output_files.size() << "):" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(5, output_files.size()); i++) {
        LogInfo << " - " << output_files[i] << std::endl;
    }
    if (output_files.size() > 5) {
        LogInfo << " ... (" << output_files.size() - 5 << " more files)" << std::endl;
    }
    
    return 0;
}
