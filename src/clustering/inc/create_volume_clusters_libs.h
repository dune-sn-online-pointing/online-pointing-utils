#ifndef __CREATE_VOLUME_CLUSTERS_LIBS_H__
#define __CREATE_VOLUME_CLUSTERS_LIBS_H__

#include <fstream>
#include <string>
#include <ctime>
#include <vector>
#include "cluster.h"
// #include "position_calculator.h"


std::vector<float> read_predictions(std::string predictions);
std::vector<std::vector<double>> get_tps_around_cluster(std::vector<std::vector<double>> tps, cluster cluster, int radius);
#endif