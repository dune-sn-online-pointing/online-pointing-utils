#include <vector>
#include <iostream>
#include <algorithm>
#include "match_clusters_libs.h"
#include "Cluster.h"
#include "geometry.h"
// #include "position_calculator.h"


bool are_compatibles(Cluster& c_u, Cluster& c_v, Cluster& c_x, float radius) {

    // check if they come from the same detector


    if (!(int(c_u.get_tp(0)->GetDetector()) == int(c_v.get_tp(0)->GetDetector()) && 
            int(c_u.get_tp(0)->GetDetector()) == int(c_x.get_tp(0)->GetDetector())))
        return false;

    float z = c_x.get_true_pos()[2];
    float x_sign = c_x.get_true_pos()[0] > 0 ? 1 : -1;
    float y_pred_u = eval_y_knowing_z_U_plane(c_u.get_tps(), z, x_sign);
    float y_pred_v = eval_y_knowing_z_V_plane(c_v.get_tps(), z, x_sign);
    float dist_y = std::abs(y_pred_u - y_pred_v);
    if (dist_y > radius) {
        return false;
    }

    float x_u = std::abs(c_u.get_true_pos()[0]);
    float x_v = std::abs(c_v.get_true_pos()[0]);
    float x_x = std::abs(c_x.get_true_pos()[0]);
    float dist_x = std::max({std::abs(x_u - x_x), std::abs(x_v - x_x), std::abs(x_u - x_v)});
    if (dist_x > radius) {
        return false;
    }

    return true;
}

bool match_with_true_pos(Cluster& c_u, Cluster& c_v, Cluster& c_x, float radius){
    float true_x = c_x.get_true_pos()[0];
    float true_y = c_x.get_true_pos()[1];
    float true_z = c_x.get_true_pos()[2];
    // Check U Cluster
    if (std::abs(std::abs(c_u.get_true_pos()[0]) - std::abs(true_x)) > radius) {
        return false;
    }
    
    if (std::abs(std::abs(std::abs(eval_y_knowing_z_U_plane(c_u.get_tps(), true_z, true_x > 0 ? 1 : -1))- std::abs(true_y))) > radius) {
        return false;
    }
    // Check V Cluster
    if (std::abs(std::abs(c_v.get_true_pos()[0]) - std::abs(true_x)) > radius) {
        return false;
    }
    if (std::abs(std::abs(eval_y_knowing_z_V_plane(c_v.get_tps(), true_z, true_x > 0 ? 1 : -1)) - std::abs(true_y)) > radius) {
        return false;
    }
    // Check X Cluster
    if (std::abs(std::abs(c_x.get_true_pos()[0]) - std::abs(true_x)) > radius) {
        return false;
    }
    if (std::abs(std::abs(c_x.get_true_pos()[2]) - std::abs(true_z)) > radius) {
        return false;
    }
    return true;
}

Cluster join_clusters(Cluster c_u, Cluster c_v, Cluster c_x){
    std::vector<TriggerPrimitive*> tps;
    
    // Set all TPs to same event (use X cluster's first TP event)
    int common_event = (c_x.get_size() > 0) ? c_x.get_tps()[0]->GetEvent() : 0;
    
    for (int i = 0; i < c_u.get_size(); i++) {
        TriggerPrimitive* tp = c_u.get_tps()[i];
        tp->SetEvent(common_event);  // Ensure consistent event
        tps.push_back(tp);
    }
    for (int i = 0; i < c_v.get_size(); i++) {
        TriggerPrimitive* tp = c_v.get_tps()[i];
        tp->SetEvent(common_event);  // Ensure consistent event
        tps.push_back(tp);
    }
    for (int i = 0; i < c_x.get_size(); i++) {
        tps.push_back(c_x.get_tps()[i]);
    }

    Cluster c(tps);
    c.set_true_pos(c_x.get_true_pos());
    c.set_true_dir(c_x.get_true_dir());
    c.set_true_momentum(c_x.get_true_momentum());
    c.set_true_energy(c_x.get_true_neutrino_energy());
    c.set_true_label(c_x.get_true_label());
    c.set_is_es_interaction(c_x.get_is_es_interaction());
    c.set_min_distance_from_true_pos(c_x.get_min_distance_from_true_pos());
    c.set_supernova_tp_fraction(c_x.get_supernova_tp_fraction());

    return c;
}


