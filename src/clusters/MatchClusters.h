#ifndef MATCH_CLUSTERS_LIBS_HD
#define MATCH_CLUSTERS_LIBS_HD
#include <vector>
#include "Cluster.h"
// #include "PositionCalculator.h"

bool are_compatibles(Cluster& c_u, Cluster& c_v, Cluster& c_x, float radius);
bool match_with_true_pos(Cluster& c_u, Cluster& c_v, Cluster& c_x, float radius);

Cluster join_clusters(Cluster c_u, Cluster c_v, Cluster c_x);
Cluster join_clusters(Cluster c1, Cluster c2);  // For partial matches (2 planes)    


#endif