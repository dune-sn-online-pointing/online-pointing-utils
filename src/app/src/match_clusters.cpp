#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <memory>
#include <algorithm>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "CmdLineParser.h"
#include "Logger.h"

#include "cluster_to_root_libs.h"
#include "cluster.h"
#include "match_clusters_libs.h"

#include <TFile.h>
#include <TDirectory.h>
#include <TKey.h>

LoggerInit([]{
  Logger::getUserHeader() << "[" << FILENAME << "]";
});

struct ClusterBundle {
    std::vector<std::unique_ptr<TriggerPrimitive>> owned_tps;
    std::vector<cluster> clusters;
};

static TTree* find_clusters_tree(TFile* file, const std::string& view) {
    if (file == nullptr) return nullptr;

    if (auto* dir = file->GetDirectory("clusters")) {
        if (auto* t = dynamic_cast<TTree*>(dir->Get(Form("clusters_tree_%s", view.c_str())))) {
            return t;
        }
        TIter nextKey(dir->GetListOfKeys());
        while (TKey* key = static_cast<TKey*>(nextKey())) {
            if (std::string(key->GetClassName()) == "TTree") {
                return dynamic_cast<TTree*>(key->ReadObj());
            }
        }
    }

    if (auto* t = dynamic_cast<TTree*>(file->Get(Form("clusters_tree_%s", view.c_str())))) return t;
    if (auto* t = dynamic_cast<TTree*>(file->Get("clusters_tree"))) return t;

    return nullptr;
}

static void load_clusters_for_view(const std::string& filename, const std::string& view, ClusterBundle& out) {
    TFile* file = TFile::Open(filename.c_str());
    if (file == nullptr || file->IsZombie()) {
        LogError << "Failed to open clusters file: " << filename << std::endl;
        if (file) { file->Close(); delete file; }
        return;
    }

    TTree* tree = find_clusters_tree(file, view);
    if (tree == nullptr) {
        LogError << "No clusters tree found in file: " << filename << std::endl;
        file->Close();
        delete file;
        return;
    }

    int event = 0;
    int n_tps = 0;
    float true_dir_x = 0.f;
    float true_dir_y = 0.f;
    float true_dir_z = 0.f;
    float true_neutrino_energy = -1.f;
    float true_particle_energy = -1.f;
    std::string* true_label = nullptr;
    std::string* true_interaction = nullptr;
    int reco_pos_x = 0;
    int reco_pos_y = 0;
    int reco_pos_z = 0;
    float min_distance_from_true_pos = 0.f;
    float supernova_tp_fraction = 0.f;
    float generator_tp_fraction = 0.f;

    std::vector<int>* tp_detector_channel = nullptr;
    std::vector<int>* tp_detector = nullptr;
    std::vector<int>* tp_samples_over_threshold = nullptr;
    std::vector<int>* tp_time_start = nullptr;
    std::vector<int>* tp_samples_to_peak = nullptr;
    std::vector<int>* tp_adc_peak = nullptr;
    std::vector<int>* tp_adc_integral = nullptr;

    tree->SetBranchAddress("event", &event);
    tree->SetBranchAddress("n_tps", &n_tps);
    if (tree->GetBranch("true_dir_x")) tree->SetBranchAddress("true_dir_x", &true_dir_x);
    if (tree->GetBranch("true_dir_y")) tree->SetBranchAddress("true_dir_y", &true_dir_y);
    if (tree->GetBranch("true_dir_z")) tree->SetBranchAddress("true_dir_z", &true_dir_z);
    if (tree->GetBranch("true_neutrino_energy")) tree->SetBranchAddress("true_neutrino_energy", &true_neutrino_energy);
    if (tree->GetBranch("true_particle_energy")) tree->SetBranchAddress("true_particle_energy", &true_particle_energy);
    if (tree->GetBranch("true_label")) tree->SetBranchAddress("true_label", &true_label);
    if (tree->GetBranch("true_interaction")) tree->SetBranchAddress("true_interaction", &true_interaction);
    if (tree->GetBranch("reco_pos_x")) tree->SetBranchAddress("reco_pos_x", &reco_pos_x);
    if (tree->GetBranch("reco_pos_y")) tree->SetBranchAddress("reco_pos_y", &reco_pos_y);
    if (tree->GetBranch("reco_pos_z")) tree->SetBranchAddress("reco_pos_z", &reco_pos_z);
    if (tree->GetBranch("min_distance_from_true_pos")) tree->SetBranchAddress("min_distance_from_true_pos", &min_distance_from_true_pos);
    if (tree->GetBranch("supernova_tp_fraction")) tree->SetBranchAddress("supernova_tp_fraction", &supernova_tp_fraction);
    if (tree->GetBranch("generator_tp_fraction")) tree->SetBranchAddress("generator_tp_fraction", &generator_tp_fraction);

    tree->SetBranchAddress("tp_detector_channel", &tp_detector_channel);
    tree->SetBranchAddress("tp_detector", &tp_detector);
    tree->SetBranchAddress("tp_samples_over_threshold", &tp_samples_over_threshold);
    tree->SetBranchAddress("tp_time_start", &tp_time_start);
    tree->SetBranchAddress("tp_samples_to_peak", &tp_samples_to_peak);
    tree->SetBranchAddress("tp_adc_peak", &tp_adc_peak);
    tree->SetBranchAddress("tp_adc_integral", &tp_adc_integral);

    Long64_t nentries = tree->GetEntries();
    out.clusters.reserve(out.clusters.size() + static_cast<size_t>(nentries));

    for (Long64_t i = 0; i < nentries; ++i) {
        tree->GetEntry(i);
        if (!tp_detector_channel || !tp_detector || !tp_samples_over_threshold || !tp_time_start ||
            !tp_samples_to_peak || !tp_adc_peak || !tp_adc_integral) {
            continue;
        }

        std::vector<TriggerPrimitive*> tps;
        size_t ntps = tp_detector_channel->size();
        tps.reserve(ntps);

        for (size_t k = 0; k < ntps; ++k) {
            int det = (k < tp_detector->size()) ? tp_detector->at(k) : 0;
            int det_ch = (k < tp_detector_channel->size()) ? tp_detector_channel->at(k) : 0;
            uint64_t channel = static_cast<uint64_t>(det) * APA::total_channels + static_cast<uint64_t>(det_ch);
            uint64_t sot = (k < tp_samples_over_threshold->size()) ? tp_samples_over_threshold->at(k) : 0;
            uint64_t tstart = (k < tp_time_start->size()) ? tp_time_start->at(k) : 0;
            uint64_t s2p = (k < tp_samples_to_peak->size()) ? tp_samples_to_peak->at(k) : 0;
            uint64_t adc_peak = (k < tp_adc_peak->size()) ? tp_adc_peak->at(k) : 0;
            uint64_t adc_int = (k < tp_adc_integral->size()) ? tp_adc_integral->at(k) : 0;

            auto tp = std::make_unique<TriggerPrimitive>(
                TriggerPrimitive::s_trigger_primitive_version,
                0,
                0,
                channel,
                sot,
                tstart,
                s2p,
                adc_int,
                adc_peak
            );
            tp->SetDetector(det);
            tp->SetDetectorChannel(det_ch);
            tp->SetEvent(event);
            tps.push_back(tp.get());
            out.owned_tps.push_back(std::move(tp));
        }

        if (tps.empty()) {
            continue;
        }

        cluster c(tps);
        c.set_true_dir({true_dir_x, true_dir_y, true_dir_z});
        c.set_true_energy(true_neutrino_energy);
        c.set_true_label(true_label ? *true_label : std::string("UNKNOWN"));
        c.set_true_interaction(true_interaction ? *true_interaction : std::string("UNKNOWN"));
        c.set_reco_pos({static_cast<float>(reco_pos_x), static_cast<float>(reco_pos_y), static_cast<float>(reco_pos_z)});
        c.set_min_distance_from_true_pos(min_distance_from_true_pos);
        c.set_supernova_tp_fraction(supernova_tp_fraction);
        c.set_generator_tp_fraction(generator_tp_fraction);

        out.clusters.push_back(std::move(c));
    }

    file->Close();
    delete file;
}

static double cluster_time_start(const cluster& c) {
    auto tps = c.get_tps();
    if (tps.empty() || tps[0] == nullptr) return 0.0;
    return tps[0]->GetTimeStart();
}

int main(int argc, char* argv[]) {
    CmdLineParser clp;

    clp.getDescription() << "> match_clusters app - match U/V/X clusters into multiplane clusters." << std::endl;
    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");

    clp.addDummyOption("Triggers");
    clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");

    clp.addDummyOption();
    LogInfo << clp.getDescription().str() << std::endl;
    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;

    clp.parseCmdLine(argc, argv);
    LogThrowIf(clp.isNoOptionTriggered(), "No option was provided.");

    std::string json = clp.getOptionVal<std::string>("json");
    std::ifstream i(json);
    LogThrowIf(!i.is_open(), "Could not open JSON: " << json);
    nlohmann::json j;
    i >> j;

    std::string outfolder = j.value("output_folder", std::string("data"));
    double radius = j.value("radius", 5.0);

    std::string clusters_file = j.value("clusters_file", std::string(""));
    std::string file_clusters_u = j.value("clusters_u", std::string(""));
    std::string file_clusters_v = j.value("clusters_v", std::string(""));
    std::string file_clusters_x = j.value("clusters_x", std::string(""));

    if (clusters_file.empty() && (file_clusters_u.empty() || file_clusters_v.empty() || file_clusters_x.empty())) {
        LogThrow("Provide clusters_file or clusters_u/v/x in JSON.");
    }

    ClusterBundle u_bundle;
    ClusterBundle v_bundle;
    ClusterBundle x_bundle;

    if (!clusters_file.empty()) {
        load_clusters_for_view(clusters_file, "U", u_bundle);
        load_clusters_for_view(clusters_file, "V", v_bundle);
        load_clusters_for_view(clusters_file, "X", x_bundle);
    } else {
        load_clusters_for_view(file_clusters_u, "U", u_bundle);
        load_clusters_for_view(file_clusters_v, "V", v_bundle);
        load_clusters_for_view(file_clusters_x, "X", x_bundle);
    }

    auto& clusters_u = u_bundle.clusters;
    auto& clusters_v = v_bundle.clusters;
    auto& clusters_x = x_bundle.clusters;

    auto by_time = [](const cluster& a, const cluster& b){ return cluster_time_start(a) < cluster_time_start(b); };
    std::sort(clusters_u.begin(), clusters_u.end(), by_time);
    std::sort(clusters_v.begin(), clusters_v.end(), by_time);
    std::sort(clusters_x.begin(), clusters_x.end(), by_time);

    std::vector<cluster> multiplane_clusters;
    std::vector<std::vector<cluster>> matched_triplets;

    int start_j = 0;
    int start_k = 0;
    std::clock_t start = std::clock();

    for (int i = 0; i < static_cast<int>(clusters_x.size()); i++) {
        if (i % 10000 == 0 && i != 0) {
            std::clock_t now = std::clock();
            std::cout << "Cluster " << i << " Time: " << (now - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
            std::cout << "Number of compatible clusters: " << multiplane_clusters.size() << std::endl;
        }

        double tx = cluster_time_start(clusters_x[i]);
        int min_range_j = 0;
        int max_range_j = clusters_u.size();
        while (min_range_j < max_range_j) {
            start_j = (min_range_j + max_range_j) / 2;
            if (cluster_time_start(clusters_u[start_j]) < tx) {
                min_range_j = start_j + 1;
            } else {
                max_range_j = start_j;
            }
        }
        start_j = std::max(start_j - 10, 0);

        for (int j = start_j; j < static_cast<int>(clusters_u.size()); j++) {
            double tu = cluster_time_start(clusters_u[j]);
            if (tu > tx + 5000) break;
            if (tu < tx - 5000) continue;

            auto* tp_u = clusters_u[j].get_tps().empty() ? nullptr : clusters_u[j].get_tps()[0];
            auto* tp_x = clusters_x[i].get_tps().empty() ? nullptr : clusters_x[i].get_tps()[0];
            if (!tp_u || !tp_x) continue;
            if (tp_u->GetDetector() != tp_x->GetDetector()) continue;

            int min_range_k = 0;
            int max_range_k = clusters_v.size();
            while (min_range_k < max_range_k) {
                start_k = (min_range_k + max_range_k) / 2;
                if (cluster_time_start(clusters_v[start_k]) < tu) {
                    min_range_k = start_k + 1;
                } else {
                    max_range_k = start_k;
                }
            }
            start_k = std::max(start_k - 10, 0);

            for (int k = start_k; k < static_cast<int>(clusters_v.size()); k++) {
                double tv = cluster_time_start(clusters_v[k]);
                if (tv > tu + 5000) break;
                if (tv < tu - 5000) continue;

                auto* tp_v = clusters_v[k].get_tps().empty() ? nullptr : clusters_v[k].get_tps()[0];
                if (!tp_v) continue;
                if (tp_v->GetDetector() != tp_x->GetDetector()) continue;

                if (are_compatibles(clusters_u[j], clusters_v[k], clusters_x[i], radius)) {
                    matched_triplets.push_back({clusters_u[j], clusters_v[k], clusters_x[i]});
                    cluster c = join_clusters(clusters_u[j], clusters_v[k], clusters_x[i]);
                    multiplane_clusters.push_back(c);
                }
            }
        }
    }

    std::cout << "Number of compatible clusters: " << matched_triplets.size() << std::endl;

    std::filesystem::create_directories(outfolder);
    std::string out_file = outfolder;
    if (!out_file.empty() && out_file.back() != '/') out_file += "/";
    out_file += "multiplane_clusters.root";

    write_clusters_to_root(multiplane_clusters, out_file, "M");
    std::cout << "multiplane clusters written to " << out_file << std::endl;

    std::clock_t end = std::clock();
    std::cout << "Time: " << (end - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    return 0;
}
