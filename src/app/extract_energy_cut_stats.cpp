/**
 * @file extract_energy_cut_stats.cpp
 * @brief Extract cluster statistics from energy cut scan ROOT files
 * 
 * This program extracts statistics about MARLEY clusters, background clusters,
 * and main track clusters from the ROOT files generated during energy cut scans.
 */

#include "Global.h"

namespace fs = std::filesystem;

struct ClusterStats {
    double energy_cut;
    int marley_clusters_viewX = 0;
    int background_clusters_viewX = 0;
    int main_track_clusters_viewX = 0;
    int total_clusters = 0;
    int total_clusters_viewX = 0;
    bool found = false;
};

ClusterStats analyze_cluster_file(const std::string& filepath) {
    ClusterStats stats;
    
    TFile* file = TFile::Open(filepath.c_str(), "READ");
    if (!file || file->IsZombie()) {
        std::cerr << "Error opening " << filepath << std::endl;
        return stats;
    }
    
    // We only care about view X
    TTree* tree = (TTree*)file->Get("clusters/clusters_tree_X");
    if (!tree) {
        std::cerr << "No clusters_tree_X in " << filepath << std::endl;
        file->Close();
        return stats;
    }
    
    // Set up branch reading
    Float_t marley_tp_fraction = 0;
    Bool_t is_main_cluster = false;
    
    tree->SetBranchAddress("marley_tp_fraction", &marley_tp_fraction);
    tree->SetBranchAddress("is_main_cluster", &is_main_cluster);
    
    stats.total_clusters_viewX = tree->GetEntries();
    
    for (Long64_t i = 0; i < tree->GetEntries(); ++i) {
        tree->GetEntry(i);
        
        // If most TPs in cluster are from MARLEY, consider it a MARLEY cluster
        if (marley_tp_fraction > 0.5) {
            stats.marley_clusters_viewX++;
            if (is_main_cluster) {
                stats.main_track_clusters_viewX++;
            }
        } else {
            stats.background_clusters_viewX++;
        }
    }
    
    file->Close();
    
    return stats;
}

std::vector<ClusterStats> process_sample(const std::string& base_path, 
                                         const std::string& sample_type,
                                         const std::vector<double>& energy_cuts) {
    std::vector<ClusterStats> results;
    
    for (double ecut : energy_cuts) {
        ClusterStats stats;
        stats.energy_cut = ecut;
        
        // Format energy cut for folder name (e.g., 0 -> e0p0, 0.5 -> e0p5, 1.0 -> e1p0)
        int ecut_int = static_cast<int>(ecut);
        int ecut_frac = static_cast<int>((ecut - ecut_int) * 10);
        std::string ecut_str = "e" + std::to_string(ecut_int) + "p" + std::to_string(ecut_frac);
        
        // Construct cluster folder path
        std::string cluster_folder = base_path + "/clusters_" + sample_type + 
                                    "_valid_bg_tick3_ch2_min2_tot2_" + ecut_str;
        
        std::cout << "\nProcessing " << sample_type << " energy_cut=" << ecut << std::endl;
        std::cout << "Looking in: " << cluster_folder << std::endl;
        
        if (!fs::exists(cluster_folder)) {
            std::cout << "  Directory not found, skipping..." << std::endl;
            results.push_back(stats);
            continue;
        }
        
        // Find all cluster ROOT files
        std::vector<std::string> cluster_files;
        for (const auto& entry : fs::directory_iterator(cluster_folder)) {
            if (entry.path().extension() == ".root" && 
                entry.path().filename().string().find("clusters_") == 0) {
                cluster_files.push_back(entry.path().string());
            }
        }
        
        if (cluster_files.empty()) {
            std::cout << "  No cluster files found" << std::endl;
            results.push_back(stats);
            continue;
        }
        
        std::cout << "  Found " << cluster_files.size() << " cluster file(s)" << std::endl;
        
        stats.found = true;
        
        // Process each file and aggregate statistics
        for (const auto& file : cluster_files) {
            ClusterStats file_stats = analyze_cluster_file(file);
            stats.marley_clusters_viewX += file_stats.marley_clusters_viewX;
            stats.background_clusters_viewX += file_stats.background_clusters_viewX;
            stats.main_track_clusters_viewX += file_stats.main_track_clusters_viewX;
            stats.total_clusters += file_stats.total_clusters;
            stats.total_clusters_viewX += file_stats.total_clusters_viewX;
        }
        
        std::cout << "  View X - MARLEY: " << stats.marley_clusters_viewX 
                  << ", Background: " << stats.background_clusters_viewX
                  << ", Main track: " << stats.main_track_clusters_viewX << std::endl;
        
        results.push_back(stats);
    }
    
    return results;
}

void save_results(const std::vector<ClusterStats>& cc_results,
                 const std::vector<ClusterStats>& es_results,
                 const std::string& output_file = "energy_cut_scan_data.txt") {
    std::ofstream out(output_file);
    
    out << "# Energy Cut Scan Results\n";
    out << "# Format: energy_cut marley_viewX background_viewX main_track_viewX total_viewX found\n\n";
    
    out << "# CC Results\n";
    out << "CC_DATA:\n";
    for (const auto& r : cc_results) {
        if (r.found) {
            out << r.energy_cut << " " << r.marley_clusters_viewX << " "
                << r.background_clusters_viewX << " " << r.main_track_clusters_viewX << " "
                << r.total_clusters_viewX << " 1\n";
        } else {
            out << r.energy_cut << " 0 0 0 0 0\n";
        }
    }
    
    out << "\n# ES Results\n";
    out << "ES_DATA:\n";
    for (const auto& r : es_results) {
        if (r.found) {
            out << r.energy_cut << " " << r.marley_clusters_viewX << " "
                << r.background_clusters_viewX << " " << r.main_track_clusters_viewX << " "
                << r.total_clusters_viewX << " 1\n";
        } else {
            out << r.energy_cut << " 0 0 0 0 0\n";
        }
    }
    
    out.close();
    std::cout << "\nResults saved to " << output_file << std::endl;
}

int main(int argc, char** argv) {
    // Energy cuts to analyze
    std::vector<double> energy_cuts = {0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0};
    
    // Base paths
    std::string cc_base = "/home/virgolaema/dune/online-pointing-utils/data/prod_cc";
    std::string es_base = "/home/virgolaema/dune/online-pointing-utils/data/prod_es";
    
    // Process CC samples
    std::cout << "============================================================" << std::endl;
    std::cout << "Processing CC samples" << std::endl;
    std::cout << "============================================================" << std::endl;
    auto cc_results = process_sample(cc_base, "cc", energy_cuts);
    
    // Process ES samples
    std::cout << "\n============================================================" << std::endl;
    std::cout << "Processing ES samples" << std::endl;
    std::cout << "============================================================" << std::endl;
    auto es_results = process_sample(es_base, "es", energy_cuts);
    
    // Save results
    save_results(cc_results, es_results);
    
    std::cout << "\n============================================================" << std::endl;
    std::cout << "Analysis complete!" << std::endl;
    std::cout << "============================================================" << std::endl;
    
    return 0;
}
