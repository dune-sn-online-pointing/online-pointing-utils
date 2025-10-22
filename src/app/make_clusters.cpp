#include "Clustering.h"

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

int main(int argc, char* argv[]) {
    CmdLineParser clp;
    clp.getDescription() << "> Cluster app - build clusters from *_tps.root files."<< std::endl;
    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");
    clp.addOption("inputFile", {"-i", "--input-file"}, "Input file with list OR single ROOT file path (overrides JSON inputs)");
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

    // Use utility function for file finding (reads from tps_folder = merged TPs with backgrounds)
    std::vector<std::string> inputs = find_input_files(j, "tps");
    
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

    LogInfo << "Number of valid files (merged TPs): " << inputs.size() << std::endl;
    LogThrowIf(inputs.empty(), "No valid input files found in tps_folder.");

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
    int energy_cut = j.value("energy_cut", 0);
    int adc_integral_cut_col = energy_cut * ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_collection");
    int adc_integral_cut_ind = energy_cut * ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_induction");
    int tot_cut = j.value("tot_cut", 0);
    int max_files = j.value("max_files", -1); // -1 means no limit

    // Get cluster folder prefix
    std::string cluster_prefix = j.value("clusters_folder_prefix", std::string("clusters"));

    // Build subfolder name with clustering conditions
    std::string clusters_subfolder = "clusters_" + cluster_prefix
        + "_tick" + std::to_string(tick_limit)
        + "_ch" + std::to_string(channel_limit)
        + "_min" + std::to_string(min_tps_to_cluster)
        + "_tot" + std::to_string(tot_cut)
        + "_e" + std::to_string(energy_cut);
    
    std::string clusters_folder_path = outfolder + "/" + clusters_subfolder;

    LogInfo << "Settings from json file:" << std::endl;
    LogInfo << " - Base output folder: " << outfolder << std::endl;
    LogInfo << " - Clusters subfolder: " << clusters_subfolder << std::endl;
    LogInfo << " - Full clusters path: " << clusters_folder_path << std::endl;
    LogInfo << " - Tick limit: " << tick_limit << std::endl;
    LogInfo << " - Channel limit: " << channel_limit << std::endl;
    LogInfo << " - Minimum TPs to form a cluster: " << min_tps_to_cluster << std::endl;
    LogInfo << " - Energy cut: " << energy_cut << std::endl;
    LogInfo << "    - ADC integral cut (induction): " << adc_integral_cut_ind << std::endl;
    LogInfo << "    - ADC integral cut (collection): " << adc_integral_cut_col << std::endl;
    LogInfo << " - ToT cut: " << tot_cut << std::endl;
    if (max_files > 0) {
        LogInfo << " - Max files to process: " << max_files << std::endl;
    } else {
        LogInfo << " - Max files to process: unlimited" << std::endl;
        max_files = inputs.size();
    }

    // Create clusters subfolder if it doesn't exist
    std::filesystem::create_directories(clusters_folder_path);    

    // Ensure output directory exists
    // Variables for numbered output files (one per 10 input files)
    const int FILES_PER_OUTPUT = 10;
    int output_file_number = 0;
    TFile* clusters_file = nullptr;
    std::string current_clusters_filename;
    std::vector<std::string> produced_files;

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
        
        metadata_tree->Branch("tick_limit", &meta_tick_limit, "tick_limit/I");
        metadata_tree->Branch("channel_limit", &meta_channel_limit, "channel_limit/I");
        metadata_tree->Branch("min_tps_to_cluster", &meta_min_tps, "min_tps_to_cluster/I");
        metadata_tree->Branch("adc_integral_cut_induction", &meta_adc_cut_ind, "adc_integral_cut_induction/I");
        metadata_tree->Branch("adc_integral_cut_collection", &meta_adc_cut_col, "adc_integral_cut_collection/I");
        metadata_tree->Branch("tot_cut", &meta_tot_cut, "tot_cut/I");
        
        metadata_tree->Fill();
        metadata_tree->Write("", TObject::kOverwrite);
    };

    // Helper lambda to create a new output file
    auto create_new_output_file = [&]() -> bool {
        // Close previous file if open
        if (clusters_file) {
            LogInfo << "Writing clustering metadata..." << std::endl;
            create_metadata_tree(clusters_file);
            clusters_file->Close();
            delete clusters_file;
            LogInfo << "Closed output file: " << current_clusters_filename << std::endl;
        }
        
        // Create new filename with simple pattern: clusters_N.root
        current_clusters_filename = clusters_folder_path + "/clusters_" + std::to_string(output_file_number) + ".root";
        
        // Delete if exists
        if (std::filesystem::exists(current_clusters_filename)) {
            LogInfo << "Output file " << current_clusters_filename << " already exists, deleting it." << std::endl;
            std::filesystem::remove(current_clusters_filename);
        }
        
        // Open new file
        clusters_file = new TFile(current_clusters_filename.c_str(), "RECREATE");
        if (!clusters_file || clusters_file->IsZombie()) {
            LogError << "Cannot create output file: " << current_clusters_filename << std::endl;
            return false;
        }
        
        LogInfo << "Opened new output file: " << current_clusters_filename << std::endl;
        produced_files.push_back(current_clusters_filename);
        output_file_number++;
        return true;
    };

    // Create first output file
    if (!create_new_output_file()) {
        return 1;
    }

    int file_count = 0;

    for (const auto& tps_file : inputs) {

        // Check if we've reached max_files limit
        if (file_count >= max_files) {
            LogInfo << "Reached max_files limit (" << max_files << "), stopping." << std::endl;
            break;
        }
        
        if (verboseMode) LogInfo << "Input TPs file: " << tps_file << std::endl;

        file_count++;
        
        // Create new output file every FILES_PER_OUTPUT input files
        if (file_count > 1 && (file_count - 1) % FILES_PER_OUTPUT == 0) {
            if (!create_new_output_file()) {
                return 1;
            }
        }
        
        int progress = (file_count * 100) / max_files;
        GenericToolbox::displayProgressBar(file_count, max_files, "Making clusters...");

        std::map<int, std::vector<TriggerPrimitive>> tps_by_event;
        std::map<int, std::vector<TrueParticle>> true_by_event;
        std::map<int, std::vector<Neutrino>> nu_by_event;
        
        read_tps(tps_file, tps_by_event, true_by_event, nu_by_event);

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

        for (auto& kv : tps_by_event) {
            int event = kv.first;
            auto& tps = kv.second;
            // split by view
            std::vector<std::vector<TriggerPrimitive*>> tps_per_view;
            tps_per_view.reserve(APA::views.size());
            for (size_t iView=0;iView<APA::views.size();++iView){ 
                std::vector<TriggerPrimitive*> v; 
                getPrimitivesForView(APA::views.at(iView), tps, v); tps_per_view.emplace_back(std::move(v)); 
            }

            std::vector<std::vector<Cluster>> clusters_per_view; clusters_per_view.reserve(APA::views.size());
            std::vector<int> adc_cut = {adc_integral_cut_ind, adc_integral_cut_ind, adc_integral_cut_col};
            
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
                
                // Find cluster with highest true particle energy
                Cluster* main_cluster = nullptr;
                float max_energy = -1.0f;
                for (auto& cluster : clusters) {
                    if (cluster.get_true_label() != "marley") continue; // only consider marley clusters
                    float energy = cluster.get_true_particle_energy();
                    if (energy > max_energy) {
                        max_energy = energy;
                        main_cluster = &cluster;
                    }
                }
                
                // Mark the main cluster
                if (main_cluster != nullptr) {
                    main_cluster->set_is_main_cluster(true);
                }
            }

            for (size_t iView=0;iView<APA::views.size();++iView)
                write_clusters(clusters_per_view.at(iView), clusters_file, APA::views.at(iView)); 
        }

    }


    // Close final output file
    if (clusters_file) {
        LogInfo << "Writing clustering metadata..." << std::endl;
        create_metadata_tree(clusters_file);
        clusters_file->Close();
        delete clusters_file;
        LogInfo << "Closed final output file: " << current_clusters_filename << std::endl;
    }
    
    // Print summary
    LogInfo << "\nClustering complete! Generated " << produced_files.size() << " output file(s):" << std::endl;
    for (const auto& f : produced_files) {
        LogInfo << "  - " << f << std::endl;
    }
    return 0;
}