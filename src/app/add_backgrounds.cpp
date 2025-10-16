#include "Backtracking.h"
#include "Clustering.h"

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

int main(int argc, char* argv[]) {
    CmdLineParser clp;
    clp.getDescription() << "> add_backgrounds app - Merge signal TPs with random background events, writing *_tps_bkg.root files." << std::endl;
    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");
    clp.addTriggerOption("verboseMode", {"-v", "--verbose"}, "Run in verbose mode");
    clp.addTriggerOption("debugMode", {"-d", "--debug"}, "Run in debug mode (more detailed than verbose)");
    clp.addDummyOption();
    
    LogInfo << clp.getDescription().str() << std::endl;
    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;
    
    clp.parseCmdLine(argc, argv);
    LogThrowIf( clp.isNoOptionTriggered(), "No option was provided." );

    bool verboseMode = clp.isOptionTriggered("verboseMode");
    bool debugMode = clp.isOptionTriggered("debugMode");
    if (debugMode) verboseMode = true;
    
    // Load parameters
    ParametersManager::getInstance().loadParameters();

    std::string json = clp.getOptionVal<std::string>("json");
    std::ifstream i(json); 
    LogThrowIf(!i.good(), "Failed to open JSON config: " << json);
    nlohmann::json j; 
    i >> j;

    // Parse JSON configuration
    std::string signal_type = j.value("signal_type", std::string("cc")); // "cc" or "es"
    bool around_vertex_only = j.value("around_vertex_only", false);
    double vertex_radius = j.value("vertex_radius", 100.0); // cm, used if around_vertex_only=true
    int max_files = j.value("max_files", -1); // -1 means no limit
    std::string bkg_folder = j.value("bkg_folder", std::string("/eos/user/e/evilla/dune/sn-tps/bkgs/"));
    
    // Determine signal input folder based on signal_type
    std::string signal_folder;
    if (signal_type == "cc") {
        signal_folder = j.value("signal_folder", std::string("/eos/user/e/evilla/dune/sn-tps/production_cc/"));
    } else if (signal_type == "es") {
        signal_folder = j.value("signal_folder", std::string("/eos/user/e/evilla/dune/sn-tps/production_es/"));
    } else {
        LogThrow("Invalid signal_type: " << signal_type << ". Must be 'cc' or 'es'.");
    }

    LogInfo << "Configuration:" << std::endl;
    LogInfo << " - Signal type: " << signal_type << std::endl;
    LogInfo << " - Signal folder: " << signal_folder << std::endl;
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

    // Find background files (looking for BG_*_tps.root pattern)
    std::vector<std::string> bkg_files;
    if (std::filesystem::exists(bkg_folder) && std::filesystem::is_directory(bkg_folder)) {
        for (const auto& entry : std::filesystem::directory_iterator(bkg_folder)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().string();
                // Look for files matching BG_*_tps.root pattern
                if (filename.find("BG_") != std::string::npos && filename.find("_tps.root") != std::string::npos) {
                    bkg_files.push_back(filename);
                }
            }
        }
    } else {
        LogThrow("Background folder does not exist: " << bkg_folder);
    }
    LogInfo << "Found " << bkg_files.size() << " background files" << std::endl;
    LogThrowIf(bkg_files.empty(), "No background files found.");

    // Load all background TPs into memory for random sampling
    LogInfo << "Loading background TPs into memory..." << std::endl;
    std::map<int, std::vector<TriggerPrimitive>> all_bkg_tps_by_event;
    int bkg_event_counter = 0;
    
    for (size_t bkg_idx = 0; bkg_idx < bkg_files.size(); bkg_idx++) {
        const auto& bkg_file = bkg_files[bkg_idx];
        if (verboseMode) {
            GenericToolbox::displayProgressBar(bkg_idx + 1, bkg_files.size(), "Loading background files...");
        }
        
        std::map<int, std::vector<TriggerPrimitive>> tps_by_event;
        std::map<int, std::vector<TrueParticle>> true_by_event; // not used
        std::map<int, std::vector<Neutrino>> nu_by_event; // not used
        
        read_tps(bkg_file, tps_by_event, true_by_event, nu_by_event);
        
        // Merge into global background map with unique event numbers
        for (const auto& kv : tps_by_event) {
            all_bkg_tps_by_event[bkg_event_counter] = kv.second;
            bkg_event_counter++;
        }
    }
    LogInfo << "\nLoaded " << all_bkg_tps_by_event.size() << " background events" << std::endl;
    
    // Create list of background event IDs for random selection
    std::vector<int> bkg_event_ids;
    for (const auto& kv : all_bkg_tps_by_event) {
        bkg_event_ids.push_back(kv.first);
    }

    // Initialize random number generator
    std::mt19937 rng(std::time(nullptr));
    std::uniform_int_distribution<int> bkg_dist(0, bkg_event_ids.size() - 1);

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
        std::string output_filename = signal_path.parent_path().string() + "/" + signal_path.stem().string();
        if (around_vertex_only) {
            output_filename += "_bkg_vtx" + std::to_string((int)vertex_radius) + ".root";
        } else {
            output_filename += "_bkg.root";
        }
        
        if (verboseMode) LogInfo << "Output file: " << output_filename << std::endl;
        
        // Prepare merged data structures - use maps first to avoid sparse vectors
        std::map<int, std::vector<TriggerPrimitive>> merged_tps_map;
        std::map<int, std::vector<TrueParticle>> merged_true_map;
        std::map<int, std::vector<Neutrino>> merged_nu_map;
        
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
            
            // Select a random background event
            if (!bkg_event_ids.empty()) {
                int random_idx = bkg_dist(rng);
                int random_bkg_event_id = bkg_event_ids[random_idx];
                const auto& bkg_tps = all_bkg_tps_by_event.at(random_bkg_event_id);
                
                if (debugMode) {
                    LogInfo << "Event " << event_id << ": merging with background event " << random_bkg_event_id
                            << " (" << bkg_tps.size() << " TPs)" << std::endl;
                }
                
                int bkg_added = 0;
                // Add background TPs (for now, skip position filtering - would need geometry info)
                // If around_vertex_only is set but we can't get TP positions, add all background TPs
                // TODO: implement proper position-based filtering using geometry
                for (const auto& bkg_tp : bkg_tps) {
                    TriggerPrimitive tp = bkg_tp;
                    
                    // Clear truth link for background TPs (truth info doesn't apply in signal file context)
                    tp.SetTrueParticle(nullptr);
                    
                    // Update event number to match signal event
                    tp.SetEvent(event_id);
                    merged_tps.push_back(tp);
                    bkg_added++;
                }
                
                if (around_vertex_only && has_vertex) {
                    LogWarning << "around_vertex_only is enabled but TP position filtering not yet implemented. Adding all background TPs." << std::endl;
                }
                
                if (debugMode) {
                    LogInfo << "Event " << event_id << ": " << signal_tps.size() << " signal TPs + "
                            << bkg_added << " background TPs = "
                            << merged_tps.size() << " total TPs" << std::endl;
                }
            }
            
            merged_tps_map[event_id] = merged_tps;
            
            // Preserve truth information
            if (signal_true_by_event.count(event_id) > 0) {
                merged_true_map[event_id] = signal_true_by_event.at(event_id);
            }
            if (signal_nu_by_event.count(event_id) > 0) {
                merged_nu_map[event_id] = signal_nu_by_event.at(event_id);
            }
        }
        
        // Convert maps to vectors for write_tps
        // Find max event ID
        int max_event = 0;
        for (const auto& kv : merged_tps_map) {
            if (kv.first > max_event) max_event = kv.first;
        }
        
        std::vector<std::vector<TriggerPrimitive>> merged_tps_vec(max_event + 1);
        std::vector<std::vector<TrueParticle>> merged_true_vec(max_event + 1);
        std::vector<std::vector<Neutrino>> merged_nu_vec(max_event + 1);
        
        for (const auto& kv : merged_tps_map) {
            merged_tps_vec[kv.first] = kv.second;
        }
        for (const auto& kv : merged_true_map) {
            merged_true_vec[kv.first] = kv.second;
        }
        for (const auto& kv : merged_nu_map) {
            merged_nu_vec[kv.first] = kv.second;
        }
        
        // Write output file with merged TPs (preserving signal truth information)
        write_tps(output_filename, merged_tps_vec, merged_true_vec, merged_nu_vec);
        output_files.push_back(output_filename);
        
        if (verboseMode) LogInfo << "Wrote: " << output_filename << std::endl;
    }
    
    LogInfo << "\n\nProcessed " << file_count << " files successfully." << std::endl;
    LogInfo << "Output files are in the same directories as input files with '_bkg' suffix." << std::endl;
    
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
