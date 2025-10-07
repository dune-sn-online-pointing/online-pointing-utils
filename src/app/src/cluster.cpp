#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>

#include "CmdLineParser.h"
#include "Logger.h"

#include "cluster_to_root_libs.h"
#include "Cluster.h"
#include "functions.h"

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

int main(int argc, char* argv[]) {
    CmdLineParser clp;
    clp.getDescription() << "> Cluster app - build clusters from *_tps.root files."<< std::endl;
    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");
    clp.addOption("outFolder", {"--output-folder"}, "Output folder path (default: data)");
    clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");
    clp.addDummyOption();
    LogInfo << clp.getDescription().str() << std::endl;
    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;
    clp.parseCmdLine(argc, argv);
    LogThrowIf( clp.isNoOptionTriggered(), "No option was provided." );

    std::string json = clp.getOptionVal<std::string>("json");
    std::ifstream i(json); nlohmann::json j; i >> j;

    std::vector<std::string> inputs;
    if (j.contains("filename") && !j["filename"].get<std::string>().empty()) inputs.push_back(j["filename"].get<std::string>());
    if (j.contains("filelist") && !j["filelist"].get<std::string>().empty()) {
        std::ifstream lf(j["filelist"].get<std::string>()); std::string line; while (std::getline(lf, line)) { if (line.empty() || line[0]=='#') continue; inputs.push_back(line); }
    }
    LogThrowIf(inputs.empty(), "Provide 'filename' or 'filelist' in JSON.");

    std::string outfolder = clp.isOptionTriggered("outFolder") ? clp.getOptionVal<std::string>("outFolder") : std::string("data");

    int ticks_limit = j.value("tick_limit", 3);
    int channel_limit = j.value("channel_limit", 1);
    int min_tps_to_cluster = j.value("min_tps_to_cluster", 1);
    int adc_integral_cut_ind = j.value("adc_integral_cut_induction", 0);
    int adc_integral_cut_col = j.value("adc_integral_cut_collection", 0);
    int tot_cut = j.value("tot_cut", 0);

    std::vector<std::string> produced;

    for (const auto& tps_file : inputs) {
        LogInfo << "Input TPs file: " << tps_file << std::endl;
        std::map<int, std::vector<TriggerPrimitive>> tps_by_event;
        std::map<int, std::vector<TrueParticle>> true_by_event;
        std::map<int, std::vector<Neutrino>> nu_by_event;
        read_tps_from_root(tps_file, tps_by_event, true_by_event, nu_by_event);

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
            for (size_t iView=0;iView<APA::views.size();++iView){ std::vector<TriggerPrimitive*> v; getPrimitivesForView(APA::views.at(iView), tps, v); tps_per_view.emplace_back(std::move(v)); }

            std::vector<std::vector<Cluster>> clusters_per_view; clusters_per_view.reserve(APA::views.size());
            std::vector<int> adc_cut = {adc_integral_cut_ind, adc_integral_cut_ind, adc_integral_cut_col};
            for (size_t iView=0;iView<APA::views.size();++iView){ clusters_per_view.emplace_back(cluster_maker(tps_per_view.at(iView), ticks_limit, channel_limit, min_tps_to_cluster, adc_cut.at(iView))); }

            for (size_t iView=0;iView<APA::views.size();++iView){ write_clusters_to_root(clusters_per_view.at(iView), clusters_filename, APA::views.at(iView)); }
        }

        produced.push_back(clusters_filename);
    }

    LogInfo << "\nSummary of produced files (" << produced.size() << "):" << std::endl;
    for (const auto& p : produced) LogInfo << " - " << p << std::endl;
    return 0;
}
