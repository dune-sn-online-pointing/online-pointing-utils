/**
 * @file analyze_matching.cpp
 * @brief Analyze cluster matching results and compute metrics
 * 
 * This app reads matched cluster files and computes:
 * - Matching efficiency (% of X clusters that find U+V matches)
 * - Match multiplicity (average matches per X cluster)
 * - Purity (true vs false matches using truth information)
 * - Time/spatial distributions of matched vs unmatched clusters
 */

#include "Clustering.h"
#include "ParametersManager.h"
#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <nlohmann/json.hpp>

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

struct MatchingMetrics {
    // Efficiency metrics
    int n_x_clusters = 0;
    int n_u_clusters = 0;
    int n_v_clusters = 0;
    int n_multiplane_clusters = 0;
    int n_matched_x = 0;  // X clusters that participated in matches
    int n_matched_u = 0;  // U clusters that participated in matches
    int n_matched_v = 0;  // V clusters that participated in matches
    
    // Multiplicity
    std::map<int, int> matches_per_x;  // cluster_id -> count
    std::map<int, int> matches_per_u;
    std::map<int, int> matches_per_v;
    
    // Truth-based purity (for MARLEY events)
    int n_marley_multiplane = 0;
    int n_pure_marley_matches = 0;  // All 3 planes are marley
    int n_partial_marley_matches = 0;  // At least 1 marley, not all
    
    // Spatial/temporal distributions
    std::vector<double> matched_x_times;
    std::vector<double> unmatched_x_times;
    std::vector<double> matched_x_channels;
    std::vector<double> unmatched_x_channels;
    
    void print() const {
        std::cout << "\n=========================================" << std::endl;
        std::cout << "Cluster Matching Analysis Results" << std::endl;
        std::cout << "=========================================" << std::endl;
        
        std::cout << "\nInput Cluster Counts:" << std::endl;
        std::cout << "  U-plane clusters: " << n_u_clusters << std::endl;
        std::cout << "  V-plane clusters: " << n_v_clusters << std::endl;
        std::cout << "  X-plane clusters: " << n_x_clusters << std::endl;
        std::cout << "  Multiplane matches: " << n_multiplane_clusters << std::endl;
        
        std::cout << "\nMatching Efficiency:" << std::endl;
        double eff_x = n_x_clusters > 0 ? 100.0 * n_matched_x / n_x_clusters : 0.0;
        double eff_u = n_u_clusters > 0 ? 100.0 * n_matched_u / n_u_clusters : 0.0;
        double eff_v = n_v_clusters > 0 ? 100.0 * n_matched_v / n_v_clusters : 0.0;
        std::cout << "  X-plane efficiency: " << eff_x << "% (" << n_matched_x << "/" << n_x_clusters << ")" << std::endl;
        std::cout << "  U-plane efficiency: " << eff_u << "% (" << n_matched_u << "/" << n_u_clusters << ")" << std::endl;
        std::cout << "  V-plane efficiency: " << eff_v << "% (" << n_matched_v << "/" << n_v_clusters << ")" << std::endl;
        
        std::cout << "\nMatch Multiplicity:" << std::endl;
        
        // Calculate average matches per cluster
        if (!matches_per_x.empty()) {
            double total_matches = 0;
            int max_matches = 0;
            for (const auto& [id, count] : matches_per_x) {
                total_matches += count;
                if (count > max_matches) max_matches = count;
            }
            double avg_x = total_matches / matches_per_x.size();
            std::cout << "  Average matches per X cluster: " << avg_x << std::endl;
            std::cout << "  Max matches for single X cluster: " << max_matches << std::endl;
        }
        
        if (!matches_per_u.empty()) {
            double total_matches = 0;
            for (const auto& [id, count] : matches_per_u) {
                total_matches += count;
            }
            double avg_u = total_matches / matches_per_u.size();
            std::cout << "  Average matches per U cluster: " << avg_u << std::endl;
        }
        
        if (!matches_per_v.empty()) {
            double total_matches = 0;
            for (const auto& [id, count] : matches_per_v) {
                total_matches += count;
            }
            double avg_v = total_matches / matches_per_v.size();
            std::cout << "  Average matches per V cluster: " << avg_v << std::endl;
        }
        
        std::cout << "\nTruth-Based Purity (MARLEY events):" << std::endl;
        if (n_marley_multiplane > 0) {
            double purity = 100.0 * n_pure_marley_matches / n_marley_multiplane;
            std::cout << "  MARLEY multiplane clusters: " << n_marley_multiplane << std::endl;
            std::cout << "  Pure MARLEY matches (all 3 planes): " << n_pure_marley_matches << std::endl;
            std::cout << "  Partial MARLEY matches: " << n_partial_marley_matches << std::endl;
            std::cout << "  Purity: " << purity << "%" << std::endl;
        } else {
            std::cout << "  No MARLEY multiplane clusters found" << std::endl;
        }
        
        std::cout << "\n=========================================" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " -j <json_config>" << std::endl;
        std::cout << "\nJSON format:" << std::endl;
        std::cout << "{" << std::endl;
        std::cout << "    \"matched_clusters_file\": \"path/to/matched_clusters.root\"" << std::endl;
        std::cout << "}" << std::endl;
        return 1;
    }
    
    // Parse command line arguments
    std::string json_file;
    int max_files = -1;  // -1 means process all files
    int skip_files = 0;  // 0 means start from the beginning
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-j" || arg == "--json") {
            if (i + 1 < argc) {
                json_file = argv[i + 1];
                i++;
            }
        } else if (arg == "-m" || arg == "--max-files") {
            if (i + 1 < argc) {
                max_files = std::stoi(argv[i + 1]);
                i++;
            }
        } else if (arg == "-s" || arg == "--skip-files") {
            if (i + 1 < argc) {
                skip_files = std::stoi(argv[i + 1]);
                i++;
            }
        }
    }
    
    if (json_file.empty()) {
        LogError << "No JSON config file provided" << std::endl;
        return 1;
    }
    
    // Read JSON configuration
    std::ifstream json_stream(json_file);
    nlohmann::json j;
    json_stream >> j;
    
    std::string matched_file = j.at("matched_clusters_file");
    
    LogInfo << "Analyzing matched clusters from: " << matched_file << std::endl;
    
    // Open matched clusters file
    TFile* file = TFile::Open(matched_file.c_str(), "READ");
    if (!file || file->IsZombie()) {
        LogError << "Failed to open file: " << matched_file << std::endl;
        return 1;
    }
    
    MatchingMetrics metrics;
    
    // Trees in matched files
    TTree* tree_u = (TTree*)file->Get("clusters/clusters_tree_U");
    TTree* tree_v = (TTree*)file->Get("clusters/clusters_tree_V");
    TTree* tree_x = (TTree*)file->Get("clusters/clusters_tree_X");
    TTree* tree_multi = (TTree*)file->Get("clusters/clusters_tree_multiplane");
    
    if (tree_u) metrics.n_u_clusters = tree_u->GetEntries();
    if (tree_v) metrics.n_v_clusters = tree_v->GetEntries();
    if (tree_x) metrics.n_x_clusters = tree_x->GetEntries();
    if (tree_multi) metrics.n_multiplane_clusters = tree_multi->GetEntries();
    
    // Legacy path: explicit multiplane tree
    if (tree_multi) {
        LogInfo << "Using legacy multiplane tree schema" << std::endl;
        LogInfo << "Found " << metrics.n_multiplane_clusters << " matched clusters" << std::endl;
        
        Float_t marley_tp_fraction = 0.f;
        Int_t cluster_id = -1;
        tree_multi->SetBranchAddress("marley_tp_fraction", &marley_tp_fraction);
        tree_multi->SetBranchAddress("cluster_id", &cluster_id);
        
        LogInfo << "Processing " << metrics.n_multiplane_clusters << " matched clusters..." << std::endl;
        
        for (Long64_t i = 0; i < tree_multi->GetEntries(); i++) {
            tree_multi->GetEntry(i);
            
            bool is_marley = marley_tp_fraction > 0.5;
            if (is_marley) {
                metrics.n_marley_multiplane++;
                if (marley_tp_fraction > 0.9) {
                    metrics.n_pure_marley_matches++;
                } else {
                    metrics.n_partial_marley_matches++;
                }
            }
        }
    } else {
        // Current path: match_id stored on per-plane trees
        if (!tree_u || !tree_v || !tree_x) {
            LogError << "No compatible matching schema found. Expected either "
                     << "clusters_tree_multiplane (legacy) or clusters_tree_U/V/X (current)." << std::endl;
            file->Close();
            return 1;
        }

        if (!tree_u->GetBranch("match_id") || !tree_v->GetBranch("match_id") || !tree_x->GetBranch("match_id")) {
            LogError << "Per-plane trees are present but missing match_id branch." << std::endl;
            file->Close();
            return 1;
        }

        LogInfo << "Using current per-plane match_id schema" << std::endl;

        std::map<int, int> match_view_mask; // match_id -> bitmask (U=1, V=2, X=4)

        auto process_plane = [&](TTree* tree, int view_bit, int& n_matched_plane, std::map<int, int>& matches_per_plane,
                                 bool track_purity_from_x = false) {
            Int_t match_id = -1;
            Int_t cluster_id = -1;
            Float_t marley_tp_fraction = 0.f;

            tree->SetBranchAddress("match_id", &match_id);
            tree->SetBranchAddress("cluster_id", &cluster_id);
            if (track_purity_from_x) {
                if (!tree->GetBranch("marley_tp_fraction")) {
                    LogError << "X tree missing marley_tp_fraction branch." << std::endl;
                    return false;
                }
                tree->SetBranchAddress("marley_tp_fraction", &marley_tp_fraction);
            }

            for (Long64_t i = 0; i < tree->GetEntries(); i++) {
                tree->GetEntry(i);
                if (match_id >= 0) {
                    n_matched_plane++;
                    matches_per_plane[cluster_id]++;
                    match_view_mask[match_id] |= view_bit;

                    if (track_purity_from_x) {
                        bool is_marley = marley_tp_fraction > 0.5;
                        if (is_marley) {
                            metrics.n_marley_multiplane++;
                            if (marley_tp_fraction > 0.9) metrics.n_pure_marley_matches++;
                            else metrics.n_partial_marley_matches++;
                        }
                    }
                }
            }
            return true;
        };

        if (!process_plane(tree_u, 1, metrics.n_matched_u, metrics.matches_per_u) ||
            !process_plane(tree_v, 2, metrics.n_matched_v, metrics.matches_per_v) ||
            !process_plane(tree_x, 4, metrics.n_matched_x, metrics.matches_per_x, true)) {
            file->Close();
            return 1;
        }

        // Count only full U+V+X matches as "multiplane"
        for (const auto& [mid, mask] : match_view_mask) {
            if (mask == 7) metrics.n_multiplane_clusters++;
        }

        LogInfo << "Found " << metrics.n_multiplane_clusters << " full U+V+X matches (from match_id linking)" << std::endl;
    }
    
    file->Close();
    
    // Print results
    metrics.print();
    
    LogInfo << "Analysis complete!" << std::endl;
    
    return 0;
}
