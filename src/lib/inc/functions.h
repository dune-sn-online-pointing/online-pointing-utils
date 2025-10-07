#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "TriggerPrimitive.hpp"
#include "cluster.h"
#include "utils.h"

#include "root.h"


// this is just a vector of pointers, to not create copies of the TriggerPrimitive objects
void getPrimitivesForView( std::string view, std::vector<TriggerPrimitive>& tps, std::vector<TriggerPrimitive*>& tps_per_view);

bool isTimeCompatible(TrueParticle* true_particle, TriggerPrimitive* tp, int time_window);

bool isChannelCompatible (TrueParticle* true_particle, TriggerPrimitive* tp);

extern const std::map<std::string, ELogLevel> rootVerbLevelMap;

ELogLevel stringToRootLevel(const std::string& level);


#endif // FUNCTIONS_H