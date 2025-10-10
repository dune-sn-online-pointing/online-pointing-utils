#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <vector>
#include <algorithm>

void analyze_cluster(int cluster_idx, int n_tps, const std::vector<int>& times, 
                     const std::vector<int>& channels, const std::string& plane) {
    
    // Sort indices by time
    std::vector<int> indices(n_tps);
    for (int i = 0; i < n_tps; ++i) indices[i] = i;
    std::sort(indices.begin(), indices.end(), [&](int a, int b) { return times[a] < times[b]; });
    
    // Calculate consecutive differences
    std::cout << "\nCluster " << cluster_idx << " (" << plane << "): n_tps=" << n_tps << std::endl;
    std::cout << "  TPs sorted by time:" << std::endl;
    
    for (size_t j = 0; j < indices.size(); ++j) {
        int idx = indices[j];
        int t = times[idx];
        int ch = channels[idx];
        
        if (j > 0) {
            int prev_idx = indices[j-1];
            int dt = t - times[prev_idx];
            int dch = std::abs(ch - channels[prev_idx]);
            std::cout << "    TP" << j << ": t=" << t << ", ch=" << ch 
                      << " | Δt=" << dt << ", Δch=" << dch << std::endl;
        } else {
            std::cout << "    TP" << j << ": t=" << t << ", ch=" << ch << std::endl;
        }
    }
}

int main() {
    TFile* f = TFile::Open("/home/virgolaema/dune/online-pointing-utils/data/es_valid_tick5_ch2_min2_tot1_clusters.root");
    if (!f || f->IsZombie()) {
        std::cerr << "Cannot open file" << std::endl;
        return 1;
    }
    
    TDirectory* clusters_dir = (TDirectory*)f->GetDirectory("clusters");
    if (!clusters_dir) {
        std::cerr << "clusters directory not found" << std::endl;
        return 1;
    }
    
    std::vector<std::string> planes = {"U", "V", "X"};
    
    for (const auto& plane : planes) {
        std::cout << "\n======================================" << std::endl;
        std::cout << "Analyzing " << plane << " plane" << std::endl;
        std::cout << "======================================" << std::endl;
        
        std::string tree_name = "clusters_tree_" + plane;
        TTree* tree = (TTree*)clusters_dir->Get(tree_name.c_str());
        if (!tree) {
            std::cout << "Tree " << tree_name << " not found" << std::endl;
            continue;
        }
        
        std::vector<int>* tp_time_start = nullptr;
        std::vector<int>* tp_detector_channel = nullptr;
        
        tree->SetBranchAddress("tp_time_start", &tp_time_start);
        tree->SetBranchAddress("tp_detector_channel", &tp_detector_channel);
        
        int total_clusters = tree->GetEntries();
        int suspicious_count = 0;
        
        // Check specific clusters mentioned by user
        std::vector<int> check_indices = {19, 23, 25, 52};
        
        for (int cluster_idx : check_indices) {
            if (cluster_idx >= total_clusters) continue;
            
            tree->GetEntry(cluster_idx);
            
            if (!tp_time_start || tp_time_start->size() < 2) continue;
            
            int time_min = *std::min_element(tp_time_start->begin(), tp_time_start->end());
            int time_max = *std::max_element(tp_time_start->begin(), tp_time_start->end());
            int time_span = time_max - time_min;
            
            if (time_span > 20) {
                analyze_cluster(cluster_idx, tp_time_start->size(), *tp_time_start, *tp_detector_channel, plane);
                suspicious_count++;
            }
        }
        
        std::cout << "\nChecked " << check_indices.size() << " specific clusters, found " 
                  << suspicious_count << " with time_span > 20 ticks" << std::endl;
    }
    
    f->Close();
    return 0;
}
