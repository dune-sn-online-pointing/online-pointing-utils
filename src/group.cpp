#include <iostream>
#include <vector>
#include "../inc/group.h"
#include "../inc/position_calculator.h" 

std::map<std::string, int> variables_to_index = {
    {"start_tick", 0},
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
    std::vector<int> reco_pos = {0, 0, 0};
    for (int i = 0; i < tps_.size(); i++) {
        std::vector<int> pos = calculate_position(tps_[i]);
        std::vector<int> true_pos = {tps_[i][variables_to_index["true_x"]], tps_[i][variables_to_index["true_y"]], tps_[i][variables_to_index["true_z"]]};
        float distance = sqrt(pow(pos[0] - true_pos[0], 2) + pow(pos[1] - true_pos[1], 2) + pow(pos[2] - true_pos[2], 2));
        if (distance < min_distance) {
            min_distance = distance;
            true_pos_ = true_pos;
            true_energy_ = tps_[i][variables_to_index["true_energy"]];
            true_label_ = tps_[i][variables_to_index["true_label"]];
        }
        
        reco_pos[0] += pos[0];
        reco_pos[1] += pos[1];
        reco_pos[2] += pos[2];
    }
    min_distance_from_true_pos_ = min_distance;
    reco_pos[0] /= tps_.size();
    reco_pos[1] /= tps_.size();
    reco_pos[2] /= tps_.size();
    reco_pos_ = reco_pos;
}




