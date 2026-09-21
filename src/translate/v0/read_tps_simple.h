#ifndef READ_TPS_SIMPLE_H
#define READ_TPS_SIMPLE_H

#include "TTree.h"

#include <vector>
#include "TriggerPrimitive.hpp"
// #include "TrueParticle.h"
// #include "Neutrino.h"

void read_tps_simple(TTree* tpTree, UInt_t theevent, UInt_t therun,
    UInt_t lo, UInt_t hi,
    std::vector<TriggerPrimitive>& tps_by_event);
#endif