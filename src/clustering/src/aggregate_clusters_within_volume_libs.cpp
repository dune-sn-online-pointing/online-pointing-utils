#include <vector>
#include <iostream>
#include "aggregate_clusters_within_volume_libs.h"
// #include "position_calculator.h"
#include "Cluster.h"


std::vector<Cluster> aggregate_clusters_within_volume(std::vector<Cluster> clusters, float radius, std::vector<float> predictions, float threshold) {
    std::vector<std::vector<Cluster>> vec_aggregated_clusters;
    
    std::cout << "predictions.size(): " << predictions.size() << std::endl;
    std::cout << "clusters.size(): " << clusters.size() << std::endl;
    for (int i = 0; i < predictions.size(); i++) {
        if (predictions[i] > threshold) {
        std::vector<Cluster> aggregated_cluster = {clusters[i]};
        vec_aggregated_clusters.push_back(aggregated_cluster);
        }
    }

    for (int i = 0; i < clusters.size(); i++) {
        for (int j = 0; j < vec_aggregated_clusters.size(); j++) {
            if (distance(clusters[i], vec_aggregated_clusters[j][0]) < radius) {
                // check if the Cluster[i] == vec_aggregated_clusters[j][0]
                if (clusters[i].get_reco_pos() == vec_aggregated_clusters[j][0].get_reco_pos()) {
                continue;
                } else {
                    vec_aggregated_clusters[j].push_back(clusters[i]);
                }
            }
        }
    }

    std::vector<Cluster> aggregated_clusters;
    for (int i = 0; i < vec_aggregated_clusters.size(); i++) {
        std::vector<TriggerPrimitive*> aggregated_tps;
        for (int j = 0; j < vec_aggregated_clusters[i].size(); j++) {
            for (int k = 0; k < vec_aggregated_clusters[i][j].get_tps().size(); k++) {
                aggregated_tps.push_back(vec_aggregated_clusters[i][j].get_tps().at(k));
            }
        }
        Cluster aggregated_cluster(aggregated_tps);
        aggregated_clusters.push_back(aggregated_cluster);        
    }
    
    return aggregated_clusters;
}



