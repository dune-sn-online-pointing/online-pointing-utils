#ifndef CLUSTER_H
#define CLUSTER_H

#include <vector>
#include <cmath>
#include <map>
#include <string>

extern std::map<std::string, int> variables_to_index;


class cluster {
public:
    cluster() { true_dir_ = {0, 0, 0}; true_pos_ = {0, 0, 0}; reco_pos_ = {0, 0, 0}; min_distance_from_true_pos_ = 0; true_energy_ = 0; true_label_ = -99; true_interaction_ = -99; supernova_tp_fraction_ = 0; }
    cluster(std::vector<std::vector<double>> tps) { tps_ = tps; true_dir_ = {0, 0, 0}; true_pos_ = {0, 0, 0}; reco_pos_ = {0, 0, 0}; min_distance_from_true_pos_ = 0; true_energy_ = 0; true_label_ = -99; true_interaction_ = -99; supernova_tp_fraction_ = 0; update_cluster_info(); }
    ~cluster() { }

    void update_cluster_info();

    std::vector<std::vector<double>> get_tps() const { return tps_; }
    void set_tps(std::vector<std::vector<double>> tps) { tps_ = tps; update_cluster_info();}
    std::vector<double> get_tp(int i) { return tps_[i]; }
    int get_size() { return tps_.size(); }
    std::vector<float> get_true_pos() { return true_pos_; }
    std::vector<float> get_reco_pos() { return reco_pos_; }
    std::vector<float> get_true_dir() { return true_dir_; }
    float get_true_energy() { return true_energy_; }
    int get_true_label() { return true_label_; }
    float get_min_distance_from_true_pos() const { return min_distance_from_true_pos_; }
    float get_supernova_tp_fraction() const { return supernova_tp_fraction_; }
    int get_true_interaction() const { return true_interaction_; }
    void set_true_pos(std::vector<float> pos) { true_pos_ = pos; }
    void set_true_label(int label) { true_label_ = label; }
    void set_true_energy(float energy) { true_energy_ = energy; }
    void set_true_dir(std::vector<float> dir) { true_dir_ = dir; }
    void set_reco_pos(std::vector<float> pos) { reco_pos_ = pos; }
    void set_min_distance_from_true_pos(float min_distance) { min_distance_from_true_pos_ = min_distance; }
    void set_supernova_tp_fraction(float fraction) { supernova_tp_fraction_ = fraction; }
    void set_true_interaction(int interaction) { true_interaction_ = interaction; }
private:
    std::vector<std::vector<double>> tps_;
    std::vector<float> true_pos_;    
    std::vector<float> true_dir_;
    std::vector<float> reco_pos_;    
    int true_interaction_;
    float min_distance_from_true_pos_;
    float true_energy_;
    int true_label_;
    float supernova_tp_fraction_;
};

#endif // CLUSTER_H
