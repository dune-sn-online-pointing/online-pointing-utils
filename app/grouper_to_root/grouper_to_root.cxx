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

std::vector<std::vector<int>> file_reader(std::vector<std::string> filenames, int plane=2, bool take_only_supernova = false) { // plane 0 1 2 = u v x
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
                if (i == 0 || i == 1 || i == 2 || i == 3 || i == 4 || i == 5 || i == 11 || i == 12 || i == 13) {
                // if (i == 0 || i == 1 || i == 2 || i == 3 || i == 4 || i == 5 || i == 9 || i == 10 || i == 11) {
                    tp.push_back(val);
                }
                ++i;
            }
            // std::cout << tp[0] << " " << tp[1] << " " << tp[2] << " " << tp[3]<< " "  << tp[4]<< " "  << tp[5] << " " << tp[6] << " " << tp[7] << " " << tp[8] << std::endl; 
            if (take_only_supernova) {
                if (tp[8] == plane and tp[6] == 1) {
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

int main(int argc, char** argv) {
    // Parse the arguments
    std::string filename;
    std::string outfolder;
    int ticks_limit;
    int channel_limit;
    int min_tps_to_group;
    int plane;
    bool take_only_supernova = false;
    // define an array containing ['U', 'V', 'X']
    std::vector<std::string> plane_names = {"U", "V", "X"};

    if (argc == 8) {
        filename = argv[1];
        outfolder = argv[2];
        ticks_limit = std::stoi(argv[3]);
        channel_limit = std::stoi(argv[4]);
        min_tps_to_group = std::stoi(argv[5]);
        plane = std::stoi(argv[6]);
        if (std::stoi(argv[7]) == 1) {
            take_only_supernova = true;
        }
        // Check that the arguments are valid
        if (ticks_limit < 0 || channel_limit < 0 || min_tps_to_group < 0) {
            std::cout << "Usage: grouper_to_root <filename> <outfolder> <ticks_limit> <channel_limit> <min_tps_to_group> <plane> <take_only_supernova>" << std::endl;
            return 1;
        }
        // If outfolder does not exist, create it 

        std::string command = "mkdir -p " + outfolder + plane_names[plane];
        system(command.c_str());
    }
    else {
        std::cout << "Usage: grouper_to_root <filename> <outfolder> <ticks_limit> <channel_limit> <min_tps_to_group>" << std::endl;
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
    std::vector<std::vector<int>> tps = file_reader(filenames, plane, take_only_supernova);
    std::cout << "Number of tps: " << tps.size() << std::endl;
    // group the TPs
    std::vector<std::vector<std::vector<int>>> groups = group_maker(tps, ticks_limit, channel_limit, min_tps_to_group);
    std::cout << "Number of groups: " << groups.size() << std::endl;
    // write the groups to a root file

    // create the root file
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