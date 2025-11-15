#include "root.h"

#include "Cluster.h"
#include "Backtracking.h"

#include <Logger.h>

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

// class cluster  

Cluster::Cluster(std::vector<TriggerPrimitive*> tps) {
    // check that all tps come from same event
    // NOTE: For multiplane clusters, TPs may come from different views (U, V, X)
    // The view check is skipped if TPs are from multiple views

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
    }
    
    // Check if all TPs are from same view (single-plane cluster)
    // If they're not all the same view, this is a multiplane cluster
    bool same_view = true;
    std::string first_view = tps.at(0)->GetView();
    for (auto& tp : tps) {
        if (tp->GetView() != first_view) {
            same_view = false;
            break;
        }
    }
    
    // Only enforce view consistency for single-plane clusters
    if (same_view) {
        // All TPs have same view - this is expected for single-plane clusters
    } else {
        // Mixed views - this is a multiplane cluster, which is valid
        if (debugMode) LogDebug << "Creating multiplane cluster with mixed views" << std::endl;
    }

    tps_ = tps;
    update_cluster_info();
}

void Cluster::update_cluster_info() {
    total_charge_ = 0.0f;
    total_energy_ = 0.0f;
    
    // Count TPs by generator to determine dominant truth
    std::map<std::string, int> generator_counts;
    
    // Use a composite key to identify unique particles: (generator, PDG, x, y, z)
    struct ParticleKey {
        std::string generator;
        int pdg;
        float x, y, z;
        
        bool operator<(const ParticleKey& other) const {
            if (generator != other.generator) return generator < other.generator;
            if (pdg != other.pdg) return pdg < other.pdg;
            if (std::abs(x - other.x) > 0.1) return x < other.x;
            if (std::abs(y - other.y) > 0.1) return y < other.y;
            return z < other.z;
        }
    };
    
    std::map<ParticleKey, int> particle_counts;
    
    // Struct to store neutrino info associated with each particle key
    struct NeutrinoInfo {
        std::string interaction;
        float energy;
        float x, y, z;
        float px, py, pz;
    };
    std::map<ParticleKey, NeutrinoInfo> neutrino_info_map;
    
    int tps_with_truth = 0;
    
    // Get ADC to energy conversion factors
    const double ADC_TO_MEV_COLLECTION = ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_collection");
    const double ADC_TO_MEV_INDUCTION = ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_induction");
    
    for (auto& tp : tps_) {
        total_charge_ += tp->GetAdcIntegral();
        // Convert ADC to energy using the appropriate conversion factor for the view
        double adc_to_mev = (tp->GetView() == "X") ? ADC_TO_MEV_COLLECTION : ADC_TO_MEV_INDUCTION;
        total_energy_ += static_cast<float>(tp->GetAdcIntegral() / adc_to_mev);
        
        // Get truth information from TP embedded data
        std::string gen_name = tp->GetGeneratorName();
        if (gen_name != "UNKNOWN") {
            tps_with_truth++;
            
            // Count generators
            generator_counts[gen_name]++;
            
            // Create particle key
            ParticleKey key{gen_name, tp->GetParticlePDG(), 
                           tp->GetParticleX(), tp->GetParticleY(), tp->GetParticleZ()};
            
            // Count particles
            particle_counts[key]++;
            
            // Store neutrino info if available (neutrino_energy >= 0 means we have truth)
            // Changed from > 0 to >= 0 because neutrino_energy = 0 is valid for background
            // and the key indicator of truth is generator_name != "UNKNOWN"
            if (tp->GetNeutrinoEnergy() >= 0 || !tp->GetNeutrinoInteraction().empty()) {
                neutrino_info_map[key] = {tp->GetNeutrinoInteraction(), tp->GetNeutrinoEnergy(),
                                         tp->GetNeutrinoX(), tp->GetNeutrinoY(), tp->GetNeutrinoZ(),
                                         tp->GetNeutrinoPx(), tp->GetNeutrinoPy(), tp->GetNeutrinoPz()};
            }
        }
    }
    
    // Set generator fractions
    // Only recalculate if TPs have truth information, otherwise preserve existing value
    if (tps_.size() > 0 && tps_with_truth > 0) {
        // Count MARLEY TPs (case-insensitive)
        int marley_count = 0;
        for (const auto& kv : generator_counts) {
            std::string gen_lower = kv.first;
            std::transform(gen_lower.begin(), gen_lower.end(), gen_lower.begin(), ::tolower);
            if (gen_lower.find("marley") != std::string::npos) {
                marley_count += kv.second;
            }
        }
        supernova_tp_fraction_ = (float)marley_count / tps_.size();
        generator_tp_fraction_ = (float)tps_with_truth / tps_.size();
    }
    
    // Find dominant particle (most TPs matched to it)
    ParticleKey dominant_key{"UNKNOWN", 0, 0, 0, 0};
    int max_particle_count = 0;
    for (const auto& kv : particle_counts) {
        if (kv.second > max_particle_count) {
            max_particle_count = kv.second;
            dominant_key = kv.first;
        }
    }
    
    // Debug: Always log for MARLEY clusters
    if (supernova_tp_fraction_ > 0) {
        if (debugMode) LogDebug << "MARLEY cluster: marley_fraction=" << supernova_tp_fraction_ 
                  << " dominant_gen=" << dominant_key.generator 
                  << " max_count=" << max_particle_count 
                  << " tps_size=" << tps_.size() << std::endl;
    }
    
    // Set truth information from dominant particle
    if (dominant_key.generator != "UNKNOWN" && max_particle_count > 0) {
        true_pos_ = {dominant_key.x, dominant_key.y, dominant_key.z};
        
        // Debug: Log attempt to find momentum
        bool found_momentum = false;
        
        // Get momentum from any TP with this dominant particle
        for (const auto& tp : tps_) {
            if (tp->GetGeneratorName() == dominant_key.generator &&
                tp->GetParticlePDG() == dominant_key.pdg &&
                std::abs(tp->GetParticleX() - dominant_key.x) < 0.1 &&
                std::abs(tp->GetParticleY() - dominant_key.y) < 0.1 &&
                std::abs(tp->GetParticleZ() - dominant_key.z) < 0.1) {
                true_momentum_ = {tp->GetParticlePx(), tp->GetParticlePy(), tp->GetParticlePz()};
                found_momentum = true;
                float p_mag = std::sqrt(true_momentum_[0]*true_momentum_[0] + 
                                       true_momentum_[1]*true_momentum_[1] + 
                                       true_momentum_[2]*true_momentum_[2]);
                if (p_mag > 0) {
                    true_dir_ = {true_momentum_[0]/p_mag, true_momentum_[1]/p_mag, true_momentum_[2]/p_mag};
                }
                break;
            }
        }
        
        // Debug: If momentum not found, log first TP's details
        if (!found_momentum && tps_.size() > 0) {
            const auto& first_tp = tps_[0];
            std::cerr << "WARNING: Could not find momentum for cluster! Dominant: gen=" << dominant_key.generator 
                      << " pdg=" << dominant_key.pdg << " pos=(" << dominant_key.x << "," << dominant_key.y << "," << dominant_key.z << ")"
                      << " | First TP: gen=" << first_tp->GetGeneratorName()
                      << " pdg=" << first_tp->GetParticlePDG()
                      << " pos=(" << first_tp->GetParticleX() << "," << first_tp->GetParticleY() << "," << first_tp->GetParticleZ() << ")"
                      << " momentum=(" << first_tp->GetParticlePx() << "," << first_tp->GetParticlePy() << "," << first_tp->GetParticlePz() << ")" << std::endl;
        }
        
        // Calculate actual deposited energy from TPs belonging to dominant particle
        // Use ADC-based energy to avoid including rest mass from SimIDE
        double deposited_energy = 0.0;
        int dominant_tp_count = 0;
        
        // Determine conversion factor based on the cluster's view
        std::string cluster_view = tps_.empty() ? "X" : tps_[0]->GetView();
        double adc_to_mev = (cluster_view == "X") ? ADC_TO_MEV_COLLECTION : ADC_TO_MEV_INDUCTION;
        
        for (const auto& tp : tps_) {
            if (tp->GetGeneratorName() == dominant_key.generator &&
                tp->GetParticlePDG() == dominant_key.pdg &&
                std::abs(tp->GetParticleX() - dominant_key.x) < 0.1 &&
                std::abs(tp->GetParticleY() - dominant_key.y) < 0.1 &&
                std::abs(tp->GetParticleZ() - dominant_key.z) < 0.1) {
                double tp_energy = tp->GetAdcIntegral() / adc_to_mev;
                deposited_energy += tp_energy;
                dominant_tp_count++;
                if (debugMode) {
                    LogInfo << "    TP ADC: " << tp->GetAdcIntegral() 
                            << " -> Energy: " << tp_energy << " MeV"
                            << " | Plane: " << tp->GetView()
                            << " | Channel: " << tp->GetChannel()
                            << " | Time Start: " << tp->GetTimeStart()
                            << " | Generator: " << tp->GetGeneratorName()
                            << " | PDG: " << tp->GetParticlePDG() << std::endl;
                }
            }
        }
        
        // Use ADC-based deposited energy
        true_particle_energy_ = deposited_energy;
        if (debugMode) {
            LogInfo << "  Using ADC-based deposited energy: " << deposited_energy << " MeV" << std::endl;
            LogInfo << "  (Conversion factor: " << adc_to_mev << " ADC/MeV for " << cluster_view << " plane)" << std::endl;
        }
        
        true_label_ = dominant_key.generator;
        true_pdg_ = dominant_key.pdg;
        
        // Set neutrino information from the stored map
        if (neutrino_info_map.count(dominant_key) > 0) {
            const auto& nu_info = neutrino_info_map[dominant_key];
            true_neutrino_energy_ = nu_info.energy;
            is_es_interaction_ = (nu_info.interaction == "ES");
        } else {
            true_neutrino_energy_ = -1.0f;
            is_es_interaction_ = false;
        }

        if (debugMode){
            LogInfo << "Information about dominant particle extracted." << std::endl;
            LogInfo << "  Dominant particle: " << dominant_key.generator
                    << " (PDG: " << dominant_key.pdg << ")" << std::endl;
            LogInfo << "  Deposited energy: " << deposited_energy << " MeV" << std::endl;
            LogInfo << "  TPs from dominant particle: " << dominant_tp_count << " / " << tps_.size() << std::endl;
            LogInfo << "  True Position: (" << true_pos_[0] << ", " << true_pos_[1] << ", " << true_pos_[2] << ")" << std::endl;
            LogInfo << "  True Momentum: (" << true_momentum_[0] << ", " << true_momentum_[1] << ", " << true_momentum_[2] << ")" << std::endl;
            LogInfo << "  True Neutrino Energy: " << true_neutrino_energy_ << std::endl;
            LogInfo << "  True Interaction: " << (is_es_interaction_ ? "ES" : "CC") << std::endl;
        }
    } else {
        // No truth info available
        true_pos_ = {0.0f, 0.0f, 0.0f};
        true_dir_ = {0.0f, 0.0f, 0.0f};
        true_momentum_ = {0.0f, 0.0f, 0.0f};
        true_particle_energy_ = -1.0f;
        true_label_ = "UNKNOWN";
        true_neutrino_energy_ = -1.0f;
        is_es_interaction_ = false;
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
    LogInfo << "  True Interaction: " << (is_es_interaction_ ? "ES" : "CC") << std::endl;
    LogInfo << "  Total Charge: " << total_charge_ << std::endl;
    LogInfo << "  Total Energy: " << total_energy_ << std::endl;
    LogInfo << "  True PDG: " << true_pdg_ << std::endl;
    LogInfo << "  Is Main Cluster: " << (is_main_cluster_ ? "Yes" : "No") << std::endl;
}
