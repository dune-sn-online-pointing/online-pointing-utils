#include <iostream>
#include <vector>

#include "cluster.h"

// std::map<std::string, int> variables_to_index = {
//     {"time_start", 0},
//     {"time_over_threshold", 1},
//     {"time_peak", 2},
//     {"channel", 3},
//     {"adc_integral", 4},
//     {"adc_peak", 5},
//     {"detid", 6},
//     {"type", 7},
//     {"algorithm", 8},
//     {"version", 9},
//     {"flag", 10},
//     {"ptype", 11},
//     {"Event", 12},
//     {"view", 13},
//     {"true_x", 14},
//     {"true_y", 15},
//     {"true_z", 16},
//     {"true_energy", 17},
//     {"n_electrons", 18},
//     {"track_id", 19},  
//     {"electron_energy", 20},
//     {"true_e_px", 21},
//     {"true_e_py", 22},
//     {"true_e_pz", 23}
// };


void cluster::update_cluster_info() {
    // the reconstructed position will be the average of the tps
    // we want to save the minimum distance from the true position
    // float min_distance = 1000000;
    // std::vector<float> pos = calculate_position(tps_[0]);
    // min_distance = sqrt(pow(pos[0] - true_pos_[0], 2) + pow(pos[2] - true_pos_[2], 2));

    // float supernova_tp_fraction = 0;
    // float total_charge = 0;
    // std::vector<float> reco_pos = {0, 0, 0};
    // int true_label = tps_[0][variables_to_index["ptype"]];
    // for (int i = 0; i < tps_.size(); i++) {
    //     total_charge += tps_[i][tp.adc_integral];
    //     std::vector<float> pos = calculate_position(tps_[i]);
    //     std::vector<float> true_pos = {float(tps_[i][variables_to_index["true_x"]]), float(tps_[i][variables_to_index["true_y"]]), float(tps_[i][variables_to_index["true_z"]])};
    //     // std::vector<int> true_pos = {tps_[i][variables_to_index["true_x"]], tps_[i][variables_to_index["true_y"]], tps_[i][variables_to_index["true_z"]]};
    //     float distance = sqrt(pow(pos[0] - true_pos[0], 2) + pow(pos[2] - true_pos[2], 2));
    //     // float distance = sqrt(pow(pos[0] - true_pos[0], 2) + pow(pos[1] - true_pos[1], 2) + pow(pos[2] - true_pos[2], 2));
    //     if (distance < min_distance and tps_[i][variables_to_index["ptype"]] == 1) {
    //         if (true_pos[0] != 0 and true_pos[1] != 0 and true_pos[2] != 0) {
    //             min_distance = distance;
    //             true_pos_ = true_pos;
    //             true_energy_ = tps_[i][variables_to_index["true_energy"]];
    //         }
    //     }
    //     if (tps_[i][variables_to_index["ptype"]] == 1) {
    //         supernova_tp_fraction++;
    //     }

    //     reco_pos[0] += pos[0];
    //     reco_pos[1] += pos[1];
    //     reco_pos[2] += pos[2];
    //     if (tps_[i][variables_to_index["ptype"]] != true_label) {
    //         true_label = -1;
    //         if (supernova_tp_fraction > 0) {
    //             true_label = 1;
    //         }
    //     }
    // }

    // supernova_tp_fraction_ = supernova_tp_fraction / tps_.size();
    // min_distance_from_true_pos_ = min_distance;
    // total_charge_ = total_charge;
    // int ntps = tps_.size();
    // reco_pos[0] /= ntps;
    // reco_pos[1] /= ntps;
    // reco_pos[2] /= ntps;
    // reco_pos_ = reco_pos;
    // true_label_ = true_label;
    // if (tps_[0].size() >= variables_to_index["true_e_pz"]) {
    //     true_dir_ = {tps_[0][variables_to_index["true_e_px"]], tps_[0][variables_to_index["true_e_py"]], tps_[0][variables_to_index["true_e_pz"]]};
    // }
}


std::vector<float> calculate_position(TriggerPrimitive* tp) { // only works for plane X
    float z;
    if (tp->view == "X") {
        float z_apa_offset = int(tp->channel) / (2560*2) * (apa_lenght_in_cm + offset_between_apa_in_cm);
        float z_channel_offset = ((int(tp->channel) % 2560 - 1600) % 480) * wire_pitch_in_cm_collection;
        z = wire_pitch_in_cm_collection + z_apa_offset + z_channel_offset;
    } else {
        z = 0;
    }

    float y = 0;
    float x_signs = ((int(tp->channel) % 2560-2080<0) ? -1 : 1);
    float x = ((int(tp->time_start) % EVENTS_OFFSET )* time_tick_in_cm + apa_width_in_cm/2) * x_signs; // check this TODO

    return {x, y, z};
}

std::vector<std::vector<float>> validate_position_calculation(std::vector<TriggerPrimitive*> tps) {
    std::vector<std::vector<float>> positions;
    // for (auto& tp : tps) {
    //     if (tp.time_start == 2 and tp[variables_to_index["true_x"]] and tp[variables_to_index["true_y"]] and tp[variables_to_index["true_z"]]) {
    //         positions.push_back({int(tp[variables_to_index["true_x"]]), int(tp[variables_to_index["true_y"]]), int(tp[variables_to_index["true_z"]]), calculate_position(tp)[0], calculate_position(tp)[1], calculate_position(tp)[2]});
    //     }
    // }
    return positions;
}

float distance(cluster cluster1, cluster cluster2) {
    float x1 = cluster1.get_reco_pos()[0];
    float y1 = cluster1.get_reco_pos()[1];
    float z1 = cluster1.get_reco_pos()[2];
    float x2 = cluster2.get_reco_pos()[0];
    float y2 = cluster2.get_reco_pos()[1];
    float z2 = cluster2.get_reco_pos()[2];
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2));
}

float eval_y_knowing_z_U_plane(std::vector<TriggerPrimitive> tps, float z, float x_sign) {
    z = z - int(tps.at(0).channel) / (2560*2) * (apa_lenght_in_cm + offset_between_apa_in_cm); // not sure about the 0 TODO
    float ordinate;
    std::vector<float> Y_pred;
    for (auto& tp : tps) {
        if ((int(tp.channel) / 2560) % 2 == 0) {
            if (x_sign < 0) {
                if (int(tp.channel) % 2560 < 400) {
                    if (z > (int(tp.channel) % 2560) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp.channel) % 2560) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp.channel) % 2560) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp.channel) % 2560 > 399) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp.channel) % 2560 - 400) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp.channel) % 2560 > 399) {
                    if (z < (799 - int(tp.channel) % 2560) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp.channel) % 2560 - 400) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (799 - int(tp.channel) % 2560) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp.channel) % 2560 < 400) {
                    ordinate = (z + (int(tp.channel) % 2560) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            }
        } else if ((int(tp.channel) / 2560) % 2 == 1) {
            if (x_sign < 0) {
                if (int(tp.channel) % 2560 < 400) {
                    if (z < (399 - int(tp.channel) % 2560) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp.channel) % 2560) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (399 - int(tp.channel) % 2560) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp.channel) % 2560 > 399) {
                    ordinate = (z + (int(tp.channel) % 2560 - 400) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp.channel) % 2560 > 399) {
                    if (z > (int(tp.channel) % 2560 - 400) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp.channel) % 2560 - 400) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp.channel) % 2560 - 400) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp.channel) % 2560 < 400) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp.channel) % 2560) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            }
        }
        // ordinate = (ordinate) - apa_height_in_cm if (tp[idx['channel']] / 2560) % 2 < 1 else apa_height_in_cm - (ordinate);
        if ((int(tp.channel) / 2560) % 2 < 1) {
            ordinate = (ordinate) - apa_height_in_cm;
        } else {
            ordinate = apa_height_in_cm - (ordinate);
        }
        Y_pred.push_back(ordinate);
    }
    // mean
    float Y_pred_mean = 0;
    for (auto& y : Y_pred) {
        Y_pred_mean += y;
    }
    Y_pred_mean = Y_pred_mean / Y_pred.size();

    return Y_pred_mean;
}


float eval_y_knowing_z_V_plane(std::vector<TriggerPrimitive*> tps, float z, float x_sign) {
    
    z = z - int(tps.at(0)->channel) / (2560*2) * (apa_lenght_in_cm + offset_between_apa_in_cm);
    float ordinate;
    std::vector<float> Y_pred;
    for (auto& tp : tps) {
        if ((int(tp->channel) / 2560) % 2 == 0) {
            if (x_sign < 0) {
                if (int(tp->channel) % 2560 < 1200) {
                    if (z < (1199 - int(tp->channel) % 2560) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp->channel) % 2560 - 800) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (1199 - int(tp->channel) % 2560) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp->channel) % 2560 > 1199) {
                    ordinate = (z + (int(tp->channel) % 2560 - 1200) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp->channel) % 2560 > 1199) {
                    if (z > (int(tp->channel) % 2560 - 1200) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp->channel) % 2560 - 1200) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp->channel) % 2560 - 1200) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp->channel) % 2560 < 1200) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp->channel) % 2560 - 800) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } 
        } else if ((int(tp->channel) / 2560) % 2 == 1) {
            if (x_sign < 0) {
                if (int(tp->channel) % 2560 < 1200) {
                    if (z > (int(tp->channel) % 2560 - 800) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp->channel) % 2560 - 800) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp->channel) % 2560 - 800) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp->channel) % 2560 > 1199) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp->channel) % 2560 - 1200) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp->channel) % 2560 > 1199) {
                    if (z < (1599 - int(tp->channel) % 2560) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp->channel) % 2560 - 1200) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (1599 - int(tp->channel) % 2560) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp->channel) % 2560 < 1200) {
                    ordinate = (z + (int(tp->channel) % 2560 - 800) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            }
        }
        if ((int(tp->channel) / 2560) % 2 < 1) {
            ordinate = (ordinate) - apa_height_in_cm;
        } else {
            ordinate = apa_height_in_cm - (ordinate);
        }

        Y_pred.push_back(ordinate);
    }
    // mean
    float Y_pred_mean = 0;
    for (auto& y : Y_pred) {
        Y_pred_mean += y;
    }
    Y_pred_mean = Y_pred_mean / Y_pred.size();

    return Y_pred_mean;
}
