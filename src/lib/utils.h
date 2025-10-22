#ifndef UTILS_H
#define UTILS_H

#include <nlohmann/json.hpp>

#include "ParametersManager.h"
#include "root.h"

// File utility functions
// std::vector<std::string> find_input_files(const nlohmann::json& j, const std::string& file_suffix = "_tpstream.root");
std::vector<std::string> find_input_files(const nlohmann::json& j, const std::vector<std::string>& file_suffixes);
std::vector<std::string> find_input_files(const nlohmann::json& j, const std::string& pattern);

std::string toLower(std::string s);

bool ensureDirectoryExists(const std::string& folder);

// Time conversion utilities (declared here, defined in utils.cpp to avoid circular dependency)
int toTPCticks(int tdcTicks);

int toTDCticks(int tpcTicks);

void bindBranch(TTree* tree, const char* name, void* address);

std::string getClustersFolder(const nlohmann::json& j);

template <typename T> bool SetBranchWithFallback(TTree* , std::initializer_list<const char*>, T*, const std::string& );


// APAs
static std::vector<std::string> views = {"U", "V", "X"};

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

// CRPs, placeholder for now
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
    static const int alpha = 1000020040;
}

// Geometry parameters - accessed via ParametersManager
inline double get_apa_length_cm() { return GET_PARAM_DOUBLE("geometry.apa_length_cm"); }
inline double get_wire_pitch_collection_cm() { return GET_PARAM_DOUBLE("geometry.wire_pitch_collection_cm"); }
inline double get_wire_pitch_induction_cm() { return GET_PARAM_DOUBLE("geometry.wire_pitch_induction_cm"); }
inline double get_apa_angle_deg() { return GET_PARAM_DOUBLE("geometry.apa_angle_deg"); }
inline double get_offset_between_apa_cm() { return GET_PARAM_DOUBLE("geometry.offset_between_apa_cm"); }
inline double get_apa_height_cm() { return GET_PARAM_DOUBLE("geometry.apa_height_cm"); }
inline double get_apa_width_cm() { return GET_PARAM_DOUBLE("geometry.apa_width_cm"); }
inline double get_apa_angular_coeff() { return GET_PARAM_DOUBLE("geometry.apa_angular_coeff"); }

// Timing parameters - accessed via ParametersManager
inline double get_time_tick_cm() { return GET_PARAM_DOUBLE("timing.time_tick_cm"); }
inline double get_drift_speed() { return GET_PARAM_DOUBLE("timing.drift_speed"); }
inline int get_conversion_tdc_to_tpc() { return GET_PARAM_INT("timing.conversion_tdc_to_tpc"); }
inline double get_clock_tick_ns() { return GET_PARAM_DOUBLE("timing.clock_tick_ns"); }
inline double get_tpc_sample_length_ns() { return GET_PARAM_DOUBLE("timing.tpc_sample_length_ns"); }
inline int get_time_window() { return GET_PARAM_INT("timing.time_window"); }
inline int get_backtracker_error_margin() { return GET_PARAM_INT("timing.backtracker_error_margin"); }

// Conversion parameters - accessed via ParametersManager
inline double get_adc_to_energy_factor() { return GET_PARAM_DOUBLE("conversion.adc_to_energy_factor"); }

// Legacy constants for backward compatibility (deprecated - use get_* functions instead)
#define apa_lenght_in_cm get_apa_length_cm()
#define wire_pitch_in_cm_collection get_wire_pitch_collection_cm()
#define wire_pitch_in_cm_induction get_wire_pitch_induction_cm()
#define offset_between_apa_in_cm get_offset_between_apa_cm()
#define apa_height_in_cm get_apa_height_cm()
#define apa_width_in_cm get_apa_width_cm()
#define apa_angular_coeff get_apa_angular_coeff()
#define time_tick_in_cm get_time_tick_cm()
#define drift_speed get_drift_speed()
#define conversion_tdc_to_tpc get_conversion_tdc_to_tpc()
#define clock_tick get_clock_tick_ns()
#define TPC_sample_length get_tpc_sample_length_ns()
#define time_window get_time_window()
#define backtracker_error_margin get_backtracker_error_margin()
#define adc_to_energy_conversion_factor get_adc_to_energy_factor()

#endif // UTILS_H
