#ifndef BACKTRACKING_H
#define BACKTRACKING_H

#include "root.h"
#include "TriggerPrimitive.hpp"
#include <vector>

std::vector<float> calculate_position(TriggerPrimitive* tp);
std::vector<std::vector<float>> validate_position_calculation(std::vector<TriggerPrimitive*> tps);
float eval_y_knowing_z_U_plane(std::vector<TriggerPrimitive*> tps, float z, float x_sign);
float eval_y_knowing_z_V_plane(std::vector<TriggerPrimitive*> tps, float z, float x_sign);
float vectorDistance(std::vector<float> a, std::vector<float> b);


void get_first_and_last_event(TTree* tree, int* branch_value, int which_event, int& first_entry, int& last_entry);

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

#endif // BACKTRACKING_H
