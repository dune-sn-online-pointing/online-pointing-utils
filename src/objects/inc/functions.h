#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "TriggerPrimitive.hpp"
#include "cluster.h"
#include "utils.h"


// this is just a vector of pointers, to not create copies of the TriggerPrimitive objects
void getPrimitivesForView( std::string view, std::vector<TriggerPrimitive>& tps, std::vector<TriggerPrimitive*>& tps_per_view);

bool isTimeCompatible(int simide_timestamp, TriggerPrimitive tp, int time_window);

#endif // FUNCTIONS_H