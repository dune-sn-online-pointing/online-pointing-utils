#ifndef BACKTRACKING_H
#define BACKTRACKING_H

#include "TriggerPrimitive.hpp"

std::vector<float> calculate_position(TriggerPrimitive* tp);
std::vector<std::vector<float>> validate_position_calculation(std::vector<TriggerPrimitive*> tps);
float eval_y_knowing_z_U_plane(std::vector<TriggerPrimitive*> tps, float z, float x_sign);
float eval_y_knowing_z_V_plane(std::vector<TriggerPrimitive*> tps, float z, float x_sign);

void get_first_and_last_event(TTree* tree, UInt_t* branch_value, int which_event, int& first_entry, int& last_entry);

// read the tps from the files and save them in a vector
// std::vector<TriggerPrimitive> read_tpstream(std::vector<std::string> filenames, int plane=2, int supernova_option=0, int max_events_per_filename = INT_MAX);
void read_tpstream(std::string filename,
				 std::vector<TriggerPrimitive>& tps,
				 std::vector<TrueParticle>& true_particles,
				 std::vector<Neutrino>& neutrinos,
				 int supernova_option = 0,
				 int event_number = 0,
				 double time_tolerance_ticks = -1.0,
				 int channel_tolerance = -1);
                 
// Direct TP-SimIDE matching based on time and channel proximity
void match_tps_to_simides_direct(
	std::vector<TriggerPrimitive>& tps,
	std::vector<TrueParticle>& true_particles,
	TFile* file,
	int event_number,
	double time_tolerance_ticks = 50.0,
	int channel_tolerance = 5);

// Write condensed TPs and truth to a ROOT file for later clustering
void write_tps(
	const std::string& out_filename,
	const std::vector<std::vector<TriggerPrimitive>>& tps_by_event,
	const std::vector<std::vector<TrueParticle>>& true_particles_by_event,
	const std::vector<std::vector<Neutrino>>& neutrinos_by_event);


#endif // BACKTRACKING_H

