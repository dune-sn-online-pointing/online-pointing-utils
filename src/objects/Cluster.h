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
        std::vector<Double_t> get_true_pos() { return true_pos_; }
        std::vector<Double_t> get_true_momentum() { return true_momentum_; }
        std::vector<Double_t> get_true_dir() { return true_dir_; }
        std::vector<Double_t> get_true_neutrino_momentum() { return true_neutrino_momentum_; }
        Double_t get_true_neutrino_energy() { return true_neutrino_energy_; }
        Double_t get_true_particle_energy() { return true_particle_energy_; }
        std::string get_true_label() { return true_label_; }
        Double_t get_min_distance_from_true_pos() const { return min_distance_from_true_pos_; }
        Double_t get_supernova_tp_fraction() const { return supernova_tp_fraction_; }
        Double_t get_generator_tp_fraction() const { return generator_tp_fraction_; }
        bool get_is_es_interaction() const { return is_es_interaction_; }
        Double_t get_total_charge(); // { return total_charge_; }
        Double_t get_total_energy(); // { return total_energy_; }
        Double_t get_number_of_tps() { return tps_.size(); }
        UInt_t get_event() { return tps_.at(0)->GetEvent(); }
        UInt_t get_run() { return tps_.at(0)->GetRun(); }
        int get_true_pdg() const { return true_pdg_; }
        bool get_is_main_cluster() const { return is_main_cluster_; }
        int get_cluster_id() const { return cluster_id_; }
        
        // setters
        std::vector<TriggerPrimitive*> get_tps() const { return tps_; }
        void set_tps(std::vector<TriggerPrimitive*> tps) { tps_ = tps;}; //update_cluster_info();} TODO
        void set_true_pos(std::vector<Double_t> pos) { true_pos_ = pos; }
        void set_true_momentum(std::vector<Double_t> momentum) { true_momentum_ = momentum; }
        void set_true_label(std::string label) { true_label_ = label; }
        void set_true_energy(Double_t energy) { true_neutrino_energy_ = energy; }
        void set_true_neutrino_energy(Double_t energy) { true_neutrino_energy_ = energy; }
        void set_true_particle_energy(Double_t energy) { true_particle_energy_ = energy; }
        void set_true_dir(std::vector<Double_t> dir) { true_dir_ = dir; }
        void set_true_neutrino_momentum(std::vector<Double_t> momentum) { true_neutrino_momentum_ = momentum; }
        void set_min_distance_from_true_pos(Double_t distance) { min_distance_from_true_pos_ = distance; }
        void set_supernova_tp_fraction(Double_t fraction) { supernova_tp_fraction_ = fraction; }
        void set_generator_tp_fraction(Double_t fraction) { generator_tp_fraction_ = fraction; }
        void set_is_es_interaction(bool is_es) { is_es_interaction_ = is_es; }
        void set_true_pdg(int pdg) { true_pdg_ = pdg; }
        void set_is_main_cluster(bool is_main) { is_main_cluster_ = is_main; }
        void set_cluster_id(int id) { cluster_id_ = id; }
        // void set_total_charge(Double_t charge) { total_charge_ = charge; }

        // methods 
        bool isCleanCluster();
        void printClusterInfo() const;

    private:
        // std::vector<std::vector<double>> tps_;
        std::vector<TriggerPrimitive*> tps_ {};
        std::vector<Double_t> true_pos_ {0.0f, 0.0f, 0.0f};    
        std::vector<Double_t> true_momentum_ {0.0f, 0.0f, 0.0f};
        std::vector<Double_t> true_dir_ {0.0f, 0.0f, 0.0f};
        std::vector<Double_t> true_neutrino_momentum_ {0.0f, 0.0f, 0.0f};
        bool is_es_interaction_ {false}; // true if ES, false if CC or unknown
        Double_t min_distance_from_true_pos_ {0.0f};
        Double_t true_neutrino_energy_ {-1.0f};
        Double_t true_particle_energy_ {-1.0f};
        std::string true_label_ = {"UNKNOWN"}; // could be nicer than this TODO
        Double_t supernova_tp_fraction_ {0.0f};
        Double_t generator_tp_fraction_ {0.0f};
        Double_t total_charge_ {0.0f};
        Double_t total_energy_ {0.0f};
        int true_pdg_ {0};
        bool is_main_cluster_ {false};
        int cluster_id_ {-1}; // Unique ID per file to link matched clusters
};

Double_t distance(Cluster cluster1, Cluster cluster2);

#endif // CLUSTER_H

