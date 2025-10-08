#include "root.h"

#include "Cluster.h"
#include "Backtracking.h"

#include <Logger.h>

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

// class cluster  

Cluster::Cluster(std::vector<TriggerPrimitive*> tps) {
    // check that all tps come from same event
    // check that all tps come from same view

    if (tps.size() == 0) {
        LogError << "Cluster has no TPs!" << std::endl;
        return;
    }

    for (auto& tp : tps) {
        if (tp == nullptr) {
            LogError << "Cluster has null TP!" << std::endl;
            return;
        }
        
        if (tp->GetEvent() != tps.at(0)->GetEvent()) {
            LogError << "Cluster has TPs from different events!" << std::endl;
            return;
        }

        if (tp->GetView() != tps.at(0)->GetView()) {
            LogError << "Cluster has TPs from different views!" << std::endl;
            return;
        }
    }


    tps_ = tps;
    update_cluster_info();
}

void Cluster::update_cluster_info() {
    total_charge_ = 0.0f;
    total_energy_ = 0.0f;
    for (auto& tp : tps_) {
        total_charge_ += tp->GetAdcIntegral();
        total_energy_ += tp->GetAdcPeak();
    }
}

float Cluster::get_total_charge() {
    return total_charge_;
}

float Cluster::get_total_energy() {
    return total_energy_;
}

// TODO remove?
float distance(Cluster cluster1, Cluster cluster2) {
    float x1 = cluster1.get_reco_pos()[0];
    float y1 = cluster1.get_reco_pos()[1];
    float z1 = cluster1.get_reco_pos()[2];
    float x2 = cluster2.get_reco_pos()[0];
    float y2 = cluster2.get_reco_pos()[1];
    float z2 = cluster2.get_reco_pos()[2];
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2));
}
