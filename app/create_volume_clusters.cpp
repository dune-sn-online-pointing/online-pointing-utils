#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <nlohmann/json.hpp>

#include "CmdLineParser.h"
#include "Logger.h"

#include "aggregate_clusters_within_volume_libs.h"
#include "position_calculator.h"
#include "cluster_to_root_libs.h"
#include "cluster.h"
#include "create_volume_clusters_libs.h"


LoggerInit([]{
  Logger::getUserHeader() << "[" << FILENAME << "]";
});

int main(int argc, char* argv[]) {
    CmdLineParser clp;

    clp.getDescription() << "> create_volume_clusters app."<< std::endl;

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

    std::string tps_filename = j["tps_filename"];
    std::string cluster_filename = j["cluster_filename"];
    std::string predictions = j["predictions"];
    std::string output_dir = j["output_dir"];
    int radius = j["radius"];
    int plane = j["plane"];
    int supernova_option = j["supernova_option"];
    int max_events_per_filename = j["max_events_per_filename"];

    std::cout << "tps_filename: " << tps_filename << std::endl;
    std::cout << "cluster_filename: " << cluster_filename << std::endl;
    std::cout << "predictions: " << predictions << std::endl;
    std::cout << "output_dir: " << output_dir << std::endl;
    std::cout << "radius: " << radius << std::endl;
    std::cout << "plane: " << plane << std::endl;
    std::cout << "supernova_option: " << supernova_option << std::endl;
    std::cout << "max_events_per_filename: " << max_events_per_filename << std::endl;

    std::vector<std::string> filenames;
    std::ifstream infile(tps_filename);
    std::string line;
    std::cout<<"Opening file: "<< tps_filename << std::endl;
    while (std::getline(infile, line)) {
        filenames.push_back(line);
    }
    std::cout << "Number of files: " << filenames.size() << std::endl;

    std::vector<std::vector<double>> tps = file_reader(filenames, plane, supernova_option, max_events_per_filename);
    std::vector<cluster> clusters = read_clusters_from_root(cluster_filename);
    std::vector<float> predictions_vector = read_predictions(predictions);

    std::cout << "Number of clusters: " << clusters.size() << std::endl;
    std::cout << "Number of predictions: " << predictions_vector.size() << std::endl;
    std::cout << "Number of tps: " << tps.size() << std::endl;

    double radius_in_ticks = radius/0.08;

    std::vector<cluster> clusters_in_volume;
    for (int i = 0; i < clusters.size(); i++) {
        if (i % 100 == 0) {
            std::cout << "Cluster number: " << i << std::endl;
        }
        std::vector<std::vector<double>> tps_around_cluster = get_tps_around_cluster(tps, clusters[i], radius_in_ticks);
        cluster c(tps_around_cluster);
        c.set_true_pos(clusters[i].get_true_pos());
        c.set_true_dir(clusters[i].get_true_dir());
        c.set_true_energy(clusters[i].get_true_energy());
        c.set_true_label(clusters[i].get_true_label());
        c.set_true_interaction(clusters[i].get_true_interaction());
        c.set_min_distance_from_true_pos(clusters[i].get_min_distance_from_true_pos());
        c.set_supernova_tp_fraction(clusters[i].get_supernova_tp_fraction());
        c.set_reco_pos(clusters[i].get_reco_pos());        
        clusters_in_volume.push_back(c);
    }
    write_clusters_to_root(clusters_in_volume, output_dir+"clusters_in_volume.root");

    return 0;
}