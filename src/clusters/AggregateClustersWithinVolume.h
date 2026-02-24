#ifndef AGGREGATE_CLUSTERS_H
#define AGGREGATE_CLUSTERS_H

#include <vector>
#include <cmath>
// #include "PositionCalculator.h"
#include "Cluster.h"

std::vector<Cluster> aggregate_clusters_within_volume(std::vector<Cluster> clusters, float radius, std::vector<float> predictions, float threshold);


#endif
