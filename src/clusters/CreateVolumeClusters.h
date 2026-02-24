#ifndef __CREATE_VOLUME_CLUSTERS_LIBS_H__
#define __CREATE_VOLUME_CLUSTERS_LIBS_H__

#include <fstream>
#include <string>
#include <ctime>
#include <vector>
#include "Cluster.h"
// #include "PositionCalculator.h"


std::vector<float> read_predictions(std::string predictions);
std::vector<TriggerPrimitive*> get_tps_around_cluster(std::vector<TriggerPrimitive*> tps, Cluster Cluster, int radius);

#endif // __CREATE_VOLUME_CLUSTERS_LIBS_H__