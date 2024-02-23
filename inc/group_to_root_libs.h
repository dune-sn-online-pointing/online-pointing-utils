// this is a .h file that contains the functions to read tps from files and create groups
#ifndef GROUP_TO_ROOT_LIBS_H
#define GROUP_TO_ROOT_LIBS_H

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <climits>

// include root libraries
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"
#include "TMatrixD.h"

#include "group.h"

// read the tps from the files and save them in a vector
std::vector<std::vector<double>> file_reader(std::vector<std::string> filenames, int plane=2, int supernova_option=0, int max_events_per_filename = INT_MAX);

// create the groups from the tps
std::vector<group> group_maker(std::vector<std::vector<double>>& all_tps, int ticks_limit=3, int channel_limit=1, int min_tps_to_group=1, int adc_integral_cut=0);

// create a map connectig the file index to the true x y z
std::map<int, std::vector<float>> file_idx_to_true_xyz(std::vector<std::string> filenames);

// take all the groups and return only main tracks
std::vector<group> filter_main_tracks(std::vector<group>& groups);

// take all the groups and return only blips
std::vector<group> filter_out_main_track(std::vector<group>& groups);

// write the groups to a root file
void write_groups_to_root(std::vector<group>& groups, std::string root_filename);

std::vector<group> read_groups_from_root(std::string root_filename);

std::map<int, std::vector<group>> create_event_mapping(std::vector<group>& groups);

#endif


