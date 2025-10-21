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
    
    // Count TPs by generator to determine dominant truth
    std::map<std::string, int> generator_counts;
    std::map<const TrueParticle*, int> particle_counts;
    std::map<const Neutrino*, int> neutrino_counts;
    int tps_with_truth = 0;
    
    // Get ADC to energy conversion factors
    const double ADC_TO_MEV_COLLECTION = ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_collection");
    const double ADC_TO_MEV_INDUCTION = ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_induction");
    
    for (auto& tp : tps_) {
        total_charge_ += tp->GetAdcIntegral();
        // Convert ADC to energy using the appropriate conversion factor for the view
        double adc_to_mev = (tp->GetView() == "X") ? ADC_TO_MEV_COLLECTION : ADC_TO_MEV_INDUCTION;
        total_energy_ += static_cast<float>(tp->GetAdcIntegral() / adc_to_mev);
        
        // Get truth information from TP
        const TrueParticle* true_particle = tp->GetTrueParticle();
        if (true_particle != nullptr) {
            tps_with_truth++;
            
            // Count particles
            particle_counts[true_particle]++;
            
            // Count generators
            std::string gen_name = true_particle->GetGeneratorName();
            generator_counts[gen_name]++;
            
            // Count neutrinos
            const Neutrino* neutrino = true_particle->GetNeutrino();
            if (neutrino != nullptr) {
                neutrino_counts[neutrino]++;
            }
        }
    }
    
    // Set generator fractions
    if (tps_.size() > 0) {
        supernova_tp_fraction_ = (generator_counts.count("marley") > 0) ? 
            (float)generator_counts["marley"] / tps_.size() : 0.0f;
        generator_tp_fraction_ = (tps_with_truth > 0) ? 
            (float)tps_with_truth / tps_.size() : 0.0f;
    }
    
    // Find dominant particle (most TPs matched to it)
    const TrueParticle* dominant_particle = nullptr;
    int max_particle_count = 0;
    for (const auto& kv : particle_counts) {
        if (kv.second > max_particle_count) {
            max_particle_count = kv.second;
            dominant_particle = kv.first;
        }
    }
    
    // Set truth information from dominant particle
    if (dominant_particle != nullptr) {
        true_pos_ = {dominant_particle->GetX(), dominant_particle->GetY(), dominant_particle->GetZ()};
        true_dir_ = {dominant_particle->GetPx(), dominant_particle->GetPy(), dominant_particle->GetPz()};
        true_momentum_ = {dominant_particle->GetPx(), dominant_particle->GetPy(), dominant_particle->GetPz()};
        
        // Calculate actual deposited energy from TPs belonging to dominant particle
        // Use ADC-based energy to avoid including rest mass from SimIDE
        double deposited_energy = 0.0;
        int dominant_tp_count = 0;
        
        // Determine conversion factor based on the cluster's view
        std::string cluster_view = tps_.empty() ? "X" : tps_[0]->GetView();
        double adc_to_mev = (cluster_view == "X") ? ADC_TO_MEV_COLLECTION : ADC_TO_MEV_INDUCTION;
        
        for (const auto& tp : tps_) {
            if (tp->GetTrueParticle() == dominant_particle) {
                double tp_energy = tp->GetAdcIntegral() / adc_to_mev;
                deposited_energy += tp_energy;
                dominant_tp_count++;
                if (debugMode) {
                    LogInfo << "    TP ADC: " << tp->GetAdcIntegral() 
                            << " -> Energy: " << tp_energy << " MeV"
                            << " | Plane: " << tp->GetView()
                            << " | Channel: " << tp->GetChannel()
                            << " | Time Start: " << tp->GetTimeStart()
                            << " | Generator: " << tp->GetTrueParticle()->GetGeneratorName()
                            << " | PDG: " << tp->GetTrueParticle()->GetPdg() << std::endl;
                }
            }
        }
        
        // Use ADC-based deposited energy
        true_particle_energy_ = deposited_energy;
        if (debugMode) {
            LogInfo << "  Using ADC-based deposited energy: " << deposited_energy << " MeV" << std::endl;
            LogInfo << "  (Conversion factor: " << adc_to_mev << " ADC/MeV for " << cluster_view << " plane)" << std::endl;
        }
        
        true_label_ = dominant_particle->GetGeneratorName();
        true_pdg_ = dominant_particle->GetPdg();
        
        // Set neutrino information
        const Neutrino* dominant_neutrino = dominant_particle->GetNeutrino();
        if (dominant_neutrino != nullptr) {
            true_neutrino_energy_ = dominant_neutrino->GetEnergy();
            true_interaction_ = dominant_neutrino->GetInteraction();
        } else {
            true_neutrino_energy_ = -1.0f;
            true_interaction_ = "UNKNOWN";
        }

        if (debugMode){
            LogInfo << "Information about dominant particle extracted." << std::endl;
            LogInfo << "  Dominant particle: " << dominant_particle->GetGeneratorName() 
                    << " (PDG: " << dominant_particle->GetPdg() << ")" << std::endl;
            LogInfo << "  Particle initial energy: " << dominant_particle->GetEnergy() << " MeV" << std::endl;
            LogInfo << "  Deposited energy (SimIDE): " << deposited_energy << " MeV" << std::endl;
            LogInfo << "  TPs from dominant particle: " << dominant_tp_count << " / " << tps_.size() << std::endl;
            LogInfo << "  True Position: (" << true_pos_[0] << ", " << true_pos_[1] << ", " << true_pos_[2] << ")" << std::endl;
            LogInfo << "  True Momentum: (" << true_momentum_[0] << ", " << true_momentum_[1] << ", " << true_momentum_[2] << ")" << std::endl;
            LogInfo << "  True Neutrino Energy: " << true_neutrino_energy_ << std::endl;
            LogInfo << "  True Interaction: " << true_interaction_ << std::endl;
        }
    } else {
        // No truth info available
        true_pos_ = {0.0f, 0.0f, 0.0f};
        true_dir_ = {0.0f, 0.0f, 0.0f};
        true_momentum_ = {0.0f, 0.0f, 0.0f};
        true_particle_energy_ = -1.0f;
        true_label_ = "UNKNOWN";
        true_neutrino_energy_ = -1.0f;
        true_interaction_ = "UNKNOWN";
        true_pdg_ = 0;
    }
    if (debugMode) {
        printClusterInfo();
    }
}

float Cluster::get_total_charge() {
    return total_charge_;
}

float Cluster::get_total_energy() {
    return total_energy_;
}

float distance(Cluster cluster1, Cluster cluster2) {
    float x1 = cluster1.get_true_pos()[0];
    float y1 = cluster1.get_true_pos()[1];
    float z1 = cluster1.get_true_pos()[2];
    float x2 = cluster2.get_true_pos()[0];
    float y2 = cluster2.get_true_pos()[1];
    float z2 = cluster2.get_true_pos()[2];
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2));
}


void Cluster::printClusterInfo() const {
    LogInfo << "Cluster Info:" << std::endl;
    LogInfo << "  Number of TPs: " << tps_.size() << std::endl;
    LogInfo << "  True Position: (" << true_pos_[0] << ", " << true_pos_[1] << ", " << true_pos_[2] << ")" << std::endl;
    LogInfo << "  True Momentum: (" << true_momentum_[0] << ", " << true_momentum_[1] << ", " << true_momentum_[2] << ")" << std::endl;
    LogInfo << "  True Direction: (" << true_dir_[0] << ", " << true_dir_[1] << ", " << true_dir_[2] << ")" << std::endl;
    LogInfo << "  True Neutrino Energy: " << true_neutrino_energy_ << std::endl;
    LogInfo << "  True Particle Energy: " << true_particle_energy_ << std::endl;
    LogInfo << "  True Label: " << true_label_ << std::endl;
    LogInfo << "  Supernova TP Fraction: " << supernova_tp_fraction_ << std::endl;
    LogInfo << "  Generator TP Fraction: " << generator_tp_fraction_ << std::endl;
    LogInfo << "  True Interaction: " << true_interaction_ << std::endl;
    LogInfo << "  Total Charge: " << total_charge_ << std::endl;
    LogInfo << "  Total Energy: " << total_energy_ << std::endl;
    LogInfo << "  True PDG: " << true_pdg_ << std::endl;
    LogInfo << "  Is Main Cluster: " << (is_main_cluster_ ? "Yes" : "No") << std::endl;
}
