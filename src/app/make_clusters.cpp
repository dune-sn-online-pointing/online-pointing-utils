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

    // Use utility function for file finding
    std::vector<std::string> patterns = {"_tps.root"};
    std::vector<std::string> inputs = find_input_files(j, patterns);
    
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

    LogInfo << "Number of valid files: " << inputs.size() << std::endl;
    LogThrowIf(inputs.empty(), "No valid input files found.");

    std::string outfolder = j.value("outputFolder", std::string("data"));
    if (clp.isOptionTriggered("outFolder")) {
        outfolder = clp.getOptionVal<std::string>("outFolder");
        LogInfo << "Overriding output folder with CLI option: " << outfolder << std::endl;
    }

    int ticks_limit = j.value("tick_limit", 3);
    int channel_limit = j.value("channel_limit", 1);
    int min_tps_to_cluster = j.value("min_tps_to_cluster", 1);
    int adc_integral_cut_ind = j.value("adc_integral_cut_induction", 0);
    int adc_integral_cut_col = j.value("adc_integral_cut_collection", 0);
    int tot_cut = j.value("tot_cut", 0);
    int max_files = j.value("max_files", -1); // -1 means no limit

    LogInfo << "Settings from json file:" << std::endl;
    LogInfo << " - Output folder: " << outfolder << std::endl;
    LogInfo << " - Tick limit: " << ticks_limit << std::endl;
    LogInfo << " - Channel limit: " << channel_limit << std::endl;
    LogInfo << " - Minimum TPs to form a cluster: " << min_tps_to_cluster << std::endl;
    LogInfo << " - ADC integral cut (induction): " << adc_integral_cut_ind << std::endl;
    LogInfo << " - ADC integral cut (collection): " << adc_integral_cut_col << std::endl;
    LogInfo << " - ToT cut: " << tot_cut << std::endl;
    if (max_files > 0) {
        LogInfo << " - Max files to process: " << max_files << std::endl;
    } else {
        LogInfo << " - Max files to process: unlimited" << std::endl;
        max_files = inputs.size();
    }

    std::string file_prefix;
    try {
        file_prefix = j.at("outputFilename").get<std::string>();
    } catch (...) {
        std::filesystem::path json_path(json);
        file_prefix = json_path.stem().string();
    }

    std::string clusters_filename = outfolder + "/" + file_prefix
        + "_tick" + std::to_string(ticks_limit)
        + "_ch" + std::to_string(channel_limit)
        + "_min" + std::to_string(min_tps_to_cluster)
        + "_tot" + std::to_string(tot_cut)
        + "_clusters.root";    

    // Ensure output directory exists
    std::filesystem::path outfolder_path(outfolder);
    if (!std::filesystem::exists(outfolder_path)) {
        LogInfo << "Output folder does not exist, creating: " << outfolder << std::endl;
        std::filesystem::create_directories(outfolder_path);
    }

    // delete clusters_filename file if already existing
    if (std::filesystem::exists(clusters_filename)) {
        LogInfo << "Output file " << clusters_filename << " already exists, deleting it." << std::endl;
        std::filesystem::remove(clusters_filename);
    } else {
        LogInfo << "Output file will be: " << clusters_filename << std::endl;
    }

    // Open the output file ONCE with RECREATE mode (more robust than UPDATE)
    TFile* clusters_file = new TFile(clusters_filename.c_str(), "RECREATE");
    if (!clusters_file || clusters_file->IsZombie()) {
        LogError << "Cannot create output file: " << clusters_filename << std::endl;
        return 1;
    } 

    LogInfo << "Opened output file for writing: " << clusters_filename << std::endl;

    int file_count = 0;

    for (const auto& tps_file : inputs) {

        // Check if we've reached max_files limit
        if (file_count >= max_files) {
            LogInfo << "Reached max_files limit (" << max_files << "), stopping." << std::endl;
            break;
        }
        
        if (verboseMode) LogInfo << "Input TPs file: " << tps_file << std::endl;

        file_count++;
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
                                                ticks_limit, 
                                                channel_limit, 
                                                min_tps_to_cluster, 
                                                adc_cut.at(iView)));
            
            // Identify the main cluster (most energetic) in each view for this event
            for (size_t iView=0; iView<APA::views.size(); ++iView) {
                auto& clusters = clusters_per_view.at(iView);
                if (clusters.empty()) continue;
                
                // Find cluster with highest true particle energy
                Cluster* main_cluster = nullptr;
                float max_energy = -1.0f;
                for (auto& cluster : clusters) {
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

    // Close the file ONCE at the end
    clusters_file->Close();
    delete clusters_file;
    
    LogInfo << "\nOutput file is: " << clusters_filename << std::endl;
    return 0;
}
