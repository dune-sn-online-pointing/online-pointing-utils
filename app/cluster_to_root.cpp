#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <nlohmann/json.hpp>

#include "CmdLineParser.h"
#include "Logger.h"

#include "position_calculator.h"
#include "cluster_to_root_libs.h"
#include "cluster.h"


LoggerInit([]{
  Logger::getUserHeader() << "[" << FILENAME << "]";
});

int main(int argc, char* argv[]) {
    CmdLineParser clp;

    clp.getDescription() << "> cluster_to_root app."<< std::endl;

    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");

    clp.addDummyOption("Triggers");
    clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");

    clp.addDummyOption();
    // usage always displayed
    LogInfo << clp.getDescription().str() << std::endl;

    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;

    clp.parseCmdLine(argc, argv);

    LogThrowIf( clp.isNoOptionTriggered(), "No option was provided." );

    LogInfo << "Provided arguments: " << std::endl;
    LogInfo << clp.getValueSummary() << std::endl << std::endl;

    std::string json = clp.getOptionVal<std::string>("json");
    // read the configuration file
    std::ifstream i(json);
    nlohmann::json j;
    i >> j;
    std::string filename = j["input_file"];
    std::string outfolder = j["output_folder"];
    int ticks_limit = j["tick_limit"];
    int channel_limit = j["channel_limit"];
    int min_tps_to_cluster = j["min_tps_to_cluster"];
    int plane = j["plane"];
    int supernova_option = j["supernova_option"];
    int main_track_option = j["main_track_option"];
    int max_events_per_filename = j["max_events_per_filename"];
    int adc_integral_cut = j["adc_integral_cut"];
    std::vector<std::string> plane_names = {"U", "V", "X"};
    // start the clock
    std::clock_t start;

    // filename is the name of the file containing the filenames to read
    std::vector<std::string> filenames;
    // read the file containing the filenames and save them in a vector
    std::ifstream infile(filename);
    std::string line;
    std::cout<<"Opening file: "<< filename << std::endl;
    // read and save the TPs
    while (std::getline(infile, line)) {
        filenames.push_back(line);
    }
    std::cout << "Number of files: " << filenames.size() << std::endl;
    std::vector<std::vector<double>> tps = file_reader(filenames, plane, supernova_option, max_events_per_filename);
    std::cout << "Number of tps: " << tps.size() << std::endl;
    std::map<int, std::vector<float>> file_idx_to_true_xyz_map = file_idx_to_true_xyz(filenames);
    std::map<int, int> file_idx_to_true_interaction_map = file_idx_to_true_interaction(filenames);
    std::cout << "XYZ map created" << std::endl;
    // cluster the tps
    std::vector<cluster> clusters = cluster_maker(tps, ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut);
    std::cout << "Number of clusters: " << clusters.size() << std::endl;
    // add true x y z dir 
    for (int i = 0; i < clusters.size(); i++) {
        clusters[i].set_true_dir(file_idx_to_true_xyz_map[clusters[i].get_tp(0)[clusters[i].get_tp(0).size() - 1]]);
        clusters[i].set_true_interaction(file_idx_to_true_interaction_map[clusters[i].get_tp(0)[clusters[i].get_tp(0).size() - 1]]);
    }
    // filter the clusters
    if (main_track_option == 1) {
        clusters = filter_main_tracks(clusters);
    } else if (main_track_option == 2) {
        clusters = filter_out_main_track(clusters);
    }else if (main_track_option == 3) {
        assing_different_label_to_main_tracks(clusters);
    }
    std::cout << "Number of clusters after filtering: " << clusters.size() << std::endl;

    std::map<int, int> label_to_count;
    for (int i = 0; i < clusters.size(); i++) {
        if (label_to_count.find(clusters[i].get_true_label()) == label_to_count.end()) {
            label_to_count[clusters[i].get_true_label()] = 0;
        }
        label_to_count[clusters[i].get_true_label()]++;
    }
    for (auto const& x : label_to_count) {
        // std::cout << "Label " << x.first << " has " << x.second << " clusters" << std::endl;
        std::cout << x.first << " ";
    }
    std::cout << std::endl;
    // if no clusters are found, return 0
    for (auto const& x : label_to_count) {
        // std::cout << "Label " << x.first << " has " << x.second << " clusters" << std::endl;
        std::cout << x.second << " ";
    }
    std::cout << std::endl;

    // write the clusters to a root file
    std::string root_filename = outfolder + "/" + plane_names[plane] + "/clusters_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_cluster_" + std::to_string(min_tps_to_cluster) + ".root";
    write_clusters_to_root(clusters, root_filename);
    std::cout << "clusters written to " << root_filename << std::endl;

    // stop the clock
    std::clock_t end;
    end = std::clock();
    return 0;
}

