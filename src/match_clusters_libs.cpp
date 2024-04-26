#include <vector>
#include <iostream>
#include <algorithm>
#include "match_clusters_libs.h"
#include "cluster.h"
#include "position_calculator.h"


bool are_compatibles(cluster& c_u, cluster& c_v, cluster& c_x, float radius) {
    if (int(c_u.get_tps()[0][3]/2560) != int(c_x.get_tps()[0][3]/2560)) {
        return false;
    }
    if (int(c_v.get_tps()[0][3]/2560) != int(c_x.get_tps()[0][3]/2560)) {
        return false;
    }

    float z = c_x.get_reco_pos()[2];
    float x_sign = c_x.get_reco_pos()[0] > 0 ? 1 : -1;
    std::vector<float> y_pred_u = eval_y_knowing_z_U_plane(c_u.get_tps(), z, x_sign);
    std::vector<float> y_pred_v = eval_y_knowing_z_V_plane(c_v.get_tps(), z, x_sign);

    float x_u = std::abs(c_u.get_reco_pos()[0]);
    float x_v = std::abs(c_v.get_reco_pos()[0]);
    float x_x = std::abs(c_x.get_reco_pos()[0]);
    float dist_x = std::max({std::abs(x_u - x_x), std::abs(x_v - x_x), std::abs(x_u - x_v)});
    if (dist_x > radius) {
        return false;
    }

    bool y_overlaps = false;
    for (int i = 0; i < y_pred_u.size(); i++) {
        for (int j = 0; j < y_pred_v.size(); j++) {
            if (std::abs(y_pred_u[i] - y_pred_v[j]) < radius) {
                y_overlaps = true;
                break;
            }
        }
    }
    return y_overlaps;
}



