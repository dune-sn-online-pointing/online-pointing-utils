#ifndef __CREATE_VOLUME_CLUSTERS_LIBS_H__
#define __CREATE_VOLUME_CLUSTERS_LIBS_H__

#include <fstream>
#include <string>
#include <ctime>
#include <vector>
#include "cluster.h"
// #include "position_calculator.h"


std::vector<float> read_predictions(std::string predictions);
std::vector<TriggerPrimitive*> get_tps_around_cluster(std::vector<TriggerPrimitive*> tps, cluster cluster, int radius);

#endif // __CREATE_VOLUME_CLUSTERS_LIBS_H__