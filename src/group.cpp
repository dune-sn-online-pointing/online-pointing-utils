#include <iostream>
#include <vector>
#include "../inc/group.h"
#include "../inc/position_calculator.h" 

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
    {"track_id", 19}
};


void group::update_group_info() {
    // the reconstructed position will be the average of the tps
    // we want to save the minimum distance from the true position
    float min_distance = 1000000;
    float supernova_tp_fraction = 0;
    std::vector<int> reco_pos = {0, 0, 0};
    int true_label = tps_[0][variables_to_index["ptype"]];
    for (int i = 0; i < tps_.size(); i++) {
        std::vector<int> pos = calculate_position(tps_[i]);
        std::vector<int> true_pos = {tps_[i][variables_to_index["true_x"]], tps_[i][variables_to_index["true_y"]], tps_[i][variables_to_index["true_z"]]};
        float distance = sqrt(pow(pos[0] - true_pos[0], 2) + pow(pos[2] - true_pos[2], 2));
        // float distance = sqrt(pow(pos[0] - true_pos[0], 2) + pow(pos[1] - true_pos[1], 2) + pow(pos[2] - true_pos[2], 2));
        if (distance < min_distance and tps_[i][variables_to_index["ptype"]] == 1) {
            min_distance = distance;
            true_pos_ = true_pos;
            true_energy_ = tps_[i][variables_to_index["true_energy"]];
        }
        if (tps_[i][variables_to_index["ptype"]] == 1) {
            supernova_tp_fraction++;
        }

        reco_pos[0] += pos[0];
        reco_pos[1] += pos[1];
        reco_pos[2] += pos[2];
        if (tps_[i][variables_to_index["ptype"]] != true_label) {
            true_label = -1;
        }
    }

    supernova_tp_fraction_ = supernova_tp_fraction / tps_.size();
    min_distance_from_true_pos_ = min_distance;
    int ntps = tps_.size();
    reco_pos[0] /= ntps;
    reco_pos[1] /= ntps;
    reco_pos[2] /= ntps;
    reco_pos_ = reco_pos;
    true_label_ = true_label;
}




