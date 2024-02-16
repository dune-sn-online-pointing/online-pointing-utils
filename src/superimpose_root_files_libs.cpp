#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <climits>
#include "../inc/group.h"
#include "../inc/group_to_root_libs.h"
#include "../inc/position_calculator.h"
#include "../inc/superimpose_root_files_libs.h"
// include root libraries
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"
#include "TMatrixD.h"


float distance(group group1, group group2) {
    float x1 = group1.get_reco_pos()[0];
    float y1 = group1.get_reco_pos()[1];
    float z1 = group1.get_reco_pos()[2];
    float x2 = group2.get_reco_pos()[0];
    float y2 = group2.get_reco_pos()[1];
    float z2 = group2.get_reco_pos()[2];
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2));
}


group filter_groups_within_radius(std::vector<group>& groups, float radius) {
    std::vector<group> filtered_groups;
    int idx_best = -1;
    int idx = 0;
    int event_number = -1;
    for (auto g : groups) {
        if (g.get_supernova_tp_fraction() > 0.) {
            if (idx_best == -1) {
                idx_best = idx;
                event_number = g.get_tp(0)[variables_to_index["event"]];
            } 
            else {
                if (g.get_min_distance_from_true_pos() < groups[idx_best].get_min_distance_from_true_pos()) {
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

    for (auto const& g : groups) {
        if (distance(g, groups[idx_best]) < radius) {
            filtered_groups.push_back(g);
        }
    }

    group final_collective_group;
    std::vector<std::vector<int>> tps;
    std::vector<std::vector<int>> tps_all;
    for (auto const& g : filtered_groups) {
        tps = g.get_tps();
        tps_all.insert(tps_all.end(), tps.begin(), tps.end());
    }
    // set all the event numbers to the same value
    for (int i = 0; i < tps_all.size(); i++) {
        tps_all[i][variables_to_index["event"]] = event_number;
    }

    final_collective_group.set_tps(tps_all);
    return final_collective_group;
}

