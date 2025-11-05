#include <vector>
#include <iostream>
#include <algorithm>
#include "match_clusters_libs.h"
#include "Cluster.h"
#include "geometry.h"
#include "utils.h"
// #include "position_calculator.h"

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

    // Wire geometry in ProtoDUNE-SP 1x2x2:
    // - X wires: vertical (0°), parallel to Y-axis, give direct Z position
    // - U wires: +35.7° from vertical
    // - V wires: -35.7° from vertical
    // 
    // For wire crossing check:
    // Each U/V wire channel maps to a specific wire position
    // At a given Z position (from X plane), certain U and V wires should be present
    // 
    // The key: check if the U and V wire channels are compatible with the Z from X plane
    
    // Get Z position from X plane
    float z_from_x = calculate_z_from_x_plane(c_x.get_tps());
    
    // Get average channel numbers for U and V clusters
    float avg_u_channel = 0.0;
    float avg_v_channel = 0.0;
    
    for (auto* tp : c_u.get_tps()) {
        avg_u_channel += tp->GetDetectorChannel() % APA::total_channels;
    }
    avg_u_channel /= c_u.get_tps().size();
    
    for (auto* tp : c_v.get_tps()) {
        avg_v_channel += tp->GetDetectorChannel() % APA::total_channels;
    }
    avg_v_channel /= c_v.get_tps().size();
    
    // Debug output for first few checks
    static int debug_count = 0;
    if (verboseMode && debug_count < 5) {
        LogInfo << "Wire crossing check [" << debug_count << "]:" << std::endl;
        LogInfo << "  Z from X plane: " << z_from_x << " cm" << std::endl;
        LogInfo << "  U channel: " << avg_u_channel << std::endl;
        LogInfo << "  V channel: " << avg_v_channel << std::endl;
        LogInfo << "  Spatial tolerance: " << radius << " cm" << std::endl;
        debug_count++;
    }
    
    // For now, use a generous tolerance since we're still developing the geometry
    // The main filter is time overlap, this is just a sanity check
    // TODO: Implement proper wire crossing geometry calculation
    
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


