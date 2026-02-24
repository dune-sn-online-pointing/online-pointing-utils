#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <fstream>
#include "Clustering.h"
#include "Cluster.h"
#include "Utils.h"
#include "ParametersManager.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct TimingDiagnostic {
    int x_cluster_id;
    int x_event;
    int x_apa;
    double x_time_min_tdc;
    double x_time_max_tdc;
    double x_truth_time;
    double x_truth_x;
    double x_truth_y;
    double x_truth_z;
    
    int nearest_u_id;
    int nearest_u_event;
    int nearest_u_apa;
    double nearest_u_time_min_tdc;
    double nearest_u_time_max_tdc;
    double nearest_u_truth_time;
    double u_time_diff_tdc;
    bool u_event_match;
    bool u_apa_match;
    
    int nearest_v_id;
    int nearest_v_event;
    int nearest_v_apa;
    double nearest_v_time_min_tdc;
    double nearest_v_time_max_tdc;
    double nearest_v_truth_time;
    double v_time_diff_tdc;
    bool v_event_match;
    bool v_apa_match;
};

std::pair<double, double> getClusterTimeRange(Cluster& cluster) {
    double min_time = std::numeric_limits<double>::max();
    double max_time = std::numeric_limits<double>::lowest();
    
    for (const auto& tp : cluster.get_tps()) {
        double start = tp->GetTimeStart();
        double end = start + tp->GetSamplesOverThreshold();
        min_time = std::min(min_time, start);
        max_time = std::max(max_time, end);
    }
    
    return {min_time, max_time};
}

double getClusterTruthTime(Cluster& cluster) {
    // Truth time is not directly stored, use reconstructed time as proxy
    // or compute from truth position if available
    if (cluster.get_tps().empty()) return -999;
    return cluster.get_tps()[0]->GetTimeStart(); // Using reco time as proxy
}

double computeMinTimeDiff(const std::pair<double,double>& range1,
                          const std::pair<double,double>& range2) {
    // If they overlap, distance is 0
    if (range1.first <= range2.second && range2.first <= range1.second) {
        return 0.0;
    }
    // Otherwise, compute gap between them
    if (range1.second < range2.first) {
        return range2.first - range1.second;
    } else {
        return range1.first - range2.second;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config.json>" << std::endl;
        return 1;
    }

    // Load config
    std::ifstream config_file(argv[1]);
    if (!config_file.is_open()) {
        std::cerr << "Error: Could not open config file: " << argv[1] << std::endl;
        return 1;
    }
    
    json config;
    config_file >> config;
    
    // Load parameters for Cluster construction (needed for ADC to energy conversion)
    ParametersManager::getInstance().loadParameters();
    
    // Use tpstream-based file tracking
    std::vector<std::string> cluster_files = find_input_files_by_tpstream_basenames(config, "clusters", 0, 5);
    
    std::cout << "Processing " << cluster_files.size() << " files..." << std::endl;
    
    std::vector<TimingDiagnostic> diagnostics;
    int total_main_x = 0;
    
    for (size_t file_idx = 0; file_idx < cluster_files.size(); ++file_idx) {
        std::string input_file = cluster_files[file_idx];
        std::cout << "\n=== File " << (file_idx+1) << ": " << input_file << " ===" << std::endl;
        
        // Load clusters
        std::vector<Cluster> clusters_u = read_clusters_from_tree(input_file, "U");
        std::vector<Cluster> clusters_v = read_clusters_from_tree(input_file, "V");
        std::vector<Cluster> clusters_x = read_clusters_from_tree(input_file, "X");
        
        int n_main_x = 0;
        for (const auto& c : clusters_x) {
            if (c.get_is_main_cluster()) n_main_x++;
        }
        total_main_x += n_main_x;
        
        std::cout << "Clusters: U=" << clusters_u.size() 
                  << " V=" << clusters_v.size() 
                  << " X=" << clusters_x.size() 
                  << " (main=" << n_main_x << ")" << std::endl;
        
        // Analyze first few main X clusters
        int analyzed = 0;
        for (auto& x_cluster : clusters_x) {
            if (!x_cluster.get_is_main_cluster()) continue;
            if (analyzed >= 3) break; // Only analyze first 3 main clusters per file
            
            TimingDiagnostic diag;
            diag.x_cluster_id = x_cluster.get_cluster_id();
            diag.x_event = x_cluster.get_tps()[0]->GetEvent();
            diag.x_apa = int(x_cluster.get_tps()[0]->GetDetectorChannel() / APA::total_channels);
            
            auto x_time_range = getClusterTimeRange(x_cluster);
            diag.x_time_min_tdc = x_time_range.first;
            diag.x_time_max_tdc = x_time_range.second;
            diag.x_truth_time = getClusterTruthTime(x_cluster);
            
            // Get truth position
            auto true_pos = x_cluster.get_true_pos();
            if (true_pos.size() >= 3) {
                diag.x_truth_x = true_pos[0];
                diag.x_truth_y = true_pos[1];
                diag.x_truth_z = true_pos[2];
            }
            
            // Find nearest U cluster
            double min_u_diff = std::numeric_limits<double>::max();
            Cluster* nearest_u = nullptr;
            for (auto& u_cluster : clusters_u) {
                auto u_time_range = getClusterTimeRange(u_cluster);
                double diff = computeMinTimeDiff(x_time_range, u_time_range);
                if (diff < min_u_diff) {
                    min_u_diff = diff;
                    nearest_u = &u_cluster;
                }
            }
            
            if (nearest_u) {
                diag.nearest_u_id = nearest_u->get_cluster_id();
                diag.nearest_u_event = nearest_u->get_tps()[0]->GetEvent();
                diag.nearest_u_apa = int(nearest_u->get_tps()[0]->GetDetectorChannel() / APA::total_channels);
                auto u_range = getClusterTimeRange(*nearest_u);
                diag.nearest_u_time_min_tdc = u_range.first;
                diag.nearest_u_time_max_tdc = u_range.second;
                diag.nearest_u_truth_time = getClusterTruthTime(*nearest_u);
                diag.u_time_diff_tdc = min_u_diff;
                diag.u_event_match = (diag.x_event == diag.nearest_u_event);
                diag.u_apa_match = (diag.x_apa == diag.nearest_u_apa);
            } else {
                diag.nearest_u_id = -1;
                diag.u_time_diff_tdc = -999;
            }
            
            // Find nearest V cluster
            double min_v_diff = std::numeric_limits<double>::max();
            Cluster* nearest_v = nullptr;
            for (auto& v_cluster : clusters_v) {
                auto v_time_range = getClusterTimeRange(v_cluster);
                double diff = computeMinTimeDiff(x_time_range, v_time_range);
                if (diff < min_v_diff) {
                    min_v_diff = diff;
                    nearest_v = &v_cluster;
                }
            }
            
            if (nearest_v) {
                diag.nearest_v_id = nearest_v->get_cluster_id();
                diag.nearest_v_event = nearest_v->get_tps()[0]->GetEvent();
                diag.nearest_v_apa = int(nearest_v->get_tps()[0]->GetDetectorChannel() / APA::total_channels);
                auto v_range = getClusterTimeRange(*nearest_v);
                diag.nearest_v_time_min_tdc = v_range.first;
                diag.nearest_v_time_max_tdc = v_range.second;
                diag.nearest_v_truth_time = getClusterTruthTime(*nearest_v);
                diag.v_time_diff_tdc = min_v_diff;
                diag.v_event_match = (diag.x_event == diag.nearest_v_event);
                diag.v_apa_match = (diag.x_apa == diag.nearest_v_apa);
            } else {
                diag.nearest_v_id = -1;
                diag.v_time_diff_tdc = -999;
            }
            
            diagnostics.push_back(diag);
            analyzed++;
            
            // Print detailed info
            std::cout << "\n--- Main X Cluster " << diag.x_cluster_id << " ---" << std::endl;
            std::cout << "Event: " << diag.x_event << ", APA: " << diag.x_apa << std::endl;
            std::cout << "Time range [TDC]: [" << diag.x_time_min_tdc << ", " << diag.x_time_max_tdc << "]" << std::endl;
            std::cout << "Truth time [TDC]: " << diag.x_truth_time << std::endl;
            std::cout << "Truth pos: (" << diag.x_truth_x << ", " << diag.x_truth_y << ", " << diag.x_truth_z << ")" << std::endl;
            
            std::cout << "\nNearest U cluster: " << diag.nearest_u_id << std::endl;
            if (nearest_u) {
                std::cout << "  Event: " << diag.nearest_u_event << " (match: " << (diag.u_event_match ? "YES" : "NO") << ")" << std::endl;
                std::cout << "  APA: " << diag.nearest_u_apa << " (match: " << (diag.u_apa_match ? "YES" : "NO") << ")" << std::endl;
                std::cout << "  Time range [TDC]: [" << diag.nearest_u_time_min_tdc << ", " << diag.nearest_u_time_max_tdc << "]" << std::endl;
                std::cout << "  Truth time [TDC]: " << diag.nearest_u_truth_time << std::endl;
                std::cout << "  Time diff [TDC]: " << diag.u_time_diff_tdc << std::endl;
                std::cout << "  Truth time diff [TDC]: " << std::abs(diag.x_truth_time - diag.nearest_u_truth_time) << std::endl;
            }
            
            std::cout << "\nNearest V cluster: " << diag.nearest_v_id << std::endl;
            if (nearest_v) {
                std::cout << "  Event: " << diag.nearest_v_event << " (match: " << (diag.v_event_match ? "YES" : "NO") << ")" << std::endl;
                std::cout << "  APA: " << diag.nearest_v_apa << " (match: " << (diag.v_apa_match ? "YES" : "NO") << ")" << std::endl;
                std::cout << "  Time range [TDC]: [" << diag.nearest_v_time_min_tdc << ", " << diag.nearest_v_time_max_tdc << "]" << std::endl;
                std::cout << "  Truth time [TDC]: " << diag.nearest_v_truth_time << std::endl;
                std::cout << "  Time diff [TDC]: " << diag.v_time_diff_tdc << std::endl;
                std::cout << "  Truth time diff [TDC]: " << std::abs(diag.x_truth_time - diag.nearest_v_truth_time) << std::endl;
            }
        }
    }
    
    std::cout << "\n\n=== SUMMARY ===" << std::endl;
    std::cout << "Total main X clusters analyzed: " << diagnostics.size() << std::endl;
    
    // Count how many have event/APA mismatches
    int u_event_mismatch = 0, v_event_mismatch = 0;
    int u_apa_mismatch = 0, v_apa_mismatch = 0;
    int u_within_1000 = 0, v_within_1000 = 0;
    
    for (const auto& d : diagnostics) {
        if (!d.u_event_match) u_event_mismatch++;
        if (!d.v_event_match) v_event_mismatch++;
        if (!d.u_apa_match) u_apa_mismatch++;
        if (!d.v_apa_match) v_apa_mismatch++;
        if (d.u_time_diff_tdc <= 1000) u_within_1000++;
        if (d.v_time_diff_tdc <= 1000) v_within_1000++;
    }
    
    std::cout << "\nNearest U cluster analysis:" << std::endl;
    std::cout << "  Event mismatches: " << u_event_mismatch << "/" << diagnostics.size() << std::endl;
    std::cout << "  APA mismatches: " << u_apa_mismatch << "/" << diagnostics.size() << std::endl;
    std::cout << "  Within 1000 TDC ticks: " << u_within_1000 << "/" << diagnostics.size() << std::endl;
    
    std::cout << "\nNearest V cluster analysis:" << std::endl;
    std::cout << "  Event mismatches: " << v_event_mismatch << "/" << diagnostics.size() << std::endl;
    std::cout << "  APA mismatches: " << v_apa_mismatch << "/" << diagnostics.size() << std::endl;
    std::cout << "  Within 1000 TDC ticks: " << v_within_1000 << "/" << diagnostics.size() << std::endl;
    
    return 0;
}
