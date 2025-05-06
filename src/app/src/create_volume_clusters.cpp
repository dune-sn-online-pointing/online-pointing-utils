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
#include "cluster.h"
#include "create_volume_clusters_libs.h"


LoggerInit([]{ Logger::getUserHeader() << "[" << FILENAME << "]";});

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
    std::string output_dir = j["output_folder"];
    int radius = j["radius"];
    int plane = j["plane"];
    int supernova_option = j["supernova_option"];
    int max_events_per_filename = j["max_events_per_filename"];
    float threshold = j["threshold"];

    LogInfo << "tps_filename: " << tps_filename << std::endl;
    LogInfo << "cluster_filename: " << cluster_filename << std::endl;
    LogInfo << "predictions: " << predictions << std::endl;
    LogInfo << "output_dir: " << output_dir << std::endl;
    LogInfo << "radius: " << radius << std::endl;
    LogInfo << "plane: " << plane << std::endl;
    LogInfo << "supernova_option: " << supernova_option << std::endl;
    LogInfo << "max_events_per_filename: " << max_events_per_filename << std::endl;
    LogInfo << "threshold: " << threshold << std::endl;

    std::vector<std::string> filenames;
    std::ifstream infile(tps_filename);
    std::string line;
    LogInfo<<"Opening file: "<< tps_filename << std::endl;
    while (std::getline(infile, line)) {
        filenames.push_back(line);
    }
    LogInfo << "Number of files: " << filenames.size() << std::endl;

    std::vector<TriggerPrimitive> tps_object       = file_reader(filenames, plane, supernova_option, max_events_per_filename);
    // create a vector of TriggerPrimitive pointers
    std::vector<TriggerPrimitive*> tps;
    for (int i = 0; i < tps_object.size(); i++) tps.push_back(&tps_object[i]);
    
    std::vector<cluster> clusters           = read_clusters_from_root(cluster_filename);
    std::vector<float> predictions_vector   = read_predictions(predictions);

    LogInfo << "Number of clusters: " << clusters.size() << std::endl;
    LogInfo << "Number of predictions: " << predictions_vector.size() << std::endl;
    LogInfo << "Number of tps: " << tps.size() << std::endl;


    std::vector<cluster> clusters_in_volume;
    for (int i = 0; i < clusters.size(); i++) {
        if (i % 100 == 0) {
            LogInfo << "Cluster number: " << i << std::endl;
        }
        if (predictions_vector[i] < threshold) {
            continue;
        }
        std::vector<TriggerPrimitive*> tps_around_cluster = get_tps_around_cluster(tps, clusters[i], radius);
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