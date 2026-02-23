#include <vector>
#include <iostream>
#include <algorithm>
#include "MatchClusters.h"
#include "Cluster.h"
#include "Geometry.h"
#include "Utils.h"
// #include "PositionCalculator.h"

// Helper function to calculate average Z position from X plane TPs
// Based on calculate_position() logic from Backtracking.cpp
float calculate_z_from_x_plane(std::vector<TriggerPrimitive*> tps) {
    if (tps.empty()) return 0.0;
    
    float z_sum = 0.0;
    int count = 0;
    
    for (auto* tp : tps) {
        int channel = tp->GetDetectorChannel();
        int local_channel = channel % APA::total_channels;
        
        // X plane channels are at 1600+ in local coordinates
        if (local_channel >= APA::induction_channels * 2) {
            // Z calculation from calculate_position():
            // z_apa_offset: which APA pair (detector/2) * (APA length + gap)
            float z_apa_offset = int(tp->GetDetector() / 2) * (apa_lenght_in_cm + offset_between_apa_in_cm);
            
            // z_channel_offset: position within half of collection plane
            // ((channel - 1600) % (collection_channels/2)) * wire_pitch
            int channel_offset_in_collection = (local_channel - APA::induction_channels * 2) % (APA::collection_channels / 2);
            float z_channel_offset = channel_offset_in_collection * wire_pitch_in_cm_collection;
            
            float z = wire_pitch_in_cm_collection + z_apa_offset + z_channel_offset;
            z_sum += z;
            count++;
        }
    }
    
    return (count > 0) ? z_sum / count : 0.0;
}

// Helper function to determine X sign from X plane TPs
// Based on calculate_position() logic: 
// x_signs = (channel % total_channels < (induction*2 + collection)) ? -1 : +1
float calculate_x_sign_from_x_plane(std::vector<TriggerPrimitive*> tps) {
    if (tps.empty()) return 1.0;
    
    int channel = tps[0]->GetDetectorChannel();
    int local_channel = channel % APA::total_channels;
    
    // From calculate_position():
    // x_signs = (channel % APA::total_channels < (induction*2 + collection)) ? -1 : +1
    int full_range = APA::induction_channels * 2 + APA::collection_channels;
    float x_sign = (local_channel < full_range) ? -1.0f : 1.0f;
    
    return x_sign;
}

bool are_compatibles(Cluster& c_u, Cluster& c_v, Cluster& c_x, float radius) {
    // Check if they come from the same detector
    if (!(int(c_u.get_tp(0)->GetDetector()) == int(c_v.get_tp(0)->GetDetector()) && 
            int(c_u.get_tp(0)->GetDetector()) == int(c_x.get_tp(0)->GetDetector())))
        return false;

    // Simplified matching: just check time+event+APA compatibility
    // Wire crossing geometry check disabled for now to see if it's the bottleneck
    // Time overlap is already checked in the outer loop (match_clusters.cpp)
    
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

// Overload for joining 2 clusters (partial matches)
Cluster join_clusters(Cluster c1, Cluster c2){
    std::vector<TriggerPrimitive*> tps;
    
    // Determine which is X plane (collection) for truth info
    bool c1_is_x = (c1.get_size() > 0 && c1.get_tps()[0]->GetView() == "X");
    bool c2_is_x = (c2.get_size() > 0 && c2.get_tps()[0]->GetView() == "X");
    
    Cluster& x_cluster = c1_is_x ? c1 : c2;
    int common_event = (x_cluster.get_size() > 0) ? x_cluster.get_tps()[0]->GetEvent() : 0;
    
    // Add all TPs from both clusters
    for (int i = 0; i < c1.get_size(); i++) {
        TriggerPrimitive* tp = c1.get_tps()[i];
        tp->SetEvent(common_event);
        tps.push_back(tp);
    }
    for (int i = 0; i < c2.get_size(); i++) {
        TriggerPrimitive* tp = c2.get_tps()[i];
        tp->SetEvent(common_event);
        tps.push_back(tp);
    }

    Cluster c(tps);
    // Copy truth info from X cluster
    c.set_true_pos(x_cluster.get_true_pos());
    c.set_true_dir(x_cluster.get_true_dir());
    c.set_true_momentum(x_cluster.get_true_momentum());
    c.set_true_energy(x_cluster.get_true_neutrino_energy());
    c.set_true_label(x_cluster.get_true_label());
    c.set_is_es_interaction(x_cluster.get_is_es_interaction());
    c.set_min_distance_from_true_pos(x_cluster.get_min_distance_from_true_pos());
    c.set_supernova_tp_fraction(x_cluster.get_supernova_tp_fraction());
    
    return c;
}

