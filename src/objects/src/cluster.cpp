#include <iostream>
#include <vector>

#include "cluster.h"

#include <Logger.h>

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

// class cluster  

cluster::cluster(std::vector<TriggerPrimitive*> tps) { 
    // check that all tps come from same event
    // check that all tps come from same view

    if (tps.size() == 0) {
        LogError << "Cluster has no TPs!" << std::endl;
        return;
    }

    for (auto& tp : tps) {
        if (tp == nullptr) {
            LogError << "Cluster has null TP!" << std::endl;
            return;
        }
        
        if (tp->GetEvent() != tps.at(0)->GetEvent()) {
            LogError << "Cluster has TPs from different events!" << std::endl;
            return;
        }

        if (tp->GetView() != tps.at(0)->GetView()) {
            LogError << "Cluster has TPs from different views!" << std::endl;
            return;
        }
    }

    // If all the above was ok, we're good to go
    tps_ = tps;

    update_cluster_info();
}

// this is in ADC count times ticks
float cluster::get_total_charge() {
    float total_charge = 0;
    for (auto& tp : tps_) {
        total_charge += tp->GetAdcIntegral();
    }
    return total_charge;
}

float cluster::get_total_energy() {
    return get_total_charge() / adc_to_energy_conversion_factor;
}


bool cluster::isCleanCluster (){
    if (tps_.size() == 0) return false;
    // check if all tps are from the same event
    int event = tps_[0]->GetEvent();
    for (int i = 0; i < tps_.size(); i++) {
        if (tps_.at(i)->GetEvent() != event) {
            return false;
        }
    }
    // check if all tps are from the same view
    std::string view = tps_[0]->GetView();
    for (int i = 0; i < tps_.size(); i++) {
        if (tps_.at(i)->GetView() != view) {
            return false;
        }
    }
    // check if all tps are from the same generator
    std::string generator = tps_[0]->GetGeneratorName();
    // LogInfo << "Generator of first TP: " << generator << std::endl;
    for (int i = 0; i < tps_.size(); i++) {
        if (tps_.at(i)->GetGeneratorName() != generator) {
            // LogInfo << "Generator of TP " << i << ": " << tps_.at(i)->GetGeneratorName() << std::endl;
            return false;
        }
    }

    // LogInfo << "Cluster is clean and all TPs are from the same generator: " << generator << std::endl;
    return true;
}

void cluster::update_cluster_info() {

    if (tps_.size() == 0) return; 

    // if all tps are from the same TrueParticle, we can just set the true label straight away, skip  some computing
    if (isCleanCluster()) {
        // LogInfo << "All TPs are from the same TrueParticle, and generator is " << tps_[0]->GetGeneratorName() << std::endl;
        true_label_ = tps_[0]->GetGeneratorName();
        // LogInfo << "True label, meaning generator, of this cluster: " << true_label_ << std::endl;
        if (true_label_ == "marley"){
            // LogInfo << "This is a pure marley cluster" << std::endl;
            if (tps_[0]->GetTrueParticle() != nullptr) {
                if (tps_[0]->GetTrueParticle()->GetNeutrino() != nullptr) {
                    // LogInfo << "True particle is not null" << std::endl;
                    true_interaction_ = tps_[0]->GetTrueParticle()->GetNeutrino()->GetInteraction(); // check if ok
                    true_neutrino_energy_ = tps_[0]->GetTrueParticle()->GetNeutrino()->GetEnergy();
                    true_pos_ = tps_[0]->GetTrueParticle()->GetNeutrino()->GetPosition();
                    true_dir_ = tps_[0]->GetTrueParticle()->GetNeutrino()->GetMomentum();
                }
            }
            supernova_tp_fraction_ = 1;
        }
    }
    else {
        // find the most common label
        // LogInfo << "Not all TPs are from the same TrueParticle, finding the most frequent one" << std::endl;
        std::map<std::string, int> label_count;
        for (int i = 0; i < tps_.size(); i++) {
            std::string label = tps_.at(i)->GetGeneratorName();
            if (label_count.find(label) == label_count.end()) {
                label_count[label] = 1;
            }
            else {
                label_count[label]++;
            }
        }
        // find the label with the most counts
        int max_count = 0;
        std::string first_label, second_label;
        int first_count = 0, second_count = 0;

        // Find top two labels by count
        for (const auto& x : label_count) {
            if (x.second > first_count) {
            second_count = first_count;
            second_label = first_label;
            first_count = x.second;
            first_label = x.first;
            } else if (x.second > second_count) {
            second_count = x.second;
            second_label = x.first;
            }
        }

        true_label_ = first_label;
        // If the most common label is "UNKNOWN" and there is a second, use the second
        if (true_label_ == "UNKNOWN" && !second_label.empty()) {
            true_label_ = second_label;
        }

        // LogInfo << "Most common label: " << true_label_ << std::endl;

        // if there is at least one marley, set true information
        if (label_count.find("marley") != label_count.end()) {
            // LogInfo << "Found at least one marley TP" << std::endl;
            if (tps_[0]->GetTrueParticle() != nullptr) {
                if (tps_[0]->GetTrueParticle()->GetNeutrino() != nullptr) {
                    // LogInfo << "True particle is not null" << std::endl;
                    true_interaction_ = tps_[0]->GetTrueParticle()->GetNeutrino()->GetInteraction(); // check if ok
                    true_neutrino_energy_ = tps_[0]->GetTrueParticle()->GetNeutrino()->GetEnergy();
                    true_particle_energy_ = tps_[0]->GetTrueParticle()->GetEnergy();
                    true_pos_ = tps_[0]->GetTrueParticle()->GetNeutrino()->GetPosition();
                    true_dir_ = tps_[0]->GetTrueParticle()->GetNeutrino()->GetMomentum();
                }
            }
            supernova_tp_fraction_ = 0;
            // TODO could do better than this
            for (int i = 0; i < tps_.size(); i++) {
                if (tps_.at(i)->GetGeneratorName() == "marley") {
                    supernova_tp_fraction_++;
                }
            }
            supernova_tp_fraction_ /= float (tps_.size());
        }
        
        // Calculate fraction of TPs with any generator name (not "UNKNOWN")
        generator_tp_fraction_ = 0;
        for (int i = 0; i < tps_.size(); i++) {
            if (tps_.at(i)->GetGeneratorName() != "UNKNOWN" && !tps_.at(i)->GetGeneratorName().empty()) {
                generator_tp_fraction_++;
            }
        }
        generator_tp_fraction_ /= float (tps_.size());
    }

    // the reconstructed position will be the average of the tps
    // we want to save the minimum distance from the true position
    float min_distance = INT_MAX;
    std::vector<float> pos = calculate_position(tps_[0]); // using first TP
    // reco_pos_ = pos;
    min_distance = sqrt(pow(pos[0] - true_pos_[0], 2) + pow(pos[2] - true_pos_[2], 2));

    // float total_charge = 0;
    std::vector<float> reco_pos = {0, 0, 0};
    for (int i = 0; i < tps_.size(); i++) {
        // total_charge += tps_.at(i)[tp.adc_integral];
        std::vector<float> pos = calculate_position(tps_.at(i));

        if (tps_.at(i)->GetTrueParticle() != nullptr) {
            std::vector<float> true_pos = tps_.at(i)->GetTrueParticle()->GetPosition();
            float distance = vectorDistance(pos, true_pos);
            if (distance < min_distance ) {
                if (true_pos != std::vector<float>{0, 0, 0}) {
                    min_distance = distance;
                    // true_pos_ = true_pos;
                }
            }
        }

        reco_pos[0] += pos[0];
        reco_pos[1] += pos[1];
        reco_pos[2] += pos[2];
    }
    reco_pos[0] /= get_number_of_tps();
    reco_pos[1] /= get_number_of_tps();
    reco_pos[2] /= get_number_of_tps();
    reco_pos_ = reco_pos;

    min_distance_from_true_pos_ = min_distance;
    // total_charge_ = total_charge;
}

////////////////////////////////////////////////////////////////////////
// Other functions

std::vector<float> calculate_position(TriggerPrimitive* tp) { // only works for plane X

    // from the channel  we can know which side of the APA we are
    float x_signs = (int(tp->GetDetectorChannel()) % APA::total_channels < (APA::induction_channels * 2 + APA::collection_channels)) ? -1.0f : 1.0f;
    float x = ((int(tp->GetTimeStart()) ) * time_tick_in_cm + apa_width_in_cm/2) * x_signs; 

    float y = 0; // need to combine views to compute

    float z = 0; // can't easily compute for induction at this stage
    if (tp->GetView() == "X") {
        // the 2 here is because the first 2 APAs are one on top of the other, so they have the same z
        float z_apa_offset = int( tp->GetDetector() / 2 ) * (apa_lenght_in_cm + offset_between_apa_in_cm);
        // half of collection wires on each side
        float z_channel_offset = ((int(tp->GetDetectorChannel()) - APA::induction_channels*2) % APA::collection_channels/2) * wire_pitch_in_cm_collection;
        z = wire_pitch_in_cm_collection + z_apa_offset + z_channel_offset;
    } 

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


// TODO remove?
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
    z = z - int(tps.at(0).GetDetectorChannel()) / (APA::total_channels*2) * (apa_lenght_in_cm + offset_between_apa_in_cm); // not sure about the 0 TODO
    float ordinate;
    std::vector<float> Y_pred;
    for (auto& tp : tps) {
        if ((int(tp.GetDetectorChannel()) / APA::total_channels) % 2 == 0) {
            if (x_sign < 0) {
                if (int(tp.GetDetectorChannel()) % APA::total_channels < 400) {
                    if (z > (int(tp.GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp.GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp.GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp.GetDetectorChannel()) % APA::total_channels > 399) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp.GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp.GetDetectorChannel()) % APA::total_channels > 399) {
                    if (z < (799 - int(tp.GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp.GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (799 - int(tp.GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp.GetDetectorChannel()) % APA::total_channels < 400) {
                    ordinate = (z + (int(tp.GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            }
        } else if ((int(tp.GetDetectorChannel()) / APA::total_channels) % 2 == 1) {
            if (x_sign < 0) {
                if (int(tp.GetDetectorChannel()) % APA::total_channels < 400) {
                    if (z < (399 - int(tp.GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp.GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (399 - int(tp.GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp.GetDetectorChannel()) % APA::total_channels > 399) {
                    ordinate = (z + (int(tp.GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp.GetDetectorChannel()) % APA::total_channels > 399) {
                    if (z > (int(tp.GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp.GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp.GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp.GetDetectorChannel()) % APA::total_channels < 400) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp.GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            }
        }
        // ordinate = (ordinate) - apa_height_in_cm if (tp[idx['channel']] / APA::total_channels) % 2 < 1 else apa_height_in_cm - (ordinate);
        if ((int(tp.GetDetectorChannel()) / APA::total_channels) % 2 < 1) {
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
};