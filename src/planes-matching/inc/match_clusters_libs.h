#ifndef MATCH_CLUSTERS_LIBS_HD
#define MATCH_CLUSTERS_LIBS_HD
#include <vector>
#include "cluster.h"
// #include "position_calculator.h"

bool are_compatibles(cluster& c_u, cluster& c_v, cluster& c_x, float radius);
bool match_with_true_pos(cluster& c_u, cluster& c_v, cluster& c_x, float radius);

cluster join_clusters(cluster c_u, cluster c_v, cluster c_x);    


#endif