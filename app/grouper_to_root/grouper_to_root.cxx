#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <cmath>
// include root libraries
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"
#include "TMatrixD.h"
#include "cpp_vector_dict.cxx"

const double apa_lenght_in_cm = 230;
const double wire_pitch_in_cm_collection = 0.479;
const double wire_pitch_in_cm_induction_diagonal = 0.4669;
const double apa_angle = (90 - 35.7);
const double wire_pitch_in_cm_induction = wire_pitch_in_cm_induction_diagonal / sin(apa_angle * M_PI / 180);
const double offset_between_apa_in_cm = 2.4;
const double apa_height_in_cm = 598.4;
const double time_tick_in_cm = 0.0805;
const double apa_width_in_cm = 4.7;
const int backtracker_error_margin = 4;
const double apa_angular_coeff = tan(apa_angle * M_PI / 180);

/*
idx = {
    'time_start': 0,
    'time_over_threshold': 1,
    'time_peak': 2,
    'channel': 3,
    'adc_integral': 4,
    'adc_peak': 5,
    'mc_truth': 6,
    'event_number': 7,
    'plane': 8
}

1581 
14 
1590 
30710 
2038 
273
11 
1 
1 
1 
0 
3 truth
2 event num
2 plane
0

*/
std::vector<std::vector<std::vector<int>>> group_maker(std::vector<std::vector<int>>& all_tps, int ticks_limit=100, int channel_limit=20, int min_tps_to_group=1) {
    std::vector<std::vector<std::vector<int>>> groups;
    std::vector<std::vector<std::vector<int>>> buffer;
    for (auto& tp : all_tps) {
        // std::cout << tp[0] << " " << tp[1] << " " << tp[2] << " " << tp[3]<< " "  << tp[4]<< " "  << tp[5] << " " << tp[6] << " " << tp[7] << " " << tp[8] << std::endl;
        if (buffer.size() == 0) {
            std::vector<std::vector<int>> temp;
            temp.push_back(tp);
            buffer.push_back(temp);
        }
        else {
            std::vector<std::vector<std::vector<int>>> buffer_copy = buffer;
            buffer.clear();
            bool appended = false;
            int idx = 0;
            int idx_appended ;
            for (auto& candidate : buffer_copy) {
                // get a the max containing the times of the TPs in the candidate
                int max_time = 0;
                for (auto& tp2 : candidate) {
                    max_time = std::max(max_time, tp2[0] + tp2[1]);
                }
                bool time_cond = (tp[0] - max_time) <= ticks_limit;
                if (time_cond) {
                    bool chan_cond = false;
                    for (auto& tp2 : candidate) {
                        if (std::abs(tp[3] - tp2[3]) <= channel_limit) {
                            chan_cond = true;
                            break;
                        }
                    }
                    if (chan_cond) {
                        // std::cout << "chan" << std::endl;
                        if (!appended) {
                            candidate.push_back(tp);
                            buffer.push_back(candidate);
                            appended = true;
                            idx_appended = idx;
                            // std::cout << "appended" << std::endl;
                        }
                        else {
                            for (auto& tp2 : candidate) {
                                buffer[idx_appended].push_back(tp2);
                            }
                        }
                    }
                    else {
                        buffer.push_back(candidate);
                        ++idx;
                    }
                }
                else {
                    // std::cout << "not time" << std::endl<< std::endl<< std::endl<< std::endl;
                    if (candidate.size() >= min_tps_to_group) {
                        groups.push_back(candidate);
                        // std::cout << "pushed" << std::endl;
                    }
                }
            }
            if (!appended) {
                std::vector<std::vector<int>> temp;
                temp.push_back(tp);
                buffer.push_back(temp);
            }
        }
    }
    if (buffer.size() > 0) {
        for (auto& candidate : buffer) {
            if (candidate.size() >= min_tps_to_group) {
                groups.push_back(candidate);
            }
        }
    }

    return groups;
}

std::vector<std::vector<int>> file_reader(std::vector<std::string> filenames, int plane=2, int supernova_option=0) { // plane 0 1 2 = u v x
    std::vector<std::vector<int>> tps;
    std::string line;
    int n_events_offset = 0;
    for (auto& filename : filenames) {
        std::ifstream infile(filename);

        // read and save the TPs
        while (std::getline(infile, line)) {
            std::istringstream iss(line);
            std::vector<int> tp;
            int val;
            int i = 0;
            while (iss >> val) {
                // (0,1,2,3,4,5,11,12,13)
                if (i == 0 || i == 1 || i == 2 || i == 3 || i == 4 || i == 5 || i == 11 || i == 12 || i == 13 || i == 14 || i == 15 || i == 16 ) {
                // if (i == 0 || i == 1 || i == 2 || i == 3 || i == 4 || i == 5 || i == 9 || i == 10 || i == 11) {
                    tp.push_back(val);
                }
                ++i;
            }
            // std::cout << tp[0] << " " << tp[1] << " " << tp[2] << " " << tp[3]<< " "  << tp[4]<< " "  << tp[5] << " " << tp[6] << " " << tp[7] << " " << tp[8] << std::endl; 
            if (supernova_option==1) {
                if (tp[8] == plane and tp[6] == 1) {
                tp[7] += n_events_offset;
                tp[0] += 4792 * tp[7];
                tp[2] += 4792 * tp[7];
                    tps.push_back(tp);
                }
            }
            else if (supernova_option==2) {
                if (tp[8] == plane and tp[6] != 1) {
                tp[7] += n_events_offset;
                tp[0] += 4792 * tp[7];
                tp[2] += 4792 * tp[7];
                    tps.push_back(tp);
                }
            }
            else {
                // if (true) {
                if (tp[8] == plane) {
                    tp[7] += n_events_offset;
                    tp[0] += 4792 * tp[7];
                    tp[2] += 4792 * tp[7];
                    tps.push_back(tp);
                }
            }
        }
        n_events_offset = tps[tps.size()-1][7];
    }

    // sort the TPs by time
    std::sort(tps.begin(), tps.end(), [](const std::vector<int>& a, const std::vector<int>& b) {
        return a[0] < b[0];
    });

    return tps;
}


float calculate_distance_from_true(std::vector<std::vector<int>>& group) {
    float min_distance = 100000000;
    for (int i=0; i<group.size(); i++) {
        int z_apa_offset = group[i][3] / (2560*2) * (apa_lenght_in_cm + offset_between_apa_in_cm);
        int z_channel_offset = ((group[i][3] % 2560 - 1600) % 480) * wire_pitch_in_cm_collection;
        int z = wire_pitch_in_cm_collection + z_apa_offset + z_channel_offset;
        int y = 0;
        int x_signs = ((group[i][3] % 2560-2080)<0) ? -1 : 1;
        int x = ((group[i][2] - 4792* group[i][7] )* time_tick_in_cm + apa_width_in_cm/2) * x_signs;

        int true_x = group[i][9];
        int true_y = group[i][10];
        int true_z = group[i][11];

        float dists = std::sqrt(std::pow((x - true_x),2) + std::pow((z - true_z),2));
        if (dists < min_distance) {
            min_distance = dists;
        }
    }

    return min_distance;
}



std::vector<std::vector<std::vector<int>>> filter_main_tracks(std::vector<std::vector<std::vector<int>>>& groups) { // valid only if the groups are ordered by event and for clean sn data
    std::vector<std::vector<std::vector<int>>> main_tracks;
    std::vector<std::vector<int>> main_group;
    int current_event = groups[0][0][7];
    main_group = groups[0];
    for (auto& group : groups) {
        if (group[0][7] != current_event) {
            main_tracks.push_back(main_group);
            main_group.clear();
            current_event = group[0][7];
            main_group=group;
        }
        else {
            if (calculate_distance_from_true(group) < calculate_distance_from_true(main_group)) {
                main_group = group;
            }
        }
    }
    std::cout << "Number of main tracks: " << main_tracks.size() << std::endl;
    return main_tracks;
}

std::vector<std::vector<std::vector<int>>> filter_out_main_track(std::vector<std::vector<std::vector<int>>>& groups) { // valid only if the groups are ordered by event and for clean sn data
    std::vector<std::vector<std::vector<int>>> blips;
    int current_event = groups[0][0][7];
    std::vector<int> bad_idx_list;
    std::vector<std::vector<int>> main_group;
    float min_distance = 100000000;

    int idx = 0;
    for (auto& group : groups) {
        if (group[0][7] != current_event) {
            bad_idx_list.push_back(idx);
            main_group.clear();
            current_event = group[0][7];
            main_group=group;
        }
        else {
            if (calculate_distance_from_true(group) < min_distance and group[0][6] == 1) {
                main_group = group;
                min_distance = calculate_distance_from_true(group);
            }
        }
    idx += 1;
    }

    for (int i=0; i<groups.size(); i++) {
        if (std::find(bad_idx_list.begin(), bad_idx_list.end(), i) == bad_idx_list.end()) {
            blips.push_back(groups[i]);
        }
    }


    std::cout << "Number of blips: " << blips.size() << std::endl;
    return blips;
}



int main(int argc, char** argv) {
    // Parse the arguments
    std::string filename;
    std::string outfolder;
    int ticks_limit;
    int channel_limit;
    int min_tps_to_group;
    int plane=2;
    int supernova_option = 0;
    int main_track_option = 0;
    // define an array containing ['U', 'V', 'X']
    std::vector<std::string> plane_names = {"U", "V", "X"};

    if (argc >3) {
        filename = argv[1];
        outfolder = argv[2];
        ticks_limit = std::stoi(argv[3]);
        channel_limit = std::stoi(argv[4]);
        min_tps_to_group = std::stoi(argv[5]);
        plane = std::stoi(argv[6]);
        supernova_option = std::stoi(argv[7]);
        main_track_option = std::stoi(argv[8]);
        // Check that the arguments are valid
        if (ticks_limit < 0 || channel_limit < 0 || min_tps_to_group < 0) {
            std::cout << "Usage: grouper_to_root <filename> <outfolder> <ticks_limit> <channel_limit> <min_tps_to_group> <plane> <supernova_option> <main_track_option>" << std::endl;
            return 1;
        }
        // If outfolder does not exist, create it 

        std::string command = "mkdir -p " + outfolder + plane_names[plane];
        system(command.c_str());
    }
    else {
        std::cout << "Usage: grouper_to_root <filename> <outfolder> <ticks_limit> <channel_limit> <min_tps_to_group> <plane> <supernova_option> <main_track_option>" << std::endl;
        return 1;
    }


    // start timer
    std::clock_t start = std::clock();
    // filename is the name of the file containing the filenames to read
    std::vector<std::string> filenames;
    // read the file containing the filenames and save them in a vector
    std::ifstream infile(filename);
    std::string line;
    // read and save the TPs
    while (std::getline(infile, line)) {
        filenames.push_back(line);
    }



    // read the file, each row is a TP
    std::vector<std::vector<int>> tps = file_reader(filenames, plane, supernova_option);
    std::cout << "Number of tps: " << tps.size() << std::endl;
    // group the TPs
    std::vector<std::vector<std::vector<int>>> groups = group_maker(tps, ticks_limit, channel_limit, min_tps_to_group);
    std::cout << "Number of groups: " << groups.size() << std::endl;
    // filter only main tracks
    if (main_track_option == 1){
        groups = filter_main_tracks(groups);
    }
    else if (main_track_option == 2){
        groups = filter_out_main_track(groups);
    }


    // write the groups to a root file
    // create the root file
    // std::string root_filename = outfolder + "/" + plane_names[plane] + "/groups_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_group_2.root";
    std::string root_filename = outfolder + "/" + plane_names[plane] + "/groups_tick_limits_" + std::to_string(ticks_limit) + "_channel_limits_" + std::to_string(channel_limit) + "_min_tps_to_group_" + std::to_string(min_tps_to_group) + ".root";
    TFile* file = new TFile(root_filename.c_str(), "RECREATE");
    // prepare objects to write to the root file
    int nrows;
    int event;
    std::vector<std::vector<int>> matrix;

    // Define variables for your matrices
    TTree* tree = new TTree("treeMatrix", "treeMatrix");
    tree->Branch("Matrix", &matrix);
    tree->Branch("NRows", &nrows);
    tree->Branch("Event", &event);

    // create a vector
    std::vector<int> eventss;

    for (int i=0; i<groups.size(); i++){
        
        nrows = groups[i].size();

        event = groups[i][0][7];
        eventss.push_back(event);
        // matrix.emplace_back(groups[i]);
        matrix = groups[i];

        // Fill the tree

        tree->Fill();
    }

    // Write the tree to the file
    file->Write();
    // Close the file
    file->Close();
    // print execution time in seconds
    std::cout << "Execution time: " << (std::clock() - start) / static_cast<double>(CLOCKS_PER_SEC) << " seconds" << std::endl;

    return 0;
}