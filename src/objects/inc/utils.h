#ifndef UTILS_H
#define UTILS_H

#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <set>

// Include parameter headers for constants moved out of utils.h
#include "../../../parameters/geometry.h"
#include "../../../parameters/timing.h"
#include "../../../parameters/conversion.h"

// #include "TriggerPrimitive.hpp"

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
}

// Constants moved to parameters/*.h above


#endif // UTILS_H