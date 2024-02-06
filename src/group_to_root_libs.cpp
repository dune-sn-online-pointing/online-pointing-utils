#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <climits>
#include "../inc/group_to_root_libs.h"
#include "../inc/group.h"

// include root libraries
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"
#include "TMatrixD.h"


// read the tps from the files and save them in a vector
std::vector<std::vector<int>> file_reader(std::vector<std::string> filenames, int plane, int supernova_option, int max_events_per_filename) {
    std::vector<std::vector<int>> tps;
    std::string line;
    int n_events_offset = 0;
    int file_idx = 0;
    for (auto& filename : filenames) {
        // std::cout << filename << std::endl;
        std::ifstream infile(filename);
        // read and save the TPs
        while (std::getline(infile, line)) {

            std::istringstream iss(line);
            std::vector<int> tp;
            int val;
            while (iss >> val) {
                tp.push_back(val);
            }
            tp.push_back(file_idx);
            if (tp[variables_to_index["event"]]>max_events_per_filename) {
                break;
            }

            if (tp[variables_to_index["view"]] != plane) {
                continue;
            }

            if (supernova_option == 1 && tp[variables_to_index["ptype"]] == 1) {
                tp[variables_to_index["event"]] += n_events_offset;
                tp[variables_to_index["time_start"]] += n_events_offset*tp[variables_to_index["event"]];
                tp[variables_to_index["time_peak"]] += n_events_offset*tp[variables_to_index["event"]];
                tps.push_back(tp);
            }   
            else if (supernova_option == 2 && tp[variables_to_index["ptype"]] != 1) {
                tp[variables_to_index["event"]] += n_events_offset;
                tp[variables_to_index["time_start"]] += n_events_offset*tp[variables_to_index["event"]];       
                tp[variables_to_index["time_peak"]] += n_events_offset*tp[variables_to_index["event"]];
                tps.push_back(tp);
            }
            else if (supernova_option == 0) {
                tp[variables_to_index["event"]] += n_events_offset;
                tp[variables_to_index["time_start"]] += n_events_offset*tp[variables_to_index["event"]];       
                tp[variables_to_index["time_peak"]] += n_events_offset*tp[variables_to_index["event"]];
                tps.push_back(tp);
            }
        }
        if (tps.size() > 0){
            if (tps[tps.size()-1][variables_to_index["event"]] != n_events_offset) {
                // std::cout << "File Works" << std::endl;
            }
            else {
                std::cout << filename << std::endl;
                std::cout << "File does not work" << std::endl;
            }
            n_events_offset = tps[tps.size()-1][variables_to_index["event"]];
        }
        else {
            std::cout << filename << std::endl;
            std::cout << "File does not work" << std::endl;
        }
        ++file_idx;
    }
        // sort the TPs by time
    std::sort(tps.begin(), tps.end(), [](const std::vector<int>& a, const std::vector<int>& b) {
        return a[0] < b[0];
    });

    return tps;
}

std::vector<group> group_maker(std::vector<std::vector<int>>& all_tps, int ticks_limit, int channel_limit, int min_tps_to_group, int adc_integral_cut) {
    std::vector<std::vector<std::vector<int>>> buffer;
    std::vector<group> groups;
    for (auto& tp : all_tps) {
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
            int idx_appended;
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
                        int adc_integral = 0;
                        for (auto& tp2 : candidate) {
                            adc_integral += tp2[4];
                        }
                        if (adc_integral > adc_integral_cut) {
                            group g(candidate);
                            groups.push_back(g);
                        }
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
                int adc_integral = 0;
                for (auto& tp : candidate) {
                    adc_integral += tp[4];
                }
                if (adc_integral > adc_integral_cut) {
                    group g(candidate);
                    groups.push_back(g);
                }
            }
        }
    }

    return groups;
}

// create a map connectig the file index to the true x y z
std::map<int, std::vector<float>> file_idx_to_true_xyz(std::vector<std::string> filenames) {
    std::map<int, std::vector<float>> file_idx_to_true_xyz;
    std::string line;
    float true_x;
    float true_y;
    float true_z;
    int n_events_offset = 0;
    int file_idx = 0;
    for (auto& filename : filenames) {
        std::ifstream infile(filename);
        // split the filename by / and change the last element to this_custom_direction.txt
        std::vector<std::string> split_filename;
        std::string delimiter = "/";
        size_t pos = 0;
        std::string token;
        while ((pos = filename.find(delimiter)) != std::string::npos) {
            token = filename.substr(0, pos);
            split_filename.push_back(token);
            filename.erase(0, pos + delimiter.length());
        }
        split_filename.push_back("this_custom_direction.txt");
        std::string new_filename = "";
        for (auto& split : split_filename) {
            new_filename += split + "/";
        }
        new_filename.pop_back();
        // std::cout << new_filename << std::endl;
        // check if the file exists      
        infile = std::ifstream(new_filename);
        if (!infile.good()) {
            std::cout << "File does not exist" << std::endl;
            file_idx_to_true_xyz[file_idx] = {0, 0, 0};
            continue;
        }
        else {
        // read new filename and save the true x y z
        std::getline(infile, line);
        std::istringstream iss(line);
        iss >> true_x >> true_y >> true_z; 
        // std::cout << true_x << " " << true_y << " " << true_z << std::endl;
        file_idx_to_true_xyz[file_idx] = {true_x, true_y, true_z};
        }
        ++file_idx;
    }

    return file_idx_to_true_xyz;
}

std::vector<group> filter_main_tracks(std::vector<group>& groups) { // valid only if the groups are ordered by event and for clean sn data
    std::vector<group> main_tracks;
    group main_track;
    int event = groups[0].get_tp(0)[variables_to_index["event"]];
    main_track = groups[0];
    for (auto& g : groups) {
        if (g.get_tp(0)[variables_to_index["event"]] != event) {
            main_tracks.push_back(main_track);
            main_track = g;
            event = g.get_tp(0)[variables_to_index["event"]];
        }
        else {
            if (g.get_true_label() == 1 and g.get_min_distance_from_true_pos() < main_track.get_min_distance_from_true_pos()){
                main_track = g;
            }
        }
    }
    
    return main_tracks;
}

std::vector<group> filter_out_main_track(std::vector<group>& groups) { // valid only if the groups are ordered by event and for clean sn data
    std::vector<group> blips;
    int current_event = groups[0].get_tp(0)[variables_to_index["event"]];
    group main_group;
    float min_distance = 100000000;
    std::vector<int> bad_idx_list;

    int idx = 0;
    for (auto& group : groups) {
        if (group.get_tp(0)[variables_to_index["event"]] != current_event) {
            bad_idx_list.push_back(idx);
            main_group = group;
            current_event = group.get_tp(0)[variables_to_index["event"]];
        }
        else {
            if (group.get_min_distance_from_true_pos() < min_distance and group.get_true_label() == 1) {
                main_group = group;
                min_distance = group.get_min_distance_from_true_pos();
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

void write_groups_to_root(std::vector<group>& groups, std::string root_filename) {
    // create folder if it does not exist
    std::string folder = root_filename.substr(0, root_filename.find_last_of("/"));
    std::string command = "mkdir -p " + folder;
    system(command.c_str());
    // create the root file
    TFile *f = new TFile(root_filename.c_str(), "recreate");
    TTree *tree = new TTree("tree", "tree");
    // prepare objects to save the data
    std::vector<std::vector<int>> matrix;
    int nrows;
    int event;
    float true_dir_x;
    float true_dir_y;
    float true_dir_z;
    float true_energy;
    int true_label;
    int reco_pos_x; 
    int reco_pos_y;
    int reco_pos_z;

    // create the branches
    tree->Branch("matrix", &matrix);
    tree->Branch("nrows", &nrows);
    tree->Branch("event", &event);
    tree->Branch("true_dir_x", &true_dir_x);
    tree->Branch("true_dir_y", &true_dir_y);
    tree->Branch("true_dir_z", &true_dir_z);
    tree->Branch("true_energy", &true_energy);
    tree->Branch("true_label", &true_label);
    tree->Branch("reco_pos_x", &reco_pos_x);
    tree->Branch("reco_pos_y", &reco_pos_y);
    tree->Branch("reco_pos_z", &reco_pos_z);

    // fill the tree
    for (auto& g : groups) {
        matrix = g.get_tps();
        nrows = g.get_size();
        event = g.get_tp(0)[variables_to_index["event"]];
        true_dir_x = g.get_true_dir()[0];
        true_dir_y = g.get_true_dir()[1];
        true_dir_z = g.get_true_dir()[2];
        true_energy = g.get_true_energy();
        true_label = g.get_true_label();
        reco_pos_x = g.get_reco_pos()[0];
        reco_pos_y = g.get_reco_pos()[1];
        reco_pos_z = g.get_reco_pos()[2];
        tree->Fill();
    }
    // write the tree
    tree->Write();
    f->Close();

    return;   
}

