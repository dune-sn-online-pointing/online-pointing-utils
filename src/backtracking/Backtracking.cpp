#include "Backtracking.h"
#include "TriggerPrimitive.hpp"
#include "root.h"
#include <cmath>

std::vector<float> calculate_position(TriggerPrimitive* tp) {
    // ...existing code from cluster.cpp...
    float x_signs = (int(tp->GetDetectorChannel()) % APA::total_channels < (APA::induction_channels * 2 + APA::collection_channels)) ? -1.0f : 1.0f;
    float x = ((int(tp->GetTimeStart()) ) * time_tick_in_cm + apa_width_in_cm/2) * x_signs; 
    float y = 0;
    float z = 0;
    if (tp->GetView() == "X") {
        float z_apa_offset = int( tp->GetDetector() / 2 ) * (apa_lenght_in_cm + offset_between_apa_in_cm);
        float z_channel_offset = ((int(tp->GetDetectorChannel()) - APA::induction_channels*2) % APA::collection_channels/2) * wire_pitch_in_cm_collection;
        z = wire_pitch_in_cm_collection + z_apa_offset + z_channel_offset;
    }
    return {x, y, z};
}

std::vector<std::vector<float>> validate_position_calculation(std::vector<TriggerPrimitive*> tps) {
    std::vector<std::vector<float>> positions;
    return positions;
}

float eval_y_knowing_z_U_plane(std::vector<TriggerPrimitive*> tps, float z, float x_sign) {
    z = z - int(tps.at(0)->GetDetectorChannel()) / (APA::total_channels*2) * (apa_lenght_in_cm + offset_between_apa_in_cm); // not sure about the 0 TODO
    float ordinate;
    std::vector<float> Y_pred;
    for (auto& tp : tps) {
        if ((int(tp->GetDetectorChannel()) / APA::total_channels) % 2 == 0) {
            if (x_sign < 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels < 400) {
                    if (z > (int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels > 399) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp->GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels > 399) {
                    if (z < (799 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (799 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels < 400) {
                    ordinate = (z + (int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            }
        } else if ((int(tp->GetDetectorChannel()) / APA::total_channels) % 2 == 1) {
            if (x_sign < 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels < 400) {
                    if (z < (399 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (399 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels > 399) {
                    ordinate = (z + (int(tp->GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels > 399) {
                    if (z > (int(tp->GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp->GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels < 400) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            }
        }
        // ordinate = (ordinate) - apa_height_in_cm if (tp[idx['channel']] / APA::total_channels) % 2 < 1 else apa_height_in_cm - (ordinate);
        if ((int(tp->GetDetectorChannel()) / APA::total_channels) % 2 < 1) {
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
    
    z = z - int(tps.at(0)->GetDetectorChannel()) / (APA::total_channels*2) * (apa_lenght_in_cm + offset_between_apa_in_cm);
    float ordinate;
    std::vector<float> Y_pred;
    for (auto& tp : tps) {
        if ((int(tp->GetDetectorChannel()) / APA::total_channels) % 2 == 0) {
            if (x_sign < 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels < 1200) {
                    if (z < (1199 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels - 800) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (1199 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels > 1199) {
                    ordinate = (z + (int(tp->GetDetectorChannel()) % APA::total_channels - 1200) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels > 1199) {
                    if (z > (int(tp->GetDetectorChannel()) % APA::total_channels - 1200) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels - 1200) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp->GetDetectorChannel()) % APA::total_channels - 1200) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels < 1200) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp->GetDetectorChannel()) % APA::total_channels - 800) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } 
        } else if ((int(tp->GetDetectorChannel()) / APA::total_channels) % 2 == 1) {
            if (x_sign < 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels < 1200) {
                    if (z > (int(tp->GetDetectorChannel()) % APA::total_channels - 800) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels - 800) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp->GetDetectorChannel()) % APA::total_channels - 800) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels > 1199) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp->GetDetectorChannel()) % APA::total_channels - 1200) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels > 1199) {
                    if (z < (1599 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels - 1200) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (1599 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels < 1200) {
                    ordinate = (z + (int(tp->GetDetectorChannel()) % APA::total_channels - 800) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            }
        }
        if ((int(tp->GetDetectorChannel()) / APA::total_channels) % 2 < 1) {
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

float vectorDistance(std::vector<float> a, std::vector<float> b){
    return sqrt(pow(a[0] - b[0], 2) + pow(a[1] - b[1], 2) + pow(a[2] - b[2], 2));
}
