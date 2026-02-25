#include "Clustering.h"

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

namespace {

bool has_all_view_trees(TDirectory* dir) {
    if (dir == nullptr) return false;
    for (const auto& view : APA::views) {
        const std::string tree_name = "clusters_tree_" + view;
        if (dir->Get(tree_name.c_str()) == nullptr) {
            return false;
        }
    }
    return true;
}

bool is_valid_clusters_output_file(const std::string& file_path) {
    TFile in_file(file_path.c_str(), "READ");
    if (in_file.IsZombie()) {
        return false;
    }

    auto* clusters_dir = dynamic_cast<TDirectory*>(in_file.Get("clusters"));
    auto* discarded_dir = dynamic_cast<TDirectory*>(in_file.Get("discarded"));
    if (clusters_dir == nullptr || discarded_dir == nullptr) {
        return false;
    }

    return has_all_view_trees(clusters_dir) && has_all_view_trees(discarded_dir);
}

}

int main(int argc, char* argv[]) {
    CmdLineParser clp;
    clp.getDescription() << "> Cluster app - build clusters from *_tps.root files."<< std::endl;
    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");
    clp.addOption("inputFile", {"-i", "--input-file"}, "Input file with list OR single ROOT file path (overrides JSON inputs)");
    clp.addOption("skip_files", {"-s", "--skip", "--skip-files"}, "Number of files to skip at start (overrides JSON)", -1);
    clp.addOption("max_files", {"-m", "--max", "--max-files"}, "Maximum number of files to process (overrides JSON)", -1);
    clp.addOption("apa", {"-a", "--apa", "--apa-filter"}, "Filter TPs by APA index (e.g. 1 for APA1). Use -1 to disable.", -1);
    clp.addOption("override", {"-f", "--override"}, "Override existing output files (default: false)", false);
    clp.addOption("outFolder", {"--output-folder"}, "Output folder path (default: data)");
    clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");
    clp.addTriggerOption("debugMode", {"-d"}, "Run in debug mode (more detailed than verbose)");
    clp.addDummyOption();
    LogInfo << clp.getDescription().str() << std::endl;
    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;
    clp.parseCmdLine(argc, argv);
    LogThrowIf( clp.isNoOptionTriggered(), "No option was provided." );

    // verbosity updating global variables
    if (clp.isOptionTriggered("verboseMode")) { verboseMode = true; }
    if (clp.isOptionTriggered("debugMode")) { verboseMode =true; debugMode = true; }

    // Load parameters
    ParametersManager::getInstance().loadParameters();

    std::string json = clp.getOptionVal<std::string>("json");
    std::ifstream i(json); nlohmann::json j; i >> j;

    bool overrideExistingFiles = false;
    if (clp.isOptionTriggered("override")) { overrideExistingFiles = true; }

    // Handle skip and max files with CLI override BEFORE file finding
    int max_files = j.value("max_files", -1);
    int skip_files = j.value("skip_files", 0);
    int apa_filter = -1;
    
    if (clp.isOptionTriggered("skip_files")) {
        skip_files = clp.getOptionVal<int>("skip_files");
    }
    if (clp.isOptionTriggered("max_files")) {
        max_files = clp.getOptionVal<int>("max_files");
    }
    if (clp.isOptionTriggered("apa")) {
        apa_filter = clp.getOptionVal<int>("apa");
    }

    // Use tpstream-based file tracking for consistent skip/max across pipeline
    // By default, load tps_bg files (TPs with backgrounds merged)
    std::vector<std::string> inputs = find_input_files_by_tpstream_basenames(j, "tps_bg", skip_files, max_files);
    
    // Override with CLI input if provided
    if (clp.isOptionTriggered("inputFile")) {
        std::string input_file = clp.getOptionVal<std::string>("inputFile");
        inputs.clear();
        if (input_file.find("_tps.root") != std::string::npos) {
            inputs.push_back(input_file);
        } else {
            std::ifstream lf(input_file);
            std::string line;
            while (std::getline(lf, line)) {
                if (!line.empty() && line[0] != '#') {
                    inputs.push_back(line);
                }
            }
        }
    }

    LogInfo << "Found " << inputs.size() << " files with backgrounds (tps_bg)" << std::endl;
    LogThrowIf(inputs.empty(), "No tps_bg files found. Please run add_backgrounds step first to merge signal and background TPs.");
    
    // Print input folder for debugging
    if (!inputs.empty()) {
        std::string first_file = inputs[0];
        size_t last_slash = first_file.find_last_of("/");
        std::string input_folder = (last_slash != std::string::npos) ? first_file.substr(0, last_slash) : ".";
        LogInfo << "Input folder: " << input_folder << std::endl;
    }

    // Get output folder: CLI > clusters_folder > outputFolder > default
    std::string outfolder;
    if (clp.isOptionTriggered("outFolder")) {
        outfolder = clp.getOptionVal<std::string>("outFolder");
        LogInfo << "Using output folder from CLI: " << outfolder << std::endl;
    } else if (j.contains("clusters_folder")) {
        outfolder = j.value("clusters_folder", std::string("data"));
    } else {
        outfolder = j.value("outputFolder", std::string("data"));
    }

    int tick_limit = j.value("tick_limit", 3);
    int channel_limit = j.value("channel_limit", 1);
    int min_tps_to_cluster = j.value("min_tps_to_cluster", 1);
    float energy_cut = 0.0f;
    if (j.contains("energy_cut")) {
        try {
            energy_cut = j.at("energy_cut").get<float>();
        } catch (const std::exception&) {
            // Fallback: try reading as double and cast to float
            energy_cut = static_cast<float>(j.at("energy_cut").get<double>());
        }
    }
    float adc_integral_cut_col = energy_cut * ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_collection");
    float adc_integral_cut_ind = energy_cut * ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_induction");
    int tot_cut = j.value("tot_cut", 0);

    // Get output folder: CLI > clusters_folder > outputFolder > default
    std::string clusters_folder_path;
    if (clp.isOptionTriggered("outFolder")) {
        // CLI override - build full path manually
        clusters_folder_path = getClustersFolder(j);
    } else {
        // Use new auto-generation logic
        clusters_folder_path = getOutputFolder(j, "clusters", "clusters_folder");
    }

    LogInfo << "Settings from json file:" << std::endl;
    LogInfo << " - Clusters output path: " << clusters_folder_path << std::endl;
    LogInfo << " - Tick limit: " << tick_limit << std::endl;
    LogInfo << " - Channel limit: " << channel_limit << std::endl;
    LogInfo << " - Minimum TPs to form a cluster: " << min_tps_to_cluster << std::endl;
    LogInfo << " - Energy cut: " << energy_cut << std::endl;
    LogInfo << "    - ADC integral cut (induction): " << adc_integral_cut_ind << std::endl;
    LogInfo << "    - ADC integral cut (collection): " << adc_integral_cut_col << std::endl;
    LogInfo << " - ToT cut: " << tot_cut << std::endl;
    LogInfo << " - APA filter: " << (apa_filter >= 0 ? std::to_string(apa_filter) : std::string("disabled")) << std::endl;
    LogInfo << " - Files to process (after skip/max): " << inputs.size() << std::endl;

    // Create clusters subfolder if it doesn't exist
    std::filesystem::create_directories(clusters_folder_path);    

    // Helper lambda to create metadata tree
    auto create_metadata_tree = [&](TFile* file) {
        file->cd();
        TTree* metadata_tree = new TTree("clustering_metadata", "Clustering parameters used");
        
        int meta_tick_limit = tick_limit;
        int meta_channel_limit = channel_limit;
        int meta_min_tps = min_tps_to_cluster;
        int meta_adc_cut_ind = adc_integral_cut_ind;
        int meta_adc_cut_col = adc_integral_cut_col;
        int meta_tot_cut = tot_cut;
        float meta_energy_cut = energy_cut;
        float meta_adc_to_mev_collection = ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_collection");
        float meta_adc_to_mev_induction = ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_induction");
        
        metadata_tree->Branch("tick_limit", &meta_tick_limit, "tick_limit/I");
        metadata_tree->Branch("channel_limit", &meta_channel_limit, "channel_limit/I");
        metadata_tree->Branch("min_tps_to_cluster", &meta_min_tps, "min_tps_to_cluster/I");
        metadata_tree->Branch("adc_integral_cut_induction", &meta_adc_cut_ind, "adc_integral_cut_induction/I");
        metadata_tree->Branch("adc_integral_cut_collection", &meta_adc_cut_col, "adc_integral_cut_collection/I");
        metadata_tree->Branch("tot_cut", &meta_tot_cut, "tot_cut/I");
        metadata_tree->Branch("energy_cut", &meta_energy_cut, "energy_cut/F");
        metadata_tree->Branch("adc_to_mev_collection", &meta_adc_to_mev_collection, "adc_to_mev_collection/F");
        metadata_tree->Branch("adc_to_mev_induction", &meta_adc_to_mev_induction, "adc_to_mev_induction/F");
        
        metadata_tree->Fill();
        metadata_tree->Write();
        
        return true;
    };

    std::vector<std::string> produced_files;
    int done_files = 0;

    for (const auto& tps_file : inputs) {
        // Generate output filename: replace "_tps.root" with "_clusters.root"
        std::filesystem::path tps_path(tps_file);
        std::string base_name = tps_path.filename().string();
        
        // Replace _tps.root with _clusters.root
        size_t tps_pos = base_name.find("_tps.root");
        if (tps_pos == std::string::npos) {
            LogWarning << "File doesn't match expected *_tps.root pattern: " << tps_file << std::endl;
            continue;
        }
        base_name.replace(tps_pos, 9, "_clusters.root");
        
        std::string current_clusters_filename = clusters_folder_path + "/" + base_name;
        
        // Check if output already exists
        if (std::filesystem::exists(current_clusters_filename) && !overrideExistingFiles) {
            if (is_valid_clusters_output_file(current_clusters_filename)) {
                LogInfo << "Output file already exists (use -f to override): " << current_clusters_filename << std::endl;
                done_files++;
                continue;
            }

            LogWarning << "Existing output file is incomplete/corrupted, regenerating: " << current_clusters_filename << std::endl;
            std::error_code ec;
            std::filesystem::remove(current_clusters_filename, ec);
            if (ec) {
                LogError << "Failed to remove invalid output file: " << current_clusters_filename << " (" << ec.message() << ")" << std::endl;
                done_files++;
                continue;
            }
        }
        
        if (verboseMode) LogInfo << "Input TPs file: " << tps_file << std::endl;
        if (verboseMode) LogInfo << "Output clusters file: " << current_clusters_filename << std::endl;

        done_files++;
        
        GenericToolbox::displayProgressBar(done_files, (int)inputs.size(), "Making clusters...");

        // Read TPs
        std::map<int, std::vector<TriggerPrimitive>> tps_by_event;
        std::map<int, std::vector<TrueParticle>> true_by_event;
        std::map<int, std::vector<Neutrino>> nu_by_event;
        
        read_tps(tps_file, tps_by_event, true_by_event, nu_by_event);

        if (apa_filter >= 0) {
            for (auto& kv : tps_by_event) {
                auto& vec = kv.second;
                vec.erase(
                    std::remove_if(vec.begin(), vec.end(), [&](const TriggerPrimitive &tp){ return tp.GetDetector() != apa_filter; }),
                    vec.end()
                );
            }
        }

        // Apply ToT cut to TPs if requested
        if (tot_cut > 0) {
            for (auto &kv : tps_by_event) {
                auto &vec = kv.second;
                vec.erase(
                    std::remove_if(vec.begin(), vec.end(), [&](const TriggerPrimitive &tp){ return (int)tp.GetSamplesOverThreshold() <= tot_cut; }),
                    vec.end()
                );
            }
        }

        // Create output file
        TFile* clusters_file = new TFile(current_clusters_filename.c_str(), "RECREATE");
        if (!clusters_file || clusters_file->IsZombie()) {
            LogError << "Failed to create output file: " << current_clusters_filename << std::endl;
            if (clusters_file) delete clusters_file;
            continue;
        }

        // Create directories for clusters and discarded clusters
        clusters_file->mkdir("clusters");
        clusters_file->mkdir("discarded");

        // Cluster ID counter (unique per file, shared across all views)
        int next_cluster_id = 0;

        // Process events
        for (auto& kv : tps_by_event) {
            int event = kv.first;
            auto& tps = kv.second;
            
            // split by view
            std::vector<std::vector<TriggerPrimitive*>> tps_per_view;
            tps_per_view.reserve(APA::views.size());
            for (size_t iView=0;iView<APA::views.size();++iView){ 
                std::vector<TriggerPrimitive*> v; 
                getPrimitivesForView(APA::views.at(iView), tps, v); 
                tps_per_view.emplace_back(std::move(v)); 
            }

            std::vector<std::vector<Cluster>> clusters_per_view; 
            clusters_per_view.reserve(APA::views.size());
            std::vector<int> adc_cut = {static_cast<int>(adc_integral_cut_ind), 
                                        static_cast<int>(adc_integral_cut_ind), 
                                        static_cast<int>(adc_integral_cut_col)};
            
            for (size_t iView=0;iView<APA::views.size();++iView)
                clusters_per_view.emplace_back(make_cluster(tps_per_view.at(iView), 
                                                tick_limit, 
                                                channel_limit, 
                                                min_tps_to_cluster, 
                                                adc_cut.at(iView)));
            
            // Identify the main marley cluster (most energetic) in each view for this event
            for (size_t iView=0; iView<APA::views.size(); ++iView) {
                auto& clusters = clusters_per_view.at(iView);
                if (clusters.empty()) continue;
                
                // Find cluster with highest reconstructed energy (not true particle energy)
                Cluster* main_cluster = nullptr;
                float max_energy = -1.0f;
                
                if (debugMode) {
                    LogInfo << "Event " << event << " View " << APA::views.at(iView) 
                            << " - Selecting main cluster from " << clusters.size() << " clusters" << std::endl;
                }
                
                for (auto& cluster : clusters) {
                    if (cluster.get_true_label() != "marley") continue; // only consider marley clusters
                    float energy = cluster.get_total_energy();
                    
                    if (debugMode) {
                        LogInfo << "  Candidate cluster: reco_energy=" << energy << " MeV"
                                << ", true_particle_energy=" << cluster.get_true_particle_energy() << " MeV"
                                << ", true_pdg=" << cluster.get_true_pdg()
                                << ", n_tps=" << cluster.get_size()
                                << ", true_label=" << cluster.get_true_label() << std::endl;
                    }
                    
                    if (energy > max_energy) {
                        max_energy = energy;
                        main_cluster = &cluster;
                    }
                }
                
                // Mark the main cluster
                if (main_cluster != nullptr) {
                    main_cluster->set_is_main_cluster(true);
                    
                    if (debugMode) {
                        LogInfo << "  SELECTED as main cluster: reco_energy=" << main_cluster->get_total_energy() << " MeV"
                                << ", true_particle_energy=" << main_cluster->get_true_particle_energy() << " MeV"
                                << ", true_pdg=" << main_cluster->get_true_pdg()
                                << ", n_tps=" << main_cluster->get_size()
                                << ", is_electron=" << (main_cluster->get_true_pdg() == 11 ? "YES" : "NO") << std::endl;
                    }
                }
            }

            // Separate clusters into accepted and discarded based on energy_cut
            for (size_t iView=0;iView<APA::views.size();++iView) {
                std::vector<Cluster> accepted_clusters;
                std::vector<Cluster> discarded_clusters;
                
                for (auto& cluster : clusters_per_view.at(iView)) {
                    // Assign unique cluster ID
                    cluster.set_cluster_id(next_cluster_id++);
                    
                    // Get cluster energy in MeV
                    float cluster_energy_mev = 0.0f;
                    if (APA::views.at(iView) == "X") {
                        cluster_energy_mev = cluster.get_total_charge() / ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_collection");
                    } else {
                        cluster_energy_mev = cluster.get_total_charge() / ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_induction");
                    }
                    
                    if (cluster_energy_mev >= energy_cut) {
                        accepted_clusters.push_back(cluster);
                    } else {
                        discarded_clusters.push_back(cluster);
                    }
                }
                
                // Write accepted clusters to clusters/ folder
                clusters_file->cd("clusters");
                write_clusters(accepted_clusters, clusters_file, APA::views.at(iView));
                
                // Write discarded clusters to discarded/ folder
                clusters_file->cd("discarded");
                write_clusters(discarded_clusters, clusters_file, APA::views.at(iView));
            }
        }

        // Write metadata and close
        LogInfo << "Writing clustering metadata..." << std::endl;
        create_metadata_tree(clusters_file);
        clusters_file->Close();
        delete clusters_file;
        
        produced_files.push_back(current_clusters_filename);
        if (verboseMode) LogInfo << "Closed output file: " << current_clusters_filename << std::endl;
    }

    // Print summary
    LogInfo << "\nClustering complete! Generated " << produced_files.size() << " output file(s):" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(5, produced_files.size()); i++) {
        LogInfo << "  - " << produced_files[i] << std::endl;
    }
    if (produced_files.size() > 5) {
        LogInfo << "  ... and " << (produced_files.size() - 5) << " more" << std::endl;
    }
    
    return 0;
}
