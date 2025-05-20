#ifndef UTILS_H
#define UTILS_H

#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <set>

// #include "TriggerPrimitive.hpp"

static std::vector<std::string> views = {"U", "V", "X"};



// APA number of channels
namespace APA{
    static const int induction_channels = 800;
    static const int collection_channels = 960;
    static const int total_channels = induction_channels*2 + collection_channels; 
    static const std::vector<std::string> views = {"U", "V", "X"};
    // map of the views to the channels
    static std::map<std::string, int> channels_in_view = {
        {"U", APA::induction_channels},
        {"V", APA::induction_channels},
        {"X", APA::collection_channels}
    };
}

// namespace CRP{
// }


namespace PDG{
    static const int electron = 11;
    static const int nue = 12;
    static const int muon = 13;
    static const int numu = 14;
    static const int photon = 22;
    static const int proton = 2212;
    static const int neutron = 2112;
    static const int pion_plus = 211;
    static const int pion_minus = -211;
    static const int kaon_plus = 321;
    static const int kaon_minus = -321;
    static const int kaon_zero = 310;
    static const int kaon_zero_bar = -310;
}

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
static const int backtracker_error_margin = 4; // prob remove
static const double apa_angular_coeff = tan(apa_angle * M_PI / 180);

// TODO very temporary, these should be configurable parameters
static const double adc_to_energy_conversion_factor = 4000; // ADC to MeV conversion factor
static int time_window = 32*32; // in tpc ticks times conversion to tdc ticks
static double drift_speed =  1.61e-4 ; // from wc config, this is cm/ns
static int tdc_tick = 16; // in ns
static const float TPC_sample_length = 512.; // ns
static const float clock_tick = 16.; // ns
static int conversion_tdc_to_tpc = 32; 


#endif // UTILS_H