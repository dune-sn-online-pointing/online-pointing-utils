#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <nlohmann/json.hpp>

#include "CmdLineParser.h"
#include "Logger.h"

#include "aggregate_clusters_within_volume_libs.h"
// #include "position_calculator.h"
#include "cluster_to_root_libs.h"
#include "Cluster.h"
#include "create_volume_clusters_libs.h"


LoggerInit([]{
  Logger::getUserHeader() << "[" << FILENAME << "]";
});

int main(int argc, char* argv[]) {
    CmdLineParser clp;

    clp.getDescription() << "> superimpose_signal_and_backgrounds app."<< std::endl;

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

    std::string bkg_filenames = j["bkg_filenames"];
    std::string signal_clusters = j["signal_clusters"];
    std::string output_dir = j["output_folder"];
    int radius = j["radius"];
    int plane = j["plane"];
    int max_events_per_filename = j["max_events_per_filename"];

    std::cout << "bkg_filenames: " << bkg_filenames << std::endl;
    std::cout << "signal_clusters: " << signal_clusters << std::endl;
    std::cout << "output_dir: " << output_dir << std::endl;
    std::cout << "radius: " << radius << std::endl;
    std::cout << "plane: " << plane << std::endl;
    std::cout << "max_events_per_filename: " << max_events_per_filename << std::endl;

    std::vector<std::string> filenames;
    std::ifstream infile(bkg_filenames);
    std::string line;
    std::cout<<"Opening file: "<< bkg_filenames << std::endl;
    while (std::getline(infile, line)) {
        filenames.push_back(line);
    }
    std::cout << "Number of files: " << filenames.size() << std::endl;

    std::vector<Cluster> sig_clusters = read_clusters_from_root(signal_clusters);
    std::map<int, std::vector<Cluster>> sig_event_mapping = create_event_mapping(sig_clusters);
    std::cout << "Sig event mapping created" << std::endl;
    std::vector<int> sig_list_of_event_numbers;
    for (auto const& x : sig_event_mapping) {
        sig_list_of_event_numbers.push_back(x.first);
    }


    std::vector<std::vector<double>> bkg_tps = read_tpstream(filenames, plane, 2, max_events_per_filename);
    std::map<int, std::vector<std::vector<double>>> bkg_event_mapping = create_background_event_mapping(bkg_tps);
    std::cout << "Bkg event mapping created" << std::endl;
    std::vector<int> bkg_list_of_event_numbers;
    for (auto const& x : bkg_event_mapping) {
        bkg_list_of_event_numbers.push_back(x.first);
    }


    std::vector<Cluster> clusters_in_volume;
    for (int i = 0; i < sig_list_of_event_numbers.size(); i++) {
        if (i % 100 == 0) {
            std::cout << "Cluster number: " << i << std::endl;
        }
        std::vector<std::vector<double>> tps;
        // get the tps in the clusters of the event
        int main_track_idx = -1;
        for (int j= 0; j < sig_event_mapping[sig_list_of_event_numbers[i]].size(); j++) {
            std::vector<std::vector<double>> tps_cluster = sig_event_mapping[sig_list_of_event_numbers[i]][j].get_tps();
            tps.insert(tps.end(), tps_cluster.begin(), tps_cluster.end());
            if (sig_event_mapping[sig_list_of_event_numbers[i]][j].get_true_label() >= 100) {
                main_track_idx = j;
            }
        }
        if (main_track_idx == -1) {
            std::cout << "No main track found in the event" << std::endl;
            continue;
        }
        // extract a random background event
        int random_bkg_event = rand() % bkg_event_mapping.size();
        int random_bkg_event_index = bkg_list_of_event_numbers[random_bkg_event];
        // std::cout << "Random bkg event: " << random_bkg_event_index << std::endl;
        std::vector<std::vector<double>> tps_bkg = bkg_event_mapping[random_bkg_event_index];
        int this_offset = int(sig_event_mapping[sig_list_of_event_numbers[i]][main_track_idx].get_tps()[0][0]/EVENTS_OFFSET);
        for (int j = 0; j < tps_bkg.size(); j++) {
            tps_bkg[j][0] = int(tps_bkg[j][0])%EVENTS_OFFSET;
            tps_bkg[j][2] = int(tps_bkg[j][2])%EVENTS_OFFSET;
            tps_bkg[j][0] += this_offset*EVENTS_OFFSET;
            tps_bkg[j][2] += this_offset*EVENTS_OFFSET;
        }
        tps.insert(tps.end(), tps_bkg.begin(), tps_bkg.end());
        // std::cout << tps_bkg.size() << std::endl;
        // std::cout << tps.size() << std::endl;
        std::sort(tps.begin(), tps.end(), [](const std::vector<double>& a, const std::vector<double>& b) {
            return a[0] < b[0];
        });

        // std::cout << sig_event_mapping[sig_list_of_event_numbers[i]].size() << std::endl;
        // std::cout << sig_event_mapping[sig_list_of_event_numbers[i]][main_track_idx].get_tps().size() << std::endl;
        // std::cout << main_track_idx << std::endl;

        std::vector<std::vector<double>> tps_around_cluster = get_tps_around_cluster(tps, sig_event_mapping[sig_list_of_event_numbers[i]][main_track_idx], radius);
        // std::cout<<tps_around_cluster.size()<<std::endl;
        Cluster c(tps_around_cluster);
        c.set_true_pos(sig_event_mapping[sig_list_of_event_numbers[i]][main_track_idx].get_true_pos());
        c.set_true_dir(sig_event_mapping[sig_list_of_event_numbers[i]][main_track_idx].get_true_dir());
        c.set_true_energy(sig_event_mapping[sig_list_of_event_numbers[i]][main_track_idx].get_true_energy());
        c.set_true_label(sig_event_mapping[sig_list_of_event_numbers[i]][main_track_idx].get_true_label());
        c.set_true_interaction(sig_event_mapping[sig_list_of_event_numbers[i]][main_track_idx].get_true_interaction());
        c.set_min_distance_from_true_pos(sig_event_mapping[sig_list_of_event_numbers[i]][main_track_idx].get_min_distance_from_true_pos());
        c.set_supernova_tp_fraction(sig_event_mapping[sig_list_of_event_numbers[i]][main_track_idx].get_supernova_tp_fraction());
        c.set_reco_pos(sig_event_mapping[sig_list_of_event_numbers[i]][main_track_idx].get_reco_pos());
        clusters_in_volume.push_back(c);

    }
    std::cout << "Number of clusters in volume: " << clusters_in_volume.size() << std::endl;
    write_clusters(clusters_in_volume, output_dir+"clusters_in_volume.root");
    return 0;
}