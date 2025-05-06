
#include <fstream>
#include <string>
#include <ctime>
#include <vector>
#include <iostream>
#include "create_volume_clusters_libs.h"
#include "cluster.h"
// #include "position_calculator.h"



std::vector<float> read_predictions(std::string predictions) {
    std::vector<float> predictions_vector;
    std::ifstream file(predictions);
    std::string line;
    while (std::getline(file, line)) {
        predictions_vector.push_back(std::stof(line));
    }
    return predictions_vector;
}

std::vector<TriggerPrimitive*> get_tps_around_cluster(std::vector<TriggerPrimitive*> tps, cluster cluster, int radius){
    std::vector<TriggerPrimitive*> tps_around_cluster;
    // search the region of interest with a binary search
    int min_time_start = cluster.get_tps().at(0)->time_start;
    int max_time_end = cluster.get_tps().at(cluster.get_tps().size() - 1)->time_start;
    double radius_in_ticks = radius/0.08;
    int start = 0;
    int end = tps.size() - 1;
    int mid = 0;
    int goal = min_time_start - 1.2*radius_in_ticks;

    while (start < end) {
        mid = (start + end) / 2;
        if (tps[mid]->time_start < goal) {
            start = mid + 1;
        } else {
            end = mid;
        }
    }


    float cluster_x = cluster.get_reco_pos()[0];
    float cluster_y = cluster.get_reco_pos()[1];
    float cluster_z = cluster.get_reco_pos()[2];


    while (tps.at(start)->time_start < max_time_end + 1.2*radius_in_ticks) {

        std::vector<float> tp_pos = calculate_position(tps.at(start));
        float tp_x = tp_pos[0];
        float tp_y = tp_pos[1];
        float tp_z = tp_pos[2];
        float distance = sqrt(pow(tp_x - cluster_x, 2) + pow(tp_y - cluster_y, 2) + pow(tp_z - cluster_z, 2));

        if (distance < radius) {
            tps_around_cluster.push_back(tps[start]);
        }

        start++;
        if (start == tps.size()) {
            break;
        }

    }

    return tps_around_cluster;
}
