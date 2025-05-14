#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <map>
#include <sstream>
#include <climits>
#include <algorithm>
#include <set>
#include <nlohmann/json.hpp>

#include "CmdLineParser.h"
#include "Logger.h"
#include "GenericToolbox.Utils.h"

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

    LogInfo << "Loaded TPs and true particles from file. Number of TPs: " << tps.size() << std::endl;

    // print generator of all true particles
    // for (int i = 0; i < true_particles.size(); i++) {
    //     std::cout << "True particle " << i << ": " << true_particles.at(i).GetGeneratorName();
    //     true_particles.at(i).Print();
    // }

    // return 0;

    // connect TPs to the true particles
    LogInfo << "Connecting TPs to true particles" << std::endl;

    // count numnber of events, meaning unique values of event
    std::set<int> events;
    for (int iTP = 0; iTP < tps.size(); iTP++) {
        events.insert(tps.at(iTP).GetEvent());
    }
    LogInfo << "Number of events: " << events.size() << std::endl;
    
    // create a vector of array of pointers for the true particles and the TPs
    // one per event. Event numbers start from 1, so we need events.size()+1. TODO think of better ways
    std::vector<std::vector<TriggerPrimitive*>> tps_per_event(events.size()+1);
    std::vector<std::vector<TrueParticle*>> true_particles_per_event(events.size()+1);
    
    for (int iTP = 0; iTP < tps.size(); iTP++) {
        // get the event number
        int event = tps.at(iTP).GetEvent();
        // add the TP to the vector of TPs for this event
        tps_per_event.at(event).push_back(&tps.at(iTP));
    }

    for (int iTruePart = 0; iTruePart < true_particles.size(); iTruePart++) {
        // get the event number
        int event = true_particles.at(iTruePart).GetEvent();
        // add the TP to the vector of TPs for this event
        true_particles_per_event.at(event).push_back(&true_particles.at(iTruePart));
    }

    // mind that events actually start from 1
    for (int iEvent = 1; iEvent < events.size(); iEvent++) {
        
        LogInfo << "Event " << iEvent << ": " << tps_per_event.at(iEvent).size() << " TPs" << std::endl;
        LogInfo << "Event " << iEvent << ": " << true_particles_per_event.at(iEvent).size() << " true particles" << std::endl;

        for (int iTP = 0; iTP < tps_per_event.at(iEvent).size(); iTP++) {
            
            // progress bar
            if (iTP % 1000 == 0)
                GenericToolbox::displayProgressBar(iTP, tps_per_event.at(iEvent).size(), "Matching TPs and true particles for this event...");

            for (int iTruePart = 0; iTruePart < true_particles_per_event.at(iEvent).size(); iTruePart++) {
                if (tps_per_event.at(iEvent).at(iTP)->GetEvent() == true_particles_per_event.at(iEvent).at(iTruePart)->GetEvent()) {
                    
                    if (isTimeCompatible(true_particles_per_event.at(iEvent).at(iTruePart), tps_per_event.at(iEvent).at(iTP), time_window) 
                        && isChannelCompatible(true_particles_per_event.at(iEvent).at(iTruePart), tps_per_event.at(iEvent).at(iTP))) 
                    {   
                        // LogInfo << "TP " << tps_per_event.at(iEvent).at(iTP)->GetEvent() << " connected to true particle " << true_particles_per_event.at(iEvent).at(iTruePart)->GetEvent() << ", generator " << true_particles_per_event.at(iEvent).at(iTruePart)->GetGeneratorName() << std::endl;
                        tps_per_event.at(iEvent).at(iTP)->SetTrueParticle(true_particles_per_event.at(iEvent).at(iTruePart));
                        break;
                    }
                }
            }
        }
    }
    
    LogInfo << "The views are " << APA::views.size() << std::endl;

    std::vector <std::vector<TriggerPrimitive*>> tps_per_view;
    tps_per_view.reserve(APA::views.size());
    

    std::vector <std::vector<cluster>> clusters_per_view;
    clusters_per_view.reserve(APA::views.size());

    for (uint iView = 0; iView < APA::views.size(); iView++) {
        // if (i < 2) continue; // testing only collection
        // divide the tps in views
        std::vector<TriggerPrimitive*> these_tps_per_view;
        getPrimitivesForView(APA::views.at(iView), tps, these_tps_per_view);
        LogInfo << "Number of TPs in " << APA::views.at(iView) << " view: " << these_tps_per_view.size() << std::endl;
        tps_per_view.emplace_back(these_tps_per_view);
        // cluster the tps
        clusters_per_view.emplace_back(cluster_maker(tps_per_view.at(iView), ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut));
        LogInfo << "Number of clusters in " << APA::views.at(iView) << " view: " << clusters_per_view.at(iView).size() << std::endl;
    }
        
    // filter the clusters
    // if (main_track_option == 1) {
    //     clusters_x = filter_main_tracks(clusters_x);
    // } else if (main_track_option == 2) {
    //     clusters_x = filter_out_main_track(clusters_x);
    // }else if (main_track_option == 3) {
    //     assing_different_label_to_main_tracks(clusters_x);
    // }

    // write clusters to root files, 
    // create the root file
    
    for (int iView = 0; iView < APA::views.size(); iView++) {
        std::string clusters_filename = outfolder + "/clusters_tick" + std::to_string(ticks_limit) + "_ch" + std::to_string(channel_limit) + "_min" + std::to_string(min_tps_to_cluster) + "_"+ APA::views.at(iView) + ".root";
        LogInfo << "Writing " << APA::views.at(iView) << " clusters to " << clusters_filename << std::endl;
        write_clusters_to_root(clusters_per_view.at(iView), clusters_filename);
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

