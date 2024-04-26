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

    std::vector<std::string> plane_names = {"U", "V", "X"};

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
        std::vector<std::vector<double>> tps_u = file_reader(filenames, 0, supernova_option, max_events_per_filename);
        std::vector<std::vector<double>> tps_v = file_reader(filenames, 1, supernova_option, max_events_per_filename);
        std::vector<std::vector<double>> tps_x = file_reader(filenames, 2, supernova_option, max_events_per_filename);
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

    int n = 0;
    for (int i = 0; i < clusters_x.size(); i++) {
        for (int j = 0; j < clusters_u.size(); j++) {
            for (int k = 0; k < clusters_v.size(); k++) {
    // for (int i = 0; i < 3; i++) {
    //     for (int j = 0; j < 3; j++) {
    //         for (int k = 0; k < 3; k++) {
                // if not in the same apa skip
                if (int(clusters_u[j].get_tps()[0][3]/2560) != int(clusters_x[i].get_tps()[0][3]/2560)) {
                    continue;
                }
                if (int(clusters_v[k].get_tps()[0][3]/2560) != int(clusters_x[i].get_tps()[0][3]/2560)) {
                    continue;
                }
            //     std::cout << "Event: " << clusters_x[i].get_tps()[0][variables_to_index["event"]] << " Cluster U: " << clusters_u[j].get_true_label() << " Cluster V: " << clusters_v[k].get_true_label() << " Cluster X: " << clusters_x[i].get_true_label() << std::endl;
            // // print(f"Cluster U apa: {c_u.get_tps()[0][3]//2560}, Cluster V apa: {c_v.get_tps()[0][3]//2560}, Cluster X apa: {c_x.get_tps()[0][3]//2560}")
            //     std::cout << "Cluster U apa: " << int(clusters_u[j].get_tps()[0][3]/2560) << " Cluster V apa: " << int(clusters_v[k].get_tps()[0][3]/2560) << " Cluster X apa: " << int(clusters_x[i].get_tps()[0][3]/2560) << std::endl;
            //     int reco_y_u = eval_y_knowing_z_U_plane(clusters_u[j].get_tps(), clusters_x[i].get_reco_pos()[2], clusters_x[i].get_reco_pos()[0] > 0 ? 1 : -1)[0];
            //     std::cout << "Cluster U: X: " << clusters_u[j].get_reco_pos()[0] << " Y: " << reco_y_u << std::endl;
            //     int reco_y_v = eval_y_knowing_z_V_plane(clusters_v[k].get_tps(), clusters_x[i].get_reco_pos()[2], clusters_x[i].get_reco_pos()[0] > 0 ? 1 : -1)[0];
            //     std::cout << "Cluster V: X: " << clusters_v[k].get_reco_pos()[0] << " Y: " << reco_y_v << std::endl;
            //     std::cout << "Cluster X: X: " << clusters_x[i].get_reco_pos()[0] << " Z: " << clusters_x[i].get_reco_pos()[2] << std::endl;
            //     std::cout << std::endl;
                if (are_compatibles(clusters_u[j], clusters_v[k], clusters_x[i], 5)) {
                    std::cout << "Compatible clusters found" << std::endl;
                    std::cout << "Event: " << clusters_x[i].get_tps()[0][variables_to_index["event"]] << " Cluster U: " << clusters_u[j].get_true_label() << " Cluster V: " << clusters_v[k].get_true_label() << " Cluster X: " << clusters_x[i].get_true_label() << std::endl;
                    std::cout << "Cluster U: X: " << clusters_u[j].get_reco_pos()[0] << " Y: " << eval_y_knowing_z_U_plane(clusters_u[j].get_tps(), clusters_x[i].get_reco_pos()[2], clusters_x[i].get_reco_pos()[0] > 0 ? 1 : -1)[0] << std::endl;
                    std::cout << "Cluster V: X: " << clusters_v[k].get_reco_pos()[0] << " Y: " << eval_y_knowing_z_V_plane(clusters_v[k].get_tps(), clusters_x[i].get_reco_pos()[2], clusters_x[i].get_reco_pos()[0] > 0 ? 1 : -1)[0] << std::endl;
                    std::cout << "Cluster X: X: " << clusters_x[i].get_reco_pos()[0] << " Z: " << clusters_x[i].get_reco_pos()[2] << std::endl;
                    std::cout << j << " " << k << " " << i << std::endl;
                    std::cout << std::endl; 
                    n++;
                }
            }
        }
    }

    std::cout << "Number of compatible clusters: " << n << std::endl;







    // stop the clock
    std::clock_t end;
    end = std::clock();
    std::cout << "Time: " << (end - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    return 0;
}
