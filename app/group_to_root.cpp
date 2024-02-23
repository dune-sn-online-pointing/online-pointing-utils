#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

#include "position_calculator.h"
#include "group_to_root_libs.h"
#include "group.h"

int main(int argc, char* argv[]) {
    std::string filename;
    std::string outfolder;
    int ticks_limit = 3;
    int channel_limit = 1;
    int min_tps_to_group = 1;
    int plane = 2;
    int supernova_option = 0;
    int main_track_option = 0;
    int max_events_per_filename = INT_MAX;
    int adc_integral_cut = 0;
    std::vector<std::string> plane_names = {"U", "V", "X"};

    if (argc < 6) {
        std::cout << "Usage: group_to_root <filename> <outfolder> <ticks_limit> <channel_limit> <min_tps_to_group> <plane> <supernova_option> <main_track_option> <max_events_per_filename> <adc_integral_cut>" << std::endl;
        return 1;        
    } else {
        filename = argv[1];
        outfolder = argv[2];
        if (argc > 3) ticks_limit = std::stoi(argv[3]);
        if (argc > 4) channel_limit = std::stoi(argv[4]);
        if (argc > 5) min_tps_to_group = std::stoi(argv[5]);
        if (argc > 6) plane = std::stoi(argv[6]);
        if (argc > 7) supernova_option = std::stoi(argv[7]);
        if (argc > 8) main_track_option = std::stoi(argv[8]);
        if (argc > 9) max_events_per_filename = std::stoi(argv[9]);
        if (argc > 10) adc_integral_cut = std::stoi(argv[10]);
    }

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
    // group the tps
    std::vector<group> groups = group_maker(tps, ticks_limit, channel_limit, min_tps_to_group, adc_integral_cut);
    std::cout << "Number of groups: " << groups.size() << std::endl;
    // filter the groups
    if (main_track_option == 1) {
        groups = filter_main_tracks(groups);
    } else if (main_track_option == 2) {
        groups = filter_out_main_track(groups);
    }
    std::cout << "Number of groups after filtering: " << groups.size() << std::endl;
    // add true x y z dir 
    for (int i = 0; i < groups.size(); i++) {
        groups[i].set_true_dir(file_idx_to_true_xyz_map[groups[i].get_tp(0)[groups[i].get_tp(0).size() - 1]]);
    }

    // write the groups to a root file
    std::string root_filename = outfolder + "/" + plane_names[plane] + "/groups_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_group_" + std::to_string(min_tps_to_group) + ".root";
    write_groups_to_root(groups, root_filename);
    std::cout << "Groups written to " << root_filename << std::endl;

    // stop the clock
    std::clock_t end;
    end = std::clock();
    return 0;
}

