#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "TriggerPrimitive.hpp"
#include "Cluster.h"
#include "Utils.h"

#include "root.h"


// this is just a vector of pointers, to not create copies of the TriggerPrimitive objects
void getPrimitivesForView( std::string view, std::vector<TriggerPrimitive>& tps, std::vector<TriggerPrimitive*>& tps_per_view);

bool isTimeCompatible(TrueParticle* true_particle, TriggerPrimitive* tp, int time_window);

bool isChannelCompatible (TrueParticle* true_particle, TriggerPrimitive* tp);

extern const std::map<std::string, ELogLevel> rootVerbLevelMap;

ELogLevel stringToRootLevel(const std::string& level);

std::map<std::string, int> extractClusteringParams(const std::string& filename);

std::string formatClusteringConditions(const std::map<std::string, int>& params);

#endif // FUNCTIONS_H