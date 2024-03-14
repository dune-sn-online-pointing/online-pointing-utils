#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <ctime>

#include "CmdLineParser.h"
#include "Logger.h"

#include "position_calculator.h"
#include "cluster_to_root_libs.h"
#include "cluster.h"
#include "superimpose_root_files_libs.h"

LoggerInit([]{
  Logger::getUserHeader() << "[" << FILENAME << "]";
});

int main(int argc, char* argv[]) {

    CmdLineParser clp;

    clp.getDescription() << "> superimpose_root_files app."<< std::endl;

    clp.addDummyOption("Main options");
    clp.addOption("sig_cluster_filename",    {"-s", "--sig-filename"}, "Signal clusters filename");
    clp.addOption("bkg_cluster_filename",    {"-b", "--bkg-filename"}, "Background clusters filename");
    clp.addOption("out_folder",               {"-o", "--output-folder"}, "Specify output directory path");
    clp.addOption("radius",                  {"-r", "--radius"}, "Radius to consider, in [m]");

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
  

    std::string sig_cluster_filename = clp.getOptionVal<std::string>("sig_cluster_filename");
    std::string bkg_cluster_filename = clp.getOptionVal<std::string>("bkg_cluster_filename");
    std::string outfolder = clp.getOptionVal<std::string>("out_folder");
    
    // default value 
    float radius = 1.0; // m 
    radius = clp.getOptionVal<double>("radius");

    // start the clock
    std::clock_t start;

    // read the clusters from the root files
    std::vector<cluster> sig_clusters = read_clusters_from_root(sig_cluster_filename);
    std::cout << "Number of sig clusters: " << sig_clusters.size() << std::endl;
    std::vector<cluster> bkg_clusters = read_clusters_from_root(bkg_cluster_filename);
    std::cout << "Number of bkg clusters: " << bkg_clusters.size() << std::endl;
    // create a map connecting the event number to the background clusters
    std::map<int, std::vector<cluster>> bkg_event_mapping = create_event_mapping(bkg_clusters);
    std::cout << "Bkg event mapping created" << std::endl;
    std::vector<int> bkg_list_of_event_numbers;
    for (auto const& x : bkg_event_mapping) {
        bkg_list_of_event_numbers.push_back(x.first);
    }
    std::cout << "Number of bkg events: " << bkg_list_of_event_numbers.size() << std::endl;

    // create a map connecting the event number to the signal clusters
    std::map<int, std::vector<cluster>> sig_event_mapping = create_event_mapping(sig_clusters);
    std::cout << "Sig event mapping created" << std::endl;
    std::vector<int> sig_list_of_event_numbers;
    for (auto const& x : sig_event_mapping) {
        sig_list_of_event_numbers.push_back(x.first);
    }
    std::cout << "Number of sig events: " << sig_list_of_event_numbers.size() << std::endl;

    // superimpose a random background event to a signal event
    std::vector<cluster> superimposed_clusters;
    std::vector<int> test_appo_vec;

    for (int i = 0; i < sig_list_of_event_numbers.size(); i++) {
        if (i%1000 ==0){
            std::cout<<"Event number: "<< i << std::endl;
        }
        int sig_event_number = sig_list_of_event_numbers[i];
        std::vector<cluster> sig_event_clusters = sig_event_mapping[sig_event_number];
        // get a random background event
        int bkg_event_number = bkg_list_of_event_numbers[rand() % bkg_list_of_event_numbers.size()];
        std::vector<cluster> bkg_event_clusters = bkg_event_mapping[bkg_event_number];
        while (int(bkg_event_clusters.size()) < 3000){
            std::cout << "WARNING: For cluster " << i << " Background event " << bkg_event_number << " has " << bkg_event_clusters.size() << " clusters" << std::endl;
            test_appo_vec.push_back(bkg_event_number);
            int bkg_event_number = bkg_list_of_event_numbers[rand() % bkg_list_of_event_numbers.size()];
            bkg_event_clusters = bkg_event_mapping[bkg_event_number];
        }
 
        // superimpose the two events by connecting the clusters        
        std::vector<cluster> superimposed_gp = sig_event_clusters;
        superimposed_gp.insert(superimposed_gp.end(), bkg_event_clusters.begin(), bkg_event_clusters.end());
        // filter the superimposed clusters within a radius
        // std::cout << "Index: " << i << " Superimposed cluster size: " << superimposed_gp.size() << std::endl;
        cluster filtered_superimposed_cluster = filter_clusters_within_radius(superimposed_gp, radius);
        if (filtered_superimposed_cluster.get_min_distance_from_true_pos()>10){
            std::cout<<"WARNING: Min distance from true pos: "<< filtered_superimposed_cluster.get_min_distance_from_true_pos() << std::endl;
            for (auto g: sig_event_clusters){
                std::cout<<"Sig cluster min distance from true pos: "<< g.get_min_distance_from_true_pos() << std::endl;
                for (auto tp: g.get_tps()){
                    for (auto var: tp){
                        std::cout<<var<<" ";
                    }
                    std::cout<<calculate_position(tp)[0] << " " << calculate_position(tp)[1] << " " << calculate_position(tp)[2] << std::endl;
                }
            }
            continue;
        }
        // else{
        //     std::cout<<"Min distance from true pos: "<< filtered_superimposed_cluster.get_min_distance_from_true_pos() << std::endl;
        // }
        superimposed_clusters.push_back(filtered_superimposed_cluster);
    }
    
    // print unique bkg event numbers with counts
    std::map<int, int> bkg_event_counts;
    for (auto const& x : test_appo_vec){
        bkg_event_counts[x]++;
    }
    for (auto const& x : bkg_event_counts){
        std::cout<<"Bkg event number: "<< x.first << " Count: " << x.second << " N events: " << bkg_event_mapping[x.first].size() << std::endl;
    }


    // write the superimposed clusters to a root file
    std::cout<<"Writing "<< superimposed_clusters.size() <<" events to root" << std::endl;
    std::string sig_name = sig_cluster_filename.substr(sig_cluster_filename.find_last_of("/")+1);
    std::string bkg_name = bkg_cluster_filename.substr(bkg_cluster_filename.find_last_of("/")+1);
    std::string superimposed_cluster_filename = outfolder + "/superimposed_radius_" + std::to_string(radius) + ".root";
    write_clusters_to_root(superimposed_clusters, superimposed_cluster_filename);
    // stop the clock
    std::clock_t end;
    double elapsed_time = double(end - start) / CLOCKS_PER_SEC;
    std::cout << "Elapsed time: " << elapsed_time << " seconds" << std::endl;

    return 0;
}

