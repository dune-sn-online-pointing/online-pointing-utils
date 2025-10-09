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
    std::vector<std::string> inputs = find_input_files(j, "_tps.root");
    
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

    std::string outfolder = clp.isOptionTriggered("outFolder") ? clp.getOptionVal<std::string>("outFolder") : std::string("data");

    int ticks_limit = j.value("tick_limit", 3);
    int channel_limit = j.value("channel_limit", 1);
    int min_tps_to_cluster = j.value("min_tps_to_cluster", 1);
    int adc_integral_cut_ind = j.value("adc_integral_cut_induction", 0);
    int adc_integral_cut_col = j.value("adc_integral_cut_collection", 0);
    int tot_cut = j.value("tot_cut", 0);

    LogInfo << "Settings from json file:" << std::endl;
    // LogInfo << " - Output folder: " << outfolder << std::endl;
    LogInfo << " - Tick limit: " << ticks_limit << std::endl;
    LogInfo << " - Channel limit: " << channel_limit << std::endl;
    LogInfo << " - Minimum TPs to form a cluster: " << min_tps_to_cluster << std::endl;
    LogInfo << " - ADC integral cut (induction): " << adc_integral_cut_ind << std::endl;
    LogInfo << " - ADC integral cut (collection): " << adc_integral_cut_col << std::endl;
    LogInfo << " - ToT cut: " << tot_cut << std::endl;

    std::vector<std::string> produced;
    int file_count = 0;

    for (const auto& tps_file : inputs) {

        if (file_count > 100) break; // TEMP
        
        if (verboseMode) LogInfo << "Input TPs file: " << tps_file << std::endl;

        file_count++;
        int progress = (file_count * 100) / inputs.size();
        GenericToolbox::displayProgressBar(file_count, inputs.size(), "Making clusters...");

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

    // build output file name
    std::string base = tps_file.substr(tps_file.find_last_of("/\\") + 1);
    // Remove trailing "_tps.root" or generic ".root" if present
    if (base.size() > 9 && base.substr(base.size()-9) == "_tps.root") base = base.substr(0, base.size()-9);
    else if (base.size() > 5 && base.substr(base.size()-5) == ".root") base = base.substr(0, base.size()-5);
    std::string clusters_filename = outfolder + "/" + base + "_clusters_tick" + std::to_string(ticks_limit) + "_ch" + std::to_string(channel_limit) + "_min" + std::to_string(min_tps_to_cluster) + "_tot" + std::to_string(tot_cut) + "_en0.root";
    // absolute path for output
    std::error_code _ec_abs;
    std::filesystem::path abs_p = std::filesystem::absolute(std::filesystem::path(clusters_filename), _ec_abs);
    if (!_ec_abs) clusters_filename = abs_p.string();

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
                clusters_per_view.emplace_back(make_cluster(tps_per_view.at(iView), ticks_limit, channel_limit, min_tps_to_cluster, adc_cut.at(iView)));

            for (size_t iView=0;iView<APA::views.size();++iView)
                write_clusters(clusters_per_view.at(iView), clusters_filename, APA::views.at(iView)); 
        }

        produced.push_back(clusters_filename);
    }

    LogInfo << "\nSummary of produced files (" << produced.size() << "):" << std::endl;
    for (const auto& p : produced) LogInfo << " - " << p << std::endl;
    return 0;
}
