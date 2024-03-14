#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <climits>

// include root libraries
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"
#include "TMatrixD.h"

#include "cluster.h"
#include "cluster_to_root_libs.h"
#include "position_calculator.h"
#include "superimpose_root_files_libs.h"



cluster filter_clusters_within_radius(std::vector<cluster>& clusters, float radius) {
    std::vector<cluster> filtered_clusters;
    int idx_best = -1;
    int idx = 0;
    int event_number = -1;
    int n_offsets = 0;
    for (auto g : clusters) {
        if (g.get_supernova_tp_fraction() > 0.) {
            if (idx_best == -1) {
                idx_best = idx;
                event_number = g.get_tp(0)[variables_to_index["event"]];
                n_offsets = g.get_tp(0)[variables_to_index["time_start"]] / EVENTS_OFFSET;
            } 
            else {
                if (g.get_min_distance_from_true_pos() < clusters[idx_best].get_min_distance_from_true_pos()) {
                    idx_best = idx;
                }
            }
        }
    idx++;
    }

    if (idx_best == -1) {
        std::cout << "No supernova true positive found" << std::endl;
        // stop the program
        exit(1);
    }

    for (auto const& g : clusters) {
        if (distance(g, clusters[idx_best]) < radius) {
            filtered_clusters.push_back(g);
        }
    }

    cluster final_collective_cluster;
    std::vector<std::vector<double>> tps; // using double in cluster, need to keep consistency
    std::vector<std::vector<double>> tps_all; // using double in cluster, need to keep consistency
    for (auto const& g : filtered_clusters) {
        tps = g.get_tps();
        // fix the different offsets coming from different events
        for (int i = 0; i < tps.size(); i++) {
            tps[i][variables_to_index["time_start"]] =  (int) tps[i][variables_to_index["time_start"]]%(EVENTS_OFFSET) + n_offsets*EVENTS_OFFSET;
            tps[i][variables_to_index["time_peak"]] = (int) tps[i][variables_to_index["time_peak"]]%(EVENTS_OFFSET) + n_offsets*EVENTS_OFFSET;
        }
        tps_all.insert(tps_all.end(), tps.begin(), tps.end());
    }
    // set all the event numbers to the same value
    for (int i = 0; i < tps_all.size(); i++) {
        tps_all[i][variables_to_index["event"]] = event_number;
    }

    final_collective_cluster.set_tps(tps_all);
    return final_collective_cluster;
}

