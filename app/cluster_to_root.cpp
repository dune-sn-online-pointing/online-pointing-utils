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
    std::string filename = j["filename"];
    std::cout << "Filename: " << filename << std::endl;
    std::string outfolder = j["output_folder"];
    std::cout << "Output folder: " << outfolder << std::endl;
    int ticks_limit = j["tick_limit"];
    std::cout << "Tick limit: " << ticks_limit << std::endl;
    int channel_limit = j["channel_limit"];
    std::cout << "Channel limit: " << channel_limit << std::endl;
    int min_tps_to_cluster = j["min_tps_to_cluster"];
    std::cout << "Min TPs to cluster: " << min_tps_to_cluster << std::endl;
    int plane = j["plane"];
    std::cout << "Plane: " << plane << std::endl;
    int supernova_option = j["supernova_option"];
    std::cout << "Supernova option: " << supernova_option << std::endl;
    int main_track_option = j["main_track_option"];
    std::cout << "Main track option: " << main_track_option << std::endl;
    int max_events_per_filename = j["max_events_per_filename"];
    std::cout << "Max events per filename: " << max_events_per_filename << std::endl;
    int adc_integral_cut = j["adc_integral_cut"];
    std::cout << "ADC integral cut: " << adc_integral_cut << std::endl;
    std::vector<std::string> plane_names = {"U", "V", "X"};
    // start the clock
    std::clock_t start;

    if (plane !=3){        
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
        std::string root_filename = outfolder + "/" + plane_names[plane] + "/clusters_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_cluster_" + std::to_string(min_tps_to_cluster) + std::to_string(adc_integral_cut) +  ".root";
        write_clusters_to_root(clusters, root_filename);
        std::cout << "clusters written to " << root_filename << std::endl;
    }
    else { // do all planes
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
        // TODO: parallelize this
        std::vector<std::vector<double>> tps_u = file_reader(filenames, 0, supernova_option, max_events_per_filename);
        std::vector<std::vector<double>> tps_v = file_reader(filenames, 1, supernova_option, max_events_per_filename);
        std::vector<std::vector<double>> tps_x = file_reader(filenames, 2, supernova_option, max_events_per_filename);
        std::cout << "Number of tps: " << tps_u.size() << " " << tps_v.size() << " " << tps_x.size() << std::endl;
        std::map<int, std::vector<float>> file_idx_to_true_xyz_map = file_idx_to_true_xyz(filenames);
        std::map<int, int> file_idx_to_true_interaction_map = file_idx_to_true_interaction(filenames);
        std::cout << "XYZ map created" << std::endl;
        
        std::vector<cluster> clusters_u = cluster_maker(tps_u, ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut/2);
        std::vector<cluster> clusters_v = cluster_maker(tps_v, ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut/2);
        std::vector<cluster> clusters_x = cluster_maker(tps_x, ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut);

        std::cout << "Number of clusters: " << clusters_u.size() << " " << clusters_v.size() << " " << clusters_x.size() << std::endl;
        // add true x y z dir
        for (int i = 0; i < clusters_u.size(); i++) {
            clusters_u[i].set_true_dir(file_idx_to_true_xyz_map[clusters_u[i].get_tp(0)[clusters_u[i].get_tp(0).size() - 1]]);
            clusters_u[i].set_true_interaction(file_idx_to_true_interaction_map[clusters_u[i].get_tp(0)[clusters_u[i].get_tp(0).size() - 1]]);
        }
        for (int i = 0; i < clusters_v.size(); i++) {
            clusters_v[i].set_true_dir(file_idx_to_true_xyz_map[clusters_v[i].get_tp(0)[clusters_v[i].get_tp(0).size() - 1]]);
            clusters_v[i].set_true_interaction(file_idx_to_true_interaction_map[clusters_v[i].get_tp(0)[clusters_v[i].get_tp(0).size() - 1]]);
        }
        for (int i = 0; i < clusters_x.size(); i++) {
            clusters_x[i].set_true_dir(file_idx_to_true_xyz_map[clusters_x[i].get_tp(0)[clusters_x[i].get_tp(0).size() - 1]]);
            clusters_x[i].set_true_interaction(file_idx_to_true_interaction_map[clusters_x[i].get_tp(0)[clusters_x[i].get_tp(0).size() - 1]]);
        }
        // filter the clusters
        if (main_track_option == 1) {
            clusters_x = filter_main_tracks(clusters_x);
        } else if (main_track_option == 2) {
            clusters_x = filter_out_main_track(clusters_x);
        }else if (main_track_option == 3) {
            assing_different_label_to_main_tracks(clusters_x);
        }

        // write the clusters to a root file
        std::string root_filename_u = outfolder + "/U/clusters_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_cluster_" + std::to_string(min_tps_to_cluster) + ".root";
        std::string root_filename_v = outfolder + "/V/clusters_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_cluster_" + std::to_string(min_tps_to_cluster) + ".root";
        std::string root_filename_x = outfolder + "/X/clusters_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_cluster_" + std::to_string(min_tps_to_cluster) + ".root";
        write_clusters_to_root(clusters_u, root_filename_u);
        write_clusters_to_root(clusters_v, root_filename_v);
        write_clusters_to_root(clusters_x, root_filename_x);
        std::cout << "clusters written to " << root_filename_u << std::endl;
        std::cout << "clusters written to " << root_filename_v << std::endl;
        std::cout << "clusters written to " << root_filename_x << std::endl;
        }
    // stop the clock
    std::clock_t end;
    end = std::clock();
    return 0;
}

