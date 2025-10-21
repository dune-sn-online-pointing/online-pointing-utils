#ifndef CLUSTER_H
#define CLUSTER_H

#include "TriggerPrimitive.hpp"
#include "global.h"

extern std::map<std::string, int> variables_to_index;

// could inherit from TP, but it might be just added complexity
class Cluster {
    public:
        Cluster() {}
        // cluster(std::vector<std::vector<double>> tps) { tps_ = tps; true_dir_ = {0, 0, 0}; true_pos_ = {0, 0, 0}; reco_pos_ = {0, 0, 0}; min_distance_from_true_pos_ = 0; true_neutrino_energy_ = 0; true_label_ = -99; true_interaction_ = -99; supernova_tp_fraction_ = 0; update_cluster_info(); }
        Cluster(std::vector<TriggerPrimitive*> tps);
        ~Cluster() {}

        void update_cluster_info();
        
        // getters
        TriggerPrimitive* get_tp(int i) { return tps_.at(i); }
        int get_size() { return tps_.size(); }
        std::vector<float> get_true_pos() { return true_pos_; }
        std::vector<float> get_true_momentum() { return true_momentum_; }
        std::vector<float> get_true_dir() { return true_dir_; }
        float get_true_neutrino_energy() { return true_neutrino_energy_; }
        float get_true_particle_energy() { return true_particle_energy_; }
        std::string get_true_label() { return true_label_; }
        float get_min_distance_from_true_pos() const { return min_distance_from_true_pos_; }
        float get_supernova_tp_fraction() const { return supernova_tp_fraction_; }
        float get_generator_tp_fraction() const { return generator_tp_fraction_; }
        std::string get_true_interaction() const { return true_interaction_; }
        float get_total_charge(); // { return total_charge_; }
        float get_total_energy(); // { return total_energy_; }
        float get_number_of_tps() { return tps_.size(); }
        int get_event() { return tps_.at(0)->GetEvent(); }
        int get_true_pdg() const { return true_pdg_; }
        bool get_is_main_cluster() const { return is_main_cluster_; }        
        
        // setters
        std::vector<TriggerPrimitive*> get_tps() const { return tps_; }
        void set_tps(std::vector<TriggerPrimitive*> tps) { tps_ = tps;}; //update_cluster_info();} TODO
        void set_true_pos(std::vector<float> pos) { true_pos_ = pos; }
        void set_true_momentum(std::vector<float> momentum) { true_momentum_ = momentum; }
        void set_true_label(std::string label) { true_label_ = label; }
        void set_true_energy(float energy) { true_neutrino_energy_ = energy; }
        void set_true_dir(std::vector<float> dir) { true_dir_ = dir; }
        void set_min_distance_from_true_pos(float min_distance) { min_distance_from_true_pos_ = min_distance; }
        void set_supernova_tp_fraction(float fraction) { supernova_tp_fraction_ = fraction; }
        void set_generator_tp_fraction(float fraction) { generator_tp_fraction_ = fraction; }
        void set_true_interaction(std::string interaction) { true_interaction_ = interaction; }
        void set_true_pdg(int pdg) { true_pdg_ = pdg; }
        void set_is_main_cluster(bool is_main) { is_main_cluster_ = is_main; }
        // void set_total_charge(float charge) { total_charge_ = charge; }

        // methods 
        bool isCleanCluster();
        void printClusterInfo() const;

    private:
        // std::vector<std::vector<double>> tps_;
        std::vector<TriggerPrimitive*> tps_ {};
        std::vector<float> true_pos_ {0.0f, 0.0f, 0.0f};    
        std::vector<float> true_momentum_ {0.0f, 0.0f, 0.0f};
        std::vector<float> true_dir_ {0.0f, 0.0f, 0.0f};
        std::string true_interaction_  {"UNKNOWN"}; // ES or CC
        float min_distance_from_true_pos_ {0.0f};
        float true_neutrino_energy_ {-1.0f};
        float true_particle_energy_ {-1.0f};
        std::string true_label_ = {"UNKNOWN"}; // could be nicer than this TODO
        float supernova_tp_fraction_ {0.0f};
        float generator_tp_fraction_ {0.0f};
        float total_charge_ {0.0f};
        float total_energy_ {0.0f};
        int true_pdg_ {0};
        bool is_main_cluster_ {false};
};

float distance(Cluster cluster1, Cluster cluster2);

#endif // CLUSTER_H

