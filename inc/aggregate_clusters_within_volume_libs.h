#ifndef AGGREGATE_CLUSTERS_H
#define AGGREGATE_CLUSTERS_H

#include <vector>
#include <cmath>
#include "position_calculator.h"
#include "cluster.h"

std::vector<cluster> aggregate_clusters_within_volume(std::vector<cluster> clusters, float radius, std::vector<float> predictions, float threshold);


#endif
