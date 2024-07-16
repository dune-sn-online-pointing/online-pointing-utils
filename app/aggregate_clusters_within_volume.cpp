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
    std::string outfolder = j["output_folder"];
    float radius = j["radius"];
    std::string prediction_file = j["predictions"];
    float threshold = j["threshold"];



    std::cout << "filename: " << filename << std::endl;
    std::vector<cluster> clusters = read_clusters_from_root(filename);
    // read the prediction file
    std::cout << "prediction_file: " << prediction_file << std::endl;
    std::ifstream infile(prediction_file);
    std::string line;
    std::vector<float> predictions;
    while (std::getline(infile, line))
    {
        float pred = std::stof(line);
        predictions.push_back(pred);
    }
    // check if the number of clusters and predictions match
    if (clusters.size() != predictions.size())
    {
        LogError << "The number of clusters and predictions do not match" << std::endl;
        return 1;
    }
    std::vector<cluster> aggregated_clusters = aggregate_clusters_within_volume(clusters, radius, predictions, threshold);

    std::string root_filename = outfolder + "/aggregated_clusters_rad_" + std::to_string(radius) + "_thr_" + std::to_string(threshold) + ".root";
    
    write_clusters_to_root(clusters, root_filename);
    return 0;
}