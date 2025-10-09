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

#include "Cluster.h"
#include "Clustering.h"
// #include "position_calculator.h"
#include "superimpose_root_files_libs.h"



Cluster filter_clusters_within_radius(std::vector<Cluster>& clusters, float radius) {
    std::vector<Cluster> filtered_clusters;
    int idx_best = -1;
    int idx = 0;
    int event_number = -1;
    int n_offsets = 0;
    for (auto g : clusters) {
        if (g.get_supernova_tp_fraction() > 0.) {
            if (idx_best == -1) {
                idx_best = idx;
                event_number = g.get_tp(0)->GetEvent();
                n_offsets = g.get_tp(0)->GetTimeStart() ;
            } 
            else {
                if (g.get_min_distance_from_true_pos() < clusters[idx_best].get_min_distance_from_true_pos()) {
                    idx_best = idx;
                }
            }
        }
    idx++;
    }

    if (clusters[idx_best].get_min_distance_from_true_pos()>5){
        std::cout<<"WARNING: using adc integral for determining main track" << std::endl;
        idx_best = -1;
        int adc_integral_best = -1; 
        int adc_integral = 0;
        for (int i=0; i<clusters.size(); i++){
            for (auto tp: clusters[i].get_tps()){
                adc_integral+=tp->GetAdcIntegral();                
            }   
            if (adc_integral>adc_integral_best){
                adc_integral_best = adc_integral;
                idx_best = i;
            }
            adc_integral = 0;
        }
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

    Cluster final_collective_cluster;
    std::vector<TriggerPrimitive*> tps; 
    std::vector<TriggerPrimitive*> tps_all;
    for (auto const& g : filtered_clusters) {
        tps = g.get_tps();
        // fix the different offsets coming from different events
        // for (int i = 0; i < tps.size(); i++) {
        //     tps.at(i)->time_start =  (int) tps.at(i)->time_start%(EVENTS_OFFSET) + n_offsets*EVENTS_OFFSET;
        //     tps.at(i)->samples_to_peak = (int) tps.at(i)->samples_to_peak%(EVENTS_OFFSET) + n_offsets*EVENTS_OFFSET; // careful here TODO fix since def changed
        // }
        tps_all.insert(tps_all.end(), tps.begin(), tps.end());
    }
    // set all the event numbers to the same value
    for (int i = 0; i < tps_all.size(); i++) {
        tps_all[i]->SetEvent(event_number);
    }

    final_collective_cluster.set_tps(tps_all);
    return final_collective_cluster;
}

