#include "Clustering.h"

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

int main(int argc, char* argv[]) {
    CmdLineParser clp;
    clp.getDescription() << "> add_backgrounds app - merge signal TPs with random background events, writing *_tps_bkg.root files."<< std::endl;
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

    // Set verbosity based on command line options
    if (clp.isOptionTriggered("verboseMode")) { verboseMode = true; }
    if (clp.isOptionTriggered("debugMode")) { verboseMode = true; debugMode = true; }

    // Load parameters
    ParametersManager::getInstance().loadParameters();

    std::string json = clp.getOptionVal<std::string>("json");
    std::ifstream i(json); 
    LogThrowIf(!i.good(), "Failed to open JSON config: " << json);
    nlohmann::json j; i >> j;

    // Parse JSON configuration
    std::string signal_type = j.value("signal_type", std::string("cc")); // "cc" or "es"
    bool around_vertex_only = j.value("around_vertex_only", false);
    int vertex_radius = j.value("vertex_radius", 100); // cm, used if around_vertex_only=true
    int max_files = j.value("max_files", -1); // -1 means no limit
    std::string bkg_folder = j.value("bkg_folder", std::string("/eos/user/e/evilla/dune/sn-tps/bkgs/"));
    
    // Determine signal input folder based on signal_type
    std::string signal_folder;
    if (signal_type == "cc") {
        signal_folder = j.value("signal_folder", std::string("/eos/user/e/evilla/dune/sn-tps/cc/"));
    } else if (signal_type == "es") {
        signal_folder = j.value("signal_folder", std::string("/eos/user/e/evilla/dune/sn-tps/es/"));
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

    // Find signal files
    std::vector<std::string> signal_files = find_input_files(j, "_tps.root");
    LogInfo << "Found " << signal_files.size() << " signal files" << std::endl;
    LogThrowIf(signal_files.empty(), "No signal files found.");

    // Find background files
    std::vector<std::string> bkg_files;
    if (std::filesystem::exists(bkg_folder) && std::filesystem::is_directory(bkg_folder)) {
        for (const auto& entry : std::filesystem::directory_iterator(bkg_folder)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().string();
                if (filename.find("_tps.root") != std::string::npos) {
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
    int bkg_file_count = 0;
    for (const auto& bkg_file : bkg_files) {
        bkg_file_count++;
        if (verboseMode) {
            GenericToolbox::displayProgressBar(bkg_file_count, bkg_files.size(), "Loading background files...");
        }
        
        std::map<int, std::vector<TriggerPrimitive>> tps_by_event;
        std::map<int, std::vector<TrueParticle>> true_by_event; // not used
        std::map<int, std::vector<Neutrino>> nu_by_event; // not used
        
        read_tps(bkg_file, tps_by_event, true_by_event, nu_by_event);
        
        // Merge into global background map with unique event numbers
        int event_offset = bkg_file_count * 100000; // ensure unique event IDs
        for (const auto& kv : tps_by_event) {
            int new_event_id = event_offset + kv.first;
            all_bkg_tps_by_event[new_event_id] = kv.second;
        }
    }
    LogInfo << "Loaded " << all_bkg_tps_by_event.size() << " background events" << std::endl;

    // Create list of background event IDs for random selection
    std::vector<int> bkg_event_ids;
    for (const auto& kv : all_bkg_tps_by_event) {
        bkg_event_ids.push_back(kv.first);
    }

    // Initialize random number generator
    std::srand(std::time(nullptr));

    // Process signal files
    int file_count = 0;
    for (const auto& signal_file : signal_files) {
        // Check if we've reached max_files limit
        if (max_files > 0 && file_count >= max_files) {
            LogInfo << "Reached max_files limit (" << max_files << "), stopping." << std::endl;
            break;
        }
        
        file_count++;
        GenericToolbox::displayProgressBar(file_count, signal_files.size(), "Adding backgrounds...");
        
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
            output_filename += "_bkg_vertex" + std::to_string(vertex_radius) + ".root";
        } else {
            output_filename += "_bkg.root";
        }
        
        if (verboseMode) LogInfo << "Output file: " << output_filename << std::endl;

        // Prepare merged data structures
        std::map<int, std::vector<TriggerPrimitive>> merged_tps_by_event;
        
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
                    vertex_pos = neutrinos[0].GetPosition();
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
                int random_idx = std::rand() % bkg_event_ids.size();
                int random_bkg_event_id = bkg_event_ids[random_idx];
                const auto& bkg_tps = all_bkg_tps_by_event.at(random_bkg_event_id);
                
                if (debugMode) {
                    LogInfo << "Event " << event_id << ": merging with background event " << random_bkg_event_id 
                            << " (" << bkg_tps.size() << " TPs)" << std::endl;
                }
                
                // Add background TPs
                for (const auto& bkg_tp : bkg_tps) {
                    TriggerPrimitive tp = bkg_tp;
                    
                    // If around_vertex_only, check distance from vertex
                    if (around_vertex_only && has_vertex) {
                        TVector3 tp_pos = tp.GetPosition();
                        double distance = (tp_pos - vertex_pos).Mag();
                        if (distance > vertex_radius) {
                            continue; // Skip TPs outside radius
                        }
                    }
                    
                    // Update event number to match signal event
                    tp.SetEvent(event_id);
                    merged_tps.push_back(tp);
                }
            }
            
            merged_tps_by_event[event_id] = merged_tps;
            
            if (debugMode) {
                LogInfo << "Event " << event_id << ": " << signal_tps.size() << " signal TPs + " 
                        << (merged_tps.size() - signal_tps.size()) << " background TPs = " 
                        << merged_tps.size() << " total TPs" << std::endl;
            }
        }
        
        // Write output file with merged TPs (preserving signal truth information)
        write_tps(output_filename, merged_tps_by_event, signal_true_by_event, signal_nu_by_event);
        
        if (verboseMode) LogInfo << "Wrote: " << output_filename << std::endl;
    }
    
    LogInfo << "\nProcessed " << file_count << " files successfully." << std::endl;
    LogInfo << "Output files are in the same directories as input files with '_bkg' suffix." << std::endl;
    
    return 0;
}
