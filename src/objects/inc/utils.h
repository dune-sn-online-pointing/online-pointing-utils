#ifndef UTILS_H
#define UTILS_H

#include <map>
#include <string>
#include <vector>
#include <cmath>

// #include "TriggerPrimitive.hpp"

static std::vector<std::string> views = {"U", "V", "X"};

static const float TPC_sample_length = 512.; // ns
static const float clock_tick = 16.; // ns

// APA number of channels
namespace APA{
    static const int induction_channels = 800;
    static const int collection_channels = 960;
    static const int total_channels = induction_channels*2 + collection_channels; // APA::total_channels
    static const std::vector<std::string> views = {"U", "V", "X"};
}

// namespace CRP{
// }


static const int EVENTS_OFFSET = 0; // what for? TODO remove I think
static const double apa_lenght_in_cm = 230;
static const double wire_pitch_in_cm_collection = 0.479;
static const double wire_pitch_in_cm_induction_diagonal = 0.4669;
static const double apa_angle = (90 - 35.7);
static const double wire_pitch_in_cm_induction = wire_pitch_in_cm_induction_diagonal / sin(apa_angle * M_PI / 180);
static const double offset_between_apa_in_cm = 2.4;
static const double apa_height_in_cm = 598.4;
static const double time_tick_in_cm = 0.0805;
static const double apa_width_in_cm = 4.7;
static const int backtracker_error_margin = 4;
static const double apa_angular_coeff = tan(apa_angle * M_PI / 180);


#endif // UTILS_H