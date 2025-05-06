#include <map>
#include <string>
#include <vector>

#include "TriggerPrimitive.hpp"

static std::vector<std::string> plane_names = {"U", "V", "X"};

static const float TPC_sampling_rate = 512.; // ns

// this is just a vector of pointers, to not create copies of the TriggerPrimitive objects
std::vector<TriggerPrimitive*>& getPrimitivesForView( std::string* view, std::vector<TriggerPrimitive>& tps);

const int EVENTS_OFFSET = 0; // what for? TODO remove I think
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