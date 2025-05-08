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
#include "functions.h"


LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

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
    LogInfo << "Filename: " << filename << std::endl;
    std::string outfolder = j["output_folder"];
    LogInfo << "Output folder: " << outfolder << std::endl;
    int ticks_limit = j["tick_limit"];
    LogInfo << "Tick limit: " << ticks_limit << std::endl;
    int channel_limit = j["channel_limit"];
    LogInfo << "Channel limit: " << channel_limit << std::endl;
    int min_tps_to_cluster = j["min_tps_to_cluster"];
    LogInfo << "Min TPs to cluster: " << min_tps_to_cluster << std::endl;
    int plane = j["plane"];
    LogInfo << "Plane: " << plane << std::endl;
    int supernova_option = j["supernova_option"];
    LogInfo << "Supernova option: " << supernova_option << std::endl;
    int main_track_option = j["main_track_option"];
    LogInfo << "Main track option: " << main_track_option << std::endl;
    int max_events_per_filename = j["max_events_per_filename"];
    LogInfo << "Max events per filename: " << max_events_per_filename << std::endl;
    int adc_integral_cut = j["adc_integral_cut"];
    LogInfo << "ADC integral cut: " << adc_integral_cut << std::endl;
    int use_electron_direction = j["use_electron_direction"];
    LogInfo << "Use electron direction: " << use_electron_direction << std::endl;

    // start the clock
    std::clock_t start = std::clock();

    // if (plane !=3){ // this means all planes TODO change to a string probably       
    //     // filename is the name of the file containing the filenames to read
    //     std::vector<std::string> filenames;
    //     // read the file containing the filenames and save them in a vector
    //     std::ifstream infile(filename);
    //     std::string line;
    //     LogInfo<<"Opening file: "<< filename << std::endl;
    //     // read and save the TPs
    //     while (std::getline(infile, line)) {
    //         filenames.push_back(line);
    //     }
    //     LogInfo << "Number of files: " << filenames.size() << std::endl;
    //     std::vector<TriggerPrimitive> tps = file_reader(filenames, plane, supernova_option, max_events_per_filename);
    //     LogInfo << "Number of tps: " << tps.size() << std::endl;
    //     std::map<int, std::vector<float>> file_idx_to_true_xyz_map;
    //     if (use_electron_direction == 0) {
    //         file_idx_to_true_xyz_map = file_idx_to_true_xyz(filenames);
    //     }
    //     std::map<int, int> file_idx_to_true_interaction_map = file_idx_to_true_interaction(filenames);
    //     LogInfo << "XYZ map created" << std::endl;
    //     // cluster the tps
    //     std::vector<cluster> clusters = cluster_maker(tps, ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut);
    //     LogInfo << "Number of clusters: " << clusters.size() << std::endl;
    //     // add true x y z dir 
        
    //     std::map<int, std::vector<float>> file_idx_to_true_pos;

    //     for (int i = 0; i < clusters.size(); i++) {
    //         if (use_electron_direction == 0) {
    //             clusters[i].set_true_dir(file_idx_to_true_xyz_map[clusters[i].get_tp(0)[clusters[i].get_tp(0).size() - 1]]);
    //         }
    //         clusters[i].set_true_interaction(file_idx_to_true_interaction_map[clusters[i].get_tp(0)[clusters[i].get_tp(0).size() - 1]]);
            
    //         if (clusters[i].get_true_label() == 1) {
    //             if (file_idx_to_true_pos.find(clusters[i].get_tp(0)[variables_to_index["event"]]) == file_idx_to_true_pos.end()) {
    //                 if (clusters[i].get_true_pos()[0] != 0 and clusters[i].get_true_pos()[1] != 0 and clusters[i].get_true_pos()[2] != 0) {
    //                     file_idx_to_true_pos[clusters[i].get_tp(0)[variables_to_index["event"]]]= clusters[i].get_true_pos();
    //                 }
    //             }
    //         }
    //     }


    //     // update the clusters
    //     int errors = 0;
    //     for (int i = 0; i < clusters.size(); i++) {
    //         if (clusters[i].get_true_label() == 1) {
    //             if (file_idx_to_true_pos.find(clusters[i].get_tp(0)[variables_to_index["event"]]) == file_idx_to_true_pos.end()) {
    //                 continue;
    //             }
    //             float old_min = clusters[i].get_min_distance_from_true_pos();
    //             std::vector<float> old_pos = clusters[i].get_true_pos();
    //             clusters[i].set_true_pos(file_idx_to_true_pos[clusters[i].get_tp(0)[variables_to_index["event"]]]);
    //             clusters[i].update_cluster_info();
    //             // LogInfo << clusters[i].get_true_pos()[0] << " " << clusters[i].get_true_pos()[1] << " " << clusters[i].get_true_pos()[2] << std::endl;
    //             float new_min = clusters[i].get_min_distance_from_true_pos();

    //             if (new_min > old_min) {
    //                 if (old_pos[0] != 0 and old_pos[1] != 0 and old_pos[2] != 0) {
    //                     errors++;
    //                 }
    //             }
    //         }
    //     }
    //     LogInfo << "Errors: " << errors << std::endl;

    //     // filter the clusters
    //     if (main_track_option == 1) {
    //         clusters = filter_main_tracks(clusters);
    //     } else if (main_track_option == 2) {
    //         clusters = filter_out_main_track(clusters);
    //     }else if (main_track_option == 3) {
    //         assing_different_label_to_main_tracks(clusters);
    //     }
    //     LogInfo << "Number of clusters after filtering: " << clusters.size() << std::endl;

    //     std::map<int, int> label_to_count;
    //     for (int i = 0; i < clusters.size(); i++) {
    //         if (label_to_count.find(clusters[i].get_true_label()) == label_to_count.end()) {
    //             label_to_count[clusters[i].get_true_label()] = 0;
    //         }
    //         label_to_count[clusters[i].get_true_label()]++;
    //     }
    //     for (auto const& x : label_to_count) {
    //         // LogInfo << "Label " << x.first << " has " << x.second << " clusters" << std::endl;
    //         LogInfo << x.first << " ";
    //     }
    //     LogInfo << std::endl;
    //     // if no clusters are found, return 0
    //     for (auto const& x : label_to_count) {
    //         // LogInfo << "Label " << x.first << " has " << x.second << " clusters" << std::endl;
    //         LogInfo << x.second << " ";
    //     }
    //     LogInfo << std::endl;

    //     // write the clusters to a root file
    //     std::string root_filename = outfolder + "/" + views[plane] + "/clusters_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_cluster_" + std::to_string(min_tps_to_cluster) + "_cut_" + std::to_string(adc_integral_cut) +  ".root";
    //     write_clusters_to_root(clusters, root_filename);
    //     LogInfo << "clusters written to " << root_filename << std::endl;
    // }
    // else { // do all planes
    // filename is the name of the file containing the filenames to read
    std::vector<std::string> filenames;
    // read the file containing the filenames and save them in a vector
    std::ifstream infile(filename);
    std::string line;
    LogInfo<<"Opening file: "<< filename << std::endl;
    // read and save the TPs
    while (std::getline(infile, line)) {
        filenames.push_back(line);
    }
    LogInfo << "Number of files: " << filenames.size() << std::endl;
    // TODO: parallelize this
    // std::vector<std::vector<TriggerPrimitive>> tps = file_reader_all_planes(filenames, supernova_option, max_events_per_filename);
    std::vector<TriggerPrimitive> tps;
    std::vector<TrueParticle> true_particles;
    std::vector<Neutrino> neutrinos;

    file_reader(filenames, tps, true_particles, neutrinos, supernova_option, max_events_per_filename);

    // connect TPs to the true particles
    int time_window = 32; // in ticks, TODO move from here
    for (int i = 0; i < tps.size(); i++) {
        for (int j = 0; j < true_particles.size(); j++) {
            if (tps.at(i).GetEvent() == true_particles.at(j).GetEvent()) {
                
                if (isTimeCompatible(true_particles.at(j), tps.at(i), time_window) 
                    && isChannelCompatible(true_particles.at(j), tps.at(i))) 
                {
                    tps.at(i).SetTrueParticle(&true_particles.at(j));
                }
            }
        }
    }
    
    LogInfo << "The views are " << APA::views.size() << std::endl;

    std::vector <std::vector<TriggerPrimitive*>> tps_per_view;
    tps_per_view.reserve(APA::views.size());
    

    std::vector <std::vector<cluster>> clusters_per_view;
    clusters_per_view.reserve(APA::views.size());

    for (uint i = 0; i < APA::views.size(); i++) {
        // divide the tps in views
        std::vector<TriggerPrimitive*> these_tps_per_view;
        getPrimitivesForView(APA::views.at(i), tps, these_tps_per_view);
        tps_per_view.emplace_back(these_tps_per_view);
        LogInfo << "Number of tps in " << views.at(i) << " view: " << tps_per_view.at(i).size() << std::endl;
        // cluster the tps
        clusters_per_view.emplace_back(cluster_maker(tps_per_view.at(i), ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut));
        LogInfo << "Number of clusters in " << views.at(i) << " view: " << clusters_per_view.at(i).size() << std::endl;
    }
        

    // std::map<int, int> file_idx_to_true_interaction_map = file_idx_to_true_interaction(filenames);
    // LogInfo << "XYZ map created" << std::endl;
    
    // LogInfo << "Creating clusters on view U" << std::endl;
    // std::vector<cluster> clusters_u = cluster_maker(tps_u, ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut/4);
    // LogInfo << "Creating clusters on view V" << std::endl;
    // std::vector<cluster> clusters_v = cluster_maker(tps_v, ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut/4);
    // LogInfo << "Creating clusters on view X" << std::endl;
    // std::vector<cluster> clusters_x = cluster_maker(tps_x, ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut);

    // LogInfo << "Number of clusters, view U: " << clusters_u.size() << ", V: " << clusters_v.size() << ",X: " << clusters_x.size() << std::endl;

    // add true x y z dir
    // for (int i = 0; i < clusters_u.size(); i++) {
    //     if (use_electron_direction == 0) {
    //         clusters_u[i].set_true_dir(file_idx_to_true_xyz_map[clusters_u[i].get_tp(0)[clusters_u[i].get_tp(0).size() - 1]]);
    //     }

    //     clusters_u[i].set_true_interaction(file_idx_to_true_interaction_map[clusters_u[i].get_tp(0)[clusters_u[i].get_tp(0).size() - 1]]);
    // }
    // for (int i = 0; i < clusters_v.size(); i++) {
    //     if (use_electron_direction == 0) {
    //         clusters_v[i].set_true_dir(file_idx_to_true_xyz_map[clusters_v[i].get_tp(0)[clusters_v[i].get_tp(0).size() - 1]]);
    //     }

    //     clusters_v[i].set_true_interaction(file_idx_to_true_interaction_map[clusters_v[i].get_tp(0)[clusters_v[i].get_tp(0).size() - 1]]);
    // }
    // for (int i = 0; i < clusters_x.size(); i++) {
    //     if (use_electron_direction == 0) {
    //         clusters_x[i].set_true_dir(file_idx_to_true_xyz_map[clusters_x[i].get_tp(0)[clusters_x[i].get_tp(0).size() - 1]]);
    //     }

    //     clusters_x[i].set_true_interaction(file_idx_to_true_interaction_map[clusters_x[i].get_tp(0)[clusters_x[i].get_tp(0).size() - 1]]);
    // }

    // filter the clusters
    // if (main_track_option == 1) {
    //     clusters_x = filter_main_tracks(clusters_x);
    // } else if (main_track_option == 2) {
    //     clusters_x = filter_out_main_track(clusters_x);
    // }else if (main_track_option == 3) {
    //     assing_different_label_to_main_tracks(clusters_x);
    // }

    // write the clusters to a root file
    // std::string root_filename_u = outfolder + "/U/clusters_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_cluster_" + std::to_string(min_tps_to_cluster) + ".root";
    // std::string root_filename_v = outfolder + "/V/clusters_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_cluster_" + std::to_string(min_tps_to_cluster) + ".root";
    // std::string root_filename_x = outfolder + "/X/clusters_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_cluster_" + std::to_string(min_tps_to_cluster) + ".root";
    // // write_clusters_to_root(clusters_u, root_filename_u);
    // // write_clusters_to_root(clusters_v, root_filename_v);
    // // write_clusters_to_root(clusters_x, root_filename_x);
    // LogInfo << "clusters written to " << root_filename_u << std::endl;
    // LogInfo << "clusters written to " << root_filename_v << std::endl;
    // LogInfo << "clusters written to " << root_filename_x << std::endl;
    // // }


    // write clusters to root files, 
    // create the root file
    
    for (int i = 0; i < APA::views.size(); i++) {
        std::string clusters_filename = outfolder + "/clusters_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_cluster_" + std::to_string(min_tps_to_cluster) + "_"+ APA::views.at(i) + ".root";
        LogInfo << "Writing " << APA::views.at(i) << " clusters to " << clusters_filename << std::endl;
        write_clusters_to_root(clusters_per_view.at(i), clusters_filename);
    }

    // stop the clock
    std::clock_t end = std::clock();

    // std::setprecision(3);
    LogInfo << "Time take  in seconds: " << float(end - start) / CLOCKS_PER_SEC << std::endl;
    LogInfo << "Time taken in minutes: " << float(end - start) / CLOCKS_PER_SEC / 60  << std::endl;
    LogInfo << "Time taken in hours: " << float(end - start) / CLOCKS_PER_SEC / 3600 << std::endl;


    // free the memory
    for (auto& view_tps : tps_per_view)
        view_tps.clear();
    tps_per_view.clear();


    return 0;
}

