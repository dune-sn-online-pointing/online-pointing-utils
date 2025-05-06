// this is a .h file that contains the functions to read tps from files and create clusters
#ifndef cluster_TO_ROOT_LIBS_H
#define cluster_TO_ROOT_LIBS_H

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

#include "cluster.h"

// read the tps from the files and save them in a vector
std::vector<TriggerPrimitive> file_reader(std::vector<std::string> filenames, int plane=2, int supernova_option=0, int max_events_per_filename = INT_MAX);
// std::vector<std::vector<TriggerPrimitive>> file_reader_all_planes(std::vector<std::string> filenames, int supernova_option=0, int max_events_per_filename= INT_MAX);

// create the clusters from the tps
bool channel_condition_with_pbc(double ch1, double ch2, int channel_limit);
std::vector<cluster> cluster_maker(std::vector<TriggerPrimitive*> all_tps, int ticks_limit=3, int channel_limit=1, int min_tps_to_cluster=1, int adc_integral_cut=0);

// create a map connectig the file index to the true x y z
std::map<int, std::vector<float>> file_idx_to_true_xyz(std::vector<std::string> filenames);

std::map<int, int> file_idx_to_true_interaction(std::vector<std::string> filenames);

// take all the clusters and return only main tracks
std::vector<cluster> filter_main_tracks(std::vector<cluster>& clusters);

// take all the clusters and return only blips
std::vector<cluster> filter_out_main_track(std::vector<cluster>& clusters);

// assing a different label to the main tracks
void assing_different_label_to_main_tracks(std::vector<cluster>& clusters, int new_label=77);

// write the clusters to a root file
void write_clusters_to_root(std::vector<cluster>& clusters, std::string root_filename);

std::vector<cluster> read_clusters_from_root(std::string root_filename);

std::map<int, std::vector<cluster>> create_event_mapping(std::vector<cluster>& clusters);

std::map<int, std::vector<TriggerPrimitive>> create_background_event_mapping(std::vector<TriggerPrimitive>& bkg_tps);

#endif


