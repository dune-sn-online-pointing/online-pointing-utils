// this is a .h file that contains the functions to read tps from files and create clusters
#ifndef cluster_TO_ROOT_LIBS_H
#define cluster_TO_ROOT_LIBS_H

#include "Cluster.h"

// create the clusters from the tps
bool channel_condition_with_pbc(TriggerPrimitive* tp1, TriggerPrimitive* tp2, int channel_limit);
std::vector<Cluster> make_cluster(std::vector<TriggerPrimitive*> all_tps, int ticks_limit=3, int channel_limit=1, int min_tps_to_cluster=1, int adc_integral_cut=0);

// create a map connectig the file index to the true x y z
// std::map<int, std::vector<float>> file_idx_to_true_xyz(std::vector<std::string> filenames);

// std::map<int, int> file_idx_to_true_interaction(std::vector<std::string> filenames);

// take all the clusters and return only main tracks
std::vector<Cluster> filter_main_tracks(std::vector<Cluster>& clusters);

// take all the clusters and return only blips
std::vector<Cluster> filter_out_main_track(std::vector<Cluster>& clusters);

// assing a different label to the main tracks
// void assing_different_label_to_main_tracks(std::vector<Cluster>& clusters, int new_label=77);

// write the clusters to a root file
void write_clusters(std::vector<Cluster>& clusters, std::string root_filename, std::string view);

std::vector<Cluster> read_clusters_from_root(std::string root_filename);

std::map<int, std::vector<Cluster>> create_event_mapping(std::vector<Cluster>& clusters);

std::map<int, std::vector<TriggerPrimitive>> create_background_event_mapping(std::vector<TriggerPrimitive>& bkg_tps);

// Write condensed TPs and truth to a ROOT file for later clustering
void write_tps(
	const std::string& out_filename,
	const std::vector<std::vector<TriggerPrimitive>>& tps_by_event,
	const std::vector<std::vector<TrueParticle>>& true_particles_by_event,
	const std::vector<std::vector<Neutrino>>& neutrinos_by_event);

// Read condensed TPs and truth back from a ROOT file
void read_tps(
	const std::string& in_filename,
	std::map<int, std::vector<TriggerPrimitive>>& tps_by_event,
	std::map<int, std::vector<TrueParticle>>& true_particles_by_event,
	std::map<int, std::vector<Neutrino>>& neutrinos_by_event);


#endif


