#include <vector>
#include <iostream>
#include <algorithm>
#include "match_clusters_libs.h"
#include "cluster.h"
// #include "position_calculator.h"


bool are_compatibles(cluster& c_u, cluster& c_v, cluster& c_x, float radius) {
    if (int(c_u.get_tps()[0][3]/2560) != int(c_x.get_tps()[0][3]/2560)) {
        return false;
    }
    if (int(c_v.get_tps()[0][3]/2560) != int(c_x.get_tps()[0][3]/2560)) {
        return false;
    }

    float z = c_x.get_reco_pos()[2];
    float x_sign = c_x.get_reco_pos()[0] > 0 ? 1 : -1;
    float y_pred_u = eval_y_knowing_z_U_plane(c_u.get_tps(), z, x_sign);
    float y_pred_v = eval_y_knowing_z_V_plane(c_v.get_tps(), z, x_sign);
    float dist_y = std::abs(y_pred_u - y_pred_v);
    if (dist_y > radius) {
        return false;
    }

    float x_u = std::abs(c_u.get_reco_pos()[0]);
    float x_v = std::abs(c_v.get_reco_pos()[0]);
    float x_x = std::abs(c_x.get_reco_pos()[0]);
    float dist_x = std::max({std::abs(x_u - x_x), std::abs(x_v - x_x), std::abs(x_u - x_v)});
    if (dist_x > radius) {
        return false;
    }

    return true;
}

bool match_with_true_pos(cluster& c_u, cluster& c_v, cluster& c_x, float radius){
    float true_x = c_x.get_true_pos()[0];
    float true_y = c_x.get_true_pos()[1];
    float true_z = c_x.get_true_pos()[2];
    // Check U cluster
    if (std::abs(std::abs(c_u.get_reco_pos()[0]) - std::abs(true_x)) > radius) {
        return false;
    }
    
    if (std::abs(std::abs(std::abs(eval_y_knowing_z_U_plane(c_u.get_tps(), true_z, true_x > 0 ? 1 : -1))- std::abs(true_y))) > radius) {
        return false;
    }
    // Check V cluster
    if (std::abs(std::abs(c_v.get_reco_pos()[0]) - std::abs(true_x)) > radius) {
        return false;
    }
    if (std::abs(std::abs(eval_y_knowing_z_V_plane(c_v.get_tps(), true_z, true_x > 0 ? 1 : -1)) - std::abs(true_y)) > radius) {
        return false;
    }
    // Check X cluster
    if (std::abs(std::abs(c_x.get_reco_pos()[0]) - std::abs(true_x)) > radius) {
        return false;
    }
    if (std::abs(std::abs(c_x.get_reco_pos()[2]) - std::abs(true_z)) > radius) {
        return false;
    }
    return true;
}

cluster join_clusters(cluster c_u, cluster c_v, cluster c_x){
    std::vector<std::vector<double>> tps;
    for (int i = 0; i < c_u.get_size(); i++) {
        tps.push_back(c_u.get_tps()[i]);
    }
    for (int i = 0; i < c_v.get_size(); i++) {
        tps.push_back(c_v.get_tps()[i]);
    }
    for (int i = 0; i < c_x.get_size(); i++) {
        tps.push_back(c_x.get_tps()[i]);
    }

    cluster c(tps);
    c.set_true_pos(c_x.get_true_pos());
    c.set_true_dir(c_x.get_true_dir());
    c.set_true_energy(c_x.get_true_energy());
    c.set_true_label(c_x.get_true_label());
    c.set_true_interaction(c_x.get_true_interaction());
    c.set_min_distance_from_true_pos(c_x.get_min_distance_from_true_pos());
    c.set_supernova_tp_fraction(c_x.get_supernova_tp_fraction());  
    std::vector<float> reco_pos = {0, 0, 0};
    reco_pos = c_x.get_reco_pos();
    reco_pos[1] = (eval_y_knowing_z_U_plane(c_u.get_tps(), c_x.get_reco_pos()[2], c_x.get_reco_pos()[0] > 0 ? 1 : -1) + eval_y_knowing_z_V_plane(c_v.get_tps(), c_x.get_reco_pos()[2], c_x.get_reco_pos()[0] > 0 ? 1 : -1)) / 2;
    c.set_reco_pos(reco_pos);

    return c;
}


