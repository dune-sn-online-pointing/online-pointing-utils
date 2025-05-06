#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <nlohmann/json.hpp>

#include "CmdLineParser.h"
#include "Logger.h"

// #include "position_calculator.h"
#include "cluster_to_root_libs.h"
#include "cluster.h"
#include "match_clusters_libs.h"


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
    std::string file_clusters_u = j["clusters_u"];
    std::string file_clusters_v = j["clusters_v"];
    std::string file_clusters_x = j["clusters_x"];
    int create_clusters_from_file = j["create_clusters_from_file"];
    int ticks_limit = j["tick_limit"];
    int channel_limit = j["channel_limit"];
    int min_tps_to_cluster = j["min_tps_to_cluster"];
    int supernova_option = j["supernova_option"];
    int main_track_option = j["main_track_option"];
    int max_events_per_filename = j["max_events_per_filename"];
    int adc_integral_cut = j["adc_integral_cut"];

    // start the clock
    std::clock_t start;
    start = std::clock();
    std::vector<cluster> clusters_u;
    std::vector<cluster> clusters_v;
    std::vector<cluster> clusters_x;

    if (create_clusters_from_file){ 
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
        std::vector<TriggerPrimitive> tps_u = file_reader(filenames, 0, supernova_option, max_events_per_filename);
        std::vector<TriggerPrimitive> tps_v = file_reader(filenames, 1, supernova_option, max_events_per_filename);
        std::vector<TriggerPrimitive> tps_x = file_reader(filenames, 2, supernova_option, max_events_per_filename);
        std::cout << "Number of tps: " << tps_u.size() << " " << tps_v.size() << " " << tps_x.size() << std::endl;
        std::map<int, std::vector<float>> file_idx_to_true_xyz_map = file_idx_to_true_xyz(filenames);
        std::map<int, int> file_idx_to_true_interaction_map = file_idx_to_true_interaction(filenames);
        std::cout << "XYZ map created" << std::endl;
        
        clusters_u = cluster_maker(tps_u, ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut/2);
        clusters_v = cluster_maker(tps_v, ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut/2);
        clusters_x = cluster_maker(tps_x, ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut);

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
        write_clusters_to_root(clusters_u, file_clusters_u);
        write_clusters_to_root(clusters_v, file_clusters_v);
        write_clusters_to_root(clusters_x, file_clusters_x);
    }
    else{
        clusters_u = read_clusters_from_root(file_clusters_u);
        clusters_v = read_clusters_from_root(file_clusters_v);
        clusters_x = read_clusters_from_root(file_clusters_x);
    }

    std::cout << "Number of clusters after filtering: " << clusters_x.size() << std::endl;
    std::map<int, int> label_to_count;

    for (int i = 0; i < clusters_x.size(); i++) {
        if (label_to_count.find(clusters_x[i].get_true_label()) == label_to_count.end()) {
            label_to_count[clusters_x[i].get_true_label()] = 0;
        }
        label_to_count[clusters_x[i].get_true_label()]++;
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
    std::cout << "clusters written to " << file_clusters_u << " " << file_clusters_v << " " << file_clusters_x << std::endl;
    std::cout << "Number of clusters: " << clusters_u.size() << " " << clusters_v.size() << " " << clusters_x.size() << std::endl;
    std::vector<std::vector<cluster>> clusters;
    std::vector<cluster> multiplane_clusters;

    int start_j = 0;
    int start_k = 0;
    std::clock_t str;

    for (int i = 0; i < clusters_x.size(); i++) {
        if (i % 10000 == 0 && i != 0) {
            str = std::clock();
            std::cout << "Cluster " << i << " Time: " << (str - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
            std::cout << "Number of compatible clusters: " << multiplane_clusters.size() << std::endl;
        }

        int min_range_j = 0;
        int max_range_j = clusters_u.size();
        while (min_range_j < max_range_j) {
            start_j = (min_range_j + max_range_j) / 2;
            if (clusters_u[start_j].get_tps()[0][variables_to_index["time_start"]] < clusters_x[i].get_tps()[0][variables_to_index["time_start"]]) {
                min_range_j = start_j + 1;
            } else {
                max_range_j = start_j;
            }
        }
        start_j = std::max(start_j-10, 0);
        for (int j = start_j; j < clusters_u.size(); j++) {

            if (clusters_u[j].get_tps()[0][variables_to_index["time_start"]] > clusters_x[i].get_tps()[0][variables_to_index["time_start"]] + 5000) {
                // std::cout << "continue" << std::endl;
                break;
            }

            if (clusters_u[j].get_tps()[0][variables_to_index["time_start"]] < clusters_x[i].get_tps()[0][variables_to_index["time_start"]] - 5000) {
                // std::cout << "continue" << std::endl;
                continue;
            }

            // if not in the same apa skip
            if (int(clusters_u[j].get_tps()[0][3]/2560) != int(clusters_x[i].get_tps()[0][3]/2560)) {
                continue;
            }
            int min_range_k = 0;
            int max_range_k = clusters_v.size();
            while (min_range_k < max_range_k) {
                start_k = (min_range_k + max_range_k) / 2;
                if (clusters_v[start_k].get_tps()[0][variables_to_index["time_start"]] < clusters_x[i].get_tps()[0][variables_to_index["time_start"]]) {
                    min_range_k = start_k + 1;
                } else {
                    max_range_k = start_k;
                }
            }
            start_k = std::max(start_k-10, 0);
            for (int k = start_k; k < clusters_v.size(); k++) {
                if (clusters_v[k].get_tps()[0][variables_to_index["time_start"]] > clusters_u[j].get_tps()[0][variables_to_index["time_start"]] + 5000) {
                    // std::cout << "break" << std::endl;
                    break;
                }
                if (clusters_v[k].get_tps()[0][variables_to_index["time_start"]] < clusters_u[j].get_tps()[0][variables_to_index["time_start"]] - 5000) {
                    // std::cout << "break" << std::endl;
                    continue;
                }
                

                if (int(clusters_v[k].get_tps()[0][3]/2560) != int(clusters_x[i].get_tps()[0][3]/2560)) {
                    continue;
                }

                if (are_compatibles(clusters_u[j], clusters_v[k], clusters_x[i], 5)) {
                    clusters.push_back({clusters_u[j], clusters_v[k], clusters_x[i]});
                    cluster c = join_clusters(clusters_u[j], clusters_v[k], clusters_x[i]);
                    multiplane_clusters.push_back(c);
                    // TODO: include option to use true information
                    // if (match_with_true_pos(clusters_u[j], clusters_v[k], clusters_x[i], 5)) {
                    //     clusters.push_back({clusters_u[j], clusters_v[k], clusters_x[i]});
                    //     cluster c = join_clusters(clusters_u[j], clusters_v[k], clusters_x[i]);
                    //     multiplane_clusters.push_back(c);
                    // }
                }
            }
        }
    }


    std::cout << "Number of compatible clusters: " << clusters.size() << std::endl;
    // return counts of label
    std::map<int, int> label_to_count_compatible;
    for (int i = 0; i < clusters.size(); i++) {
        if (label_to_count_compatible.find(clusters[i][2].get_true_label()) == label_to_count_compatible.end()) {
            label_to_count_compatible[clusters[i][2].get_true_label()] = 0;
        }
        label_to_count_compatible[clusters[i][2].get_true_label()]++;
    }
    for (auto const& x : label_to_count_compatible) {
        // std::cout << "Label " << x.first << " has " << x.second << " clusters" << std::endl;
        std::cout << x.first << " ";
    }
    std::cout << std::endl;
    for (auto const& x : label_to_count_compatible) {
        // std::cout << "Label " << x.first << " has " << x.second << " clusters" << std::endl;
        std::cout << x.second << " ";
    }
    std::cout << std::endl;

    // save the multiplane clusters to a root file
    write_clusters_to_root(multiplane_clusters, outfolder + "multiplane_clusters.root");
    std::cout << "multiplane clusters written to " << outfolder + "multiplane_clusters.root" << std::endl;
       

    // stop the clock
    std::clock_t end;
    end = std::clock();
    std::cout << "Time: " << (end - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    return 0;
}
