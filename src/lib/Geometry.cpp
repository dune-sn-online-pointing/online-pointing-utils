#include "Geometry.h"
#include "Global.h"
#include "Utils.h"

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
