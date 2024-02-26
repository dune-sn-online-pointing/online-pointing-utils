#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

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
    clp.addOption("filename",    {"-f", "--filename"}, "Filename containing the filenames to read");
    clp.addOption("outfolder",    {"-o", "--outfolder"}, "Output folder");
    clp.addOption("ticks_limit",    {"-t", "--ticks-limit"}, "Ticks limit");
    clp.addOption("channel_limit",    {"-c", "--channel-limit"}, "Channel limit");
    clp.addOption("min_tps_to_cluster",    {"-m", "--min-tps-to-cluster"}, "Minimum number of TPs to form a cluster");
    clp.addOption("plane",    {"-p", "--plane"}, "Plane");
    clp.addOption("supernova_option",    {"-s", "--supernova-option"}, "Supernova option");
    clp.addOption("main_track_option",    {"-mt", "--main-track-option"}, "Main track option");
    clp.addOption("max_events_per_filename",    {"-me", "--max-events-per-filename"}, "Max events per filename");
    clp.addOption("adc_integral_cut",    {"-a", "--adc-integral-cut"}, "ADC integral cut");

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

    std::string filename = clp.getOptionVal<std::string>("filename");
    std::string outfolder = clp.getOptionVal<std::string>("outfolder");
    int ticks_limit = clp.getOptionVal<int>("ticks_limit");
    int channel_limit = clp.getOptionVal<int>("channel_limit");
    int min_tps_to_cluster = clp.getOptionVal<int>("min_tps_to_cluster");
    int plane = clp.getOptionVal<int>("plane");
    int supernova_option = clp.getOptionVal<int>("supernova_option");
    int main_track_option = clp.getOptionVal<int>("main_track_option");
    int max_events_per_filename = clp.getOptionVal<int>("max_events_per_filename");
    int adc_integral_cut = clp.getOptionVal<int>("adc_integral_cut");
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
    std::cout << "XYZ map created" << std::endl;
    // cluster the tps
    std::vector<cluster> clusters = cluster_maker(tps, ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut);
    std::cout << "Number of clusters: " << clusters.size() << std::endl;
    // filter the clusters
    if (main_track_option == 1) {
        clusters = filter_main_tracks(clusters);
    } else if (main_track_option == 2) {
        clusters = filter_out_main_track(clusters);
    }
    std::cout << "Number of clusters after filtering: " << clusters.size() << std::endl;
    // add true x y z dir 
    for (int i = 0; i < clusters.size(); i++) {
        clusters[i].set_true_dir(file_idx_to_true_xyz_map[clusters[i].get_tp(0)[clusters[i].get_tp(0).size() - 1]]);
    }

    // write the clusters to a root file
    std::string root_filename = outfolder + "/" + plane_names[plane] + "/clusters_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_cluster_" + std::to_string(min_tps_to_cluster) + ".root";
    write_clusters_to_root(clusters, root_filename);
    std::cout << "clusters written to " << root_filename << std::endl;

    // stop the clock
    std::clock_t end;
    end = std::clock();
    return 0;
}

