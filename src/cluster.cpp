#include <iostream>
#include <vector>

#include "cluster.h"
#include "position_calculator.h" 

std::map<std::string, int> variables_to_index = {
    {"time_start", 0},
    {"time_over_threshold", 1},
    {"time_peak", 2},
    {"channel", 3},
    {"adc_integral", 4},
    {"adc_peak", 5},
    {"detid", 6},
    {"type", 7},
    {"algorithm", 8},
    {"version", 9},
    {"flag", 10},
    {"ptype", 11},
    {"event", 12},
    {"view", 13},
    {"true_x", 14},
    {"true_y", 15},
    {"true_z", 16},
    {"true_energy", 17},
    {"n_electrons", 18},
    {"track_id", 19},  
    {"electron_energy", 20},
    {"true_e_px", 21},
    {"true_e_py", 22},
    {"true_e_pz", 23}
};


void cluster::update_cluster_info() {
    // the reconstructed position will be the average of the tps
    // we want to save the minimum distance from the true position
    float min_distance = 1000000;
    std::vector<float> pos = calculate_position(tps_[0]);
    min_distance = sqrt(pow(pos[0] - true_pos_[0], 2) + pow(pos[2] - true_pos_[2], 2));

    float supernova_tp_fraction = 0;
    float total_charge = 0;
    std::vector<float> reco_pos = {0, 0, 0};
    int true_label = tps_[0][variables_to_index["ptype"]];
    for (int i = 0; i < tps_.size(); i++) {
        total_charge += tps_[i][variables_to_index["adc_integral"]];
        std::vector<float> pos = calculate_position(tps_[i]);
        std::vector<float> true_pos = {float(tps_[i][variables_to_index["true_x"]]), float(tps_[i][variables_to_index["true_y"]]), float(tps_[i][variables_to_index["true_z"]])};
        // std::vector<int> true_pos = {tps_[i][variables_to_index["true_x"]], tps_[i][variables_to_index["true_y"]], tps_[i][variables_to_index["true_z"]]};
        float distance = sqrt(pow(pos[0] - true_pos[0], 2) + pow(pos[2] - true_pos[2], 2));
        // float distance = sqrt(pow(pos[0] - true_pos[0], 2) + pow(pos[1] - true_pos[1], 2) + pow(pos[2] - true_pos[2], 2));
        if (distance < min_distance and tps_[i][variables_to_index["ptype"]] == 1) {
            if (true_pos[0] != 0 and true_pos[1] != 0 and true_pos[2] != 0) {
                min_distance = distance;
                true_pos_ = true_pos;
                true_energy_ = tps_[i][variables_to_index["true_energy"]];
            }
        }
        if (tps_[i][variables_to_index["ptype"]] == 1) {
            supernova_tp_fraction++;
        }

        reco_pos[0] += pos[0];
        reco_pos[1] += pos[1];
        reco_pos[2] += pos[2];
        if (tps_[i][variables_to_index["ptype"]] != true_label) {
            true_label = -1;
            if (supernova_tp_fraction > 0) {
                true_label = 1;
            }
        }
    }

    supernova_tp_fraction_ = supernova_tp_fraction / tps_.size();
    min_distance_from_true_pos_ = min_distance;
    total_charge_ = total_charge;
    int ntps = tps_.size();
    reco_pos[0] /= ntps;
    reco_pos[1] /= ntps;
    reco_pos[2] /= ntps;
    reco_pos_ = reco_pos;
    true_label_ = true_label;
    if (tps_[0].size() >= variables_to_index["true_e_pz"]) {
        true_dir_ = {tps_[0][variables_to_index["true_e_px"]], tps_[0][variables_to_index["true_e_py"]], tps_[0][variables_to_index["true_e_pz"]]};
    }
}


