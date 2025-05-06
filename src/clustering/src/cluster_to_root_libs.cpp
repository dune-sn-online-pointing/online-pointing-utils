#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <climits>

#include "cluster_to_root_libs.h"
#include "cluster.h"
#include "TriggerPrimitive.hpp"
// #include "position_calculator.h"

#include "Logger.h"

LoggerInit([]{
    Logger::getUserHeader() << "[" << FILENAME << "]";
});

// read the tps from the files and save them in a vector
std::vector<TriggerPrimitive> file_reader(std::vector<std::string> filenames, int plane, int supernova_option, int max_events_per_filename) {
    std::string line;
    int n_events_offset = 0;
    int file_idx = 0;
    std::vector<TriggerPrimitive> tps;
    
    for (auto& filename : filenames) {
        // LogInfo << filename << std::endl;
        // std::ifstream infile(filename);
        // read and save the TPs
        
        TFile *file = TFile::Open(filename.c_str());
        if (!file || file->IsZombie()) {
            LogError << "Error opening file: " << filename << std::endl;
            return std::vector<TriggerPrimitive>{};
        }

        std::string TPtree_path = "triggerAnaDumpAll/TriggerPrimitives/tpmakerTPC__TriggerAnaTree1x2x2"; // TODO make flexible for 1x2x6 and maybe else
        TTree *TPtree = dynamic_cast<TTree*>(file->Get(TPtree_path.c_str()));
        if (!TPtree) {
            LogError << "Tree not found: " << TPtree_path << std::endl;
            return std::vector<TriggerPrimitive>{};
        }
        
        tps.reserve(TPtree->GetEntries());

        // Loop over the entries in the tree
        for (Long64_t i = 0; i < TPtree->GetEntries(); ++i) {
            TPtree->GetEntry(i);
        
            // Fill the TriggerPrimitive object with data from the tree
            uint64_t this_version = TPtree->GetLeaf("version")->GetValue(); 
            uint64_t this_time_start = TPtree->GetLeaf("time_start")->GetValue();
            uint64_t this_channel = TPtree->GetLeaf("channel")->GetValue();
            uint64_t this_adc_integral = TPtree->GetLeaf("adc_integral")->GetValue();
            uint64_t this_adc_peak = TPtree->GetLeaf("adc_peak")->GetValue();
            uint64_t this_detid = TPtree->GetLeaf("detid")->GetValue();
            uint64_t this_samples_over_threshold = 0;
            uint64_t this_samples_to_peak = 0;
            if (this_version == 1) {
                this_samples_over_threshold = (TPtree->GetLeaf("time_over_threshold")->GetValue() )/ TPC_sampling_rate;
                this_samples_to_peak = (TPtree->GetLeaf("time_peak")->GetValue() - this_time_start )/ TPC_sampling_rate;
            }
            else if (this_version == 2) {
                this_samples_over_threshold = TPtree->GetLeaf("samples_over_threshold")->GetValue();
                this_samples_to_peak = TPtree->GetLeaf("samples_to_peak")->GetValue();
            }
            // skipping flag (useless), type, and algorithm (dropped from TP v2)
            
            TriggerPrimitive this_tp = TriggerPrimitive(
                this_version,
                0, // flag
                this_detid,
                this_channel,
                this_samples_over_threshold,
                this_time_start,
                this_samples_to_peak,
                this_adc_integral,
                this_adc_peak
            );
            
            // if in the path of the file there is _es_, then se interaction_type to "ES", same with _cc_ and "CC"
            // this is maybe not the best way to  do it, might find another way from metadata
            if      (filename.find("_es_") != std::string::npos) {this_tp.interaction_type = "ES";}
            else if (filename.find("_cc_") != std::string::npos) {this_tp.interaction_type = "CC";} 
            else                                                 {this_tp.interaction_type = "UNKNOWN";}

            tps.emplace_back(this_tp); // add to collection of TPs
        }
        
        file->Close();

        // while (std::getline(infile, line)) {

            // std::istringstream iss(line);
            // TriggerPrimitive tp;
            // double val;
            // while (iss >> val) {
            //     tp.push_back(val);
            // }
            // tp.push_back(file_idx);
            // if (tp.event>max_events_per_filename) {
            //     break;
            // }

            // if (tp[variables_to_index["view"]] != plane) {
            //     continue;
            // }

            // if (supernova_option == 1 && tp[variables_to_index["ptype"]] == 1) {
            //     tp.event += n_events_offset;
            //     tp[variables_to_index["time_start"]] += EVENTS_OFFSET*tp.event;
            //     tp[variables_to_index["time_peak"]] += EVENTS_OFFSET*tp.event;
            //     tps.push_back(tp);
            // }   
            // else if (supernova_option == 2 && tp[variables_to_index["ptype"]] != 1) {
            //     tp.event += n_events_offset;
            //     tp[variables_to_index["time_start"]] += EVENTS_OFFSET*tp.event;
            //     tp[variables_to_index["time_peak"]] += EVENTS_OFFSET*tp.event;
            //     tps.push_back(tp);
            // }
            // else if (supernova_option == 0) {
            //     tp.event += n_events_offset;
            //     tp[variables_to_index["time_start"]] += EVENTS_OFFSET*tp.event;       
            //     tp[variables_to_index["time_peak"]] += EVENTS_OFFSET*tp.event;
            //     tps.push_back(tp);
            // }
        // }
        if (tps.size() > 0){
            // if (tps[tps.size()-1].event != n_events_offset) {
            //     // LogInfo << "File Works" << std::endl;
            // }
            // else {
            //     LogInfo << filename << std::endl;
            //     LogInfo << "File does not work" << std::endl;
            // }
            // n_events_offset = tps[tps.size()-1].event;

            LogInfo << "Found " << tps.size() << " TPs in file " << filename << std::endl;
        }
        else {
            LogWarning << "Found no TPs in file " << filename << std::endl;
        }
        // ++file_idx;
    }
    
    // sort the TPs by time
    // C++ 17 has the parameter std::execution::par that handles parallelization, can try that out TODO
    std::clock_t start_sorting = std::clock();
    std::sort(tps.begin(), tps.end(), [](const TriggerPrimitive& a, const TriggerPrimitive& b) {
        return a.time_start < b.time_start;
    });
    std::clock_t end_sorting = std::clock();
    double elapsed_time = double(end_sorting - start_sorting) / CLOCKS_PER_SEC;
    LogInfo << "Sorting TPs took " << elapsed_time << " seconds" << std::endl;
    LogInfo << "or " << elapsed_time/60 << " minutes" << std::endl;

    return tps;
}

// std::vector<std::vector<TriggerPrimitive>> file_reader_all_planes(std::vector<std::string> filenames, int supernova_option, int max_events_per_filename) {
//     std::vector<std::vector<TriggerPrimitive>> tps;
//     // add three empty vectors for the three planes
//     tps.push_back(std::vector<TriggerPrimitive>());
//     tps.push_back(std::vector<TriggerPrimitive>());
//     tps.push_back(std::vector<TriggerPrimitive>());
    
//     std::string line;
//     int n_events_offset = 0;
//     int file_idx = 0;
//     for (auto& filename : filenames) {
//         // LogInfo << filename << std::endl;
//         std::ifstream infile(filename);
//         // read and save the TPs
//         while (std::getline(infile, line)) {

//             std::istringstream iss(line);
//             TriggerPrimitive tp;
//             double val;
//             while (iss >> val) {
//                 tp.push_back(val);
//             }
//             tp.push_back(file_idx);
//             if (tp.event>max_events_per_filename) {
//                 break;
//             }


//             if (supernova_option == 1 && tp[variables_to_index["ptype"]] == 1) {
//                 tp.event += n_events_offset;
//                 tp[variables_to_index["time_start"]] += EVENTS_OFFSET*tp.event;
//                 tp[variables_to_index["time_peak"]] += EVENTS_OFFSET*tp.event;
//                 tps[tp[variables_to_index["view"]]].push_back(tp);
//             }   
//             else if (supernova_option == 2 && tp[variables_to_index["ptype"]] != 1) {
//                 tp.event += n_events_offset;
//                 tp[variables_to_index["time_start"]] += EVENTS_OFFSET*tp.event;
//                 tp[variables_to_index["time_peak"]] += EVENTS_OFFSET*tp.event;
//                 tps[tp[variables_to_index["view"]]].push_back(tp);

//             }
//             else if (supernova_option == 0) {
//                 tp.event += n_events_offset;
//                 tp[variables_to_index["time_start"]] += EVENTS_OFFSET*tp.event;       
//                 tp[variables_to_index["time_peak"]] += EVENTS_OFFSET*tp.event;
//                 tps[tp[variables_to_index["view"]]].push_back(tp);
//             }
//         }
//         if (tps.size() > 0){
//             if (tps[0][tps[0].size()-1].event != n_events_offset) {
//                 // LogInfo << "File Works" << std::endl;
//             }
//             else {
//                 LogError << "Considering the event offset, no TPs found in " << filename << std::endl;
//             }
//             n_events_offset = std::max(tps[0][tps[0].size()-1].event, std::max(tps[1][tps[1].size()-1].event, tps[2][tps[2].size()-1].event));
//         }
//         else {
//             LogError << "No TPs found in " << filename << std::endl;
//         }
//         ++file_idx;
//     }
//         // sort the TPs by time
//     for (int i = 0; i < 3; i++) {
//         std::sort(tps[i].begin(), tps[i].end(), [](const TriggerPrimitive& a, const TriggerPrimitive& b) {
//             return a[0] < b[0];
//         });
//     }

//     return tps;
// }

// PBC is periodic boundary condition
bool channel_condition_with_pbc(double ch1, double ch2, int channel_limit) {
    if (int(ch1/2560) != int(ch2/2560)) {
        return false;
    }

    double diff = std::abs(ch1 - ch2);
    int n_chan;
    int mod_ch1 = int(ch1) % 2560;
    if (mod_ch1 >= 0 and mod_ch1 <800) {
        n_chan = 800;
    }
    else if (mod_ch1 >= 800 and mod_ch1 < 1600) {
        n_chan = 800;
    }
    else {
        n_chan = 960;
        if (diff <= channel_limit) {
            // LogInfo << "chan" << std::endl;   
            return true;
        } else{
            return false;
        }
    }

    if (diff <= channel_limit) {
        // LogInfo << "chan" << std::endl;   
        return true;
    }
    else if (diff >= n_chan - channel_limit) {
        LogInfo << "chan with PCB!" << std::endl;
        LogInfo << int(ch1) << " " << int(ch2) << std::endl;
        // LogInfo << ch1 << " " << ch2 << std::endl;
        return true;
    }
    return false;
}

// TODO optimize and refactor this function
std::vector<cluster> cluster_maker(std::vector<TriggerPrimitive*> all_tps, int ticks_limit, int channel_limit, int min_tps_to_cluster, int adc_integral_cut) {
    std::vector<std::vector<TriggerPrimitive*>> buffer;
    std::vector<cluster> clusters;
    for (auto& tp : all_tps) {
        if (buffer.size() == 0) {
            std::vector<TriggerPrimitive*> temp;
            temp.push_back(tp);
            buffer.push_back(temp);
        }
        else {
            std::vector<std::vector<TriggerPrimitive*>> buffer_copy = buffer;
            buffer.clear();
            bool appended = false;
            int idx = 0;
            int idx_appended;
            for (auto& candidate : buffer_copy) {
                // get a the max containing the times of the TPs in the candidate
                float max_time = 0;
                for (auto& tp2 : candidate) {
                    max_time = std::max(max_time,(tp2->time_start + tp2->samples_over_threshold * TPC_sampling_rate));
                }
                bool time_cond = (tp->time_start - max_time) <= ticks_limit;
                if (time_cond) {
                    bool chan_cond = false;
                    for (auto& tp2 : candidate) {
                        if (channel_condition_with_pbc(tp->channel, tp2->channel, channel_limit)) {
                        // if (std::abs(tp->channel - tp2.channel) <= channel_limit) {
                            chan_cond = true;
                            break;
                        }
                    }
                    if (chan_cond) {
                        // LogInfo << "chan" << std::endl;
                        if (!appended) {
                            candidate.push_back(tp);
                            buffer.push_back(candidate);
                            appended = true;
                            idx_appended = idx;
                            // LogInfo << "appended" << std::endl;
                        }
                        else {
                            for (auto& tp2 : candidate) {
                                buffer[idx_appended].push_back(tp2);
                            }
                        }
                    }
                    else {
                        buffer.push_back(candidate);
                        ++idx;
                    }
                }
                else {
                    if (candidate.size() >= min_tps_to_cluster) {
                        int adc_integral = 0;
                        for (auto& tp2 : candidate) {
                            adc_integral += tp2->adc_integral;
                        }
                        if (adc_integral > adc_integral_cut) {
                            cluster g(candidate);
                            clusters.push_back(g);
                        }
                    }
                }
            }
            if (!appended) {
                std::vector<TriggerPrimitive*> temp;
                temp.push_back(tp);
                buffer.push_back(temp);
            }
        }
    }
    if (buffer.size() > 0) {
        for (auto& candidate : buffer) {
            if (candidate.size() >= min_tps_to_cluster) {
                int adc_integral = 0;
                for (auto& tp : candidate) {
                    adc_integral += tp->adc_integral;
                }
                if (adc_integral > adc_integral_cut) {
                    cluster g(candidate);
                    clusters.push_back(g);
                }
            }
        }
    }

    return clusters;
}

// create a map connectig the file index to the true x y z
std::map<int, std::vector<float>> file_idx_to_true_xyz(std::vector<std::string> filenames) {
    std::map<int, std::vector<float>> file_idx_to_true_xyz;
    std::string line;
    float true_x;
    float true_y;
    float true_z;
    int n_events_offset = 0;
    int file_idx = 0;
    for (auto& filename : filenames) {
        std::ifstream infile(filename);
        // get the number between the last underscore and the .txt extension
        // Find the last underscore
        size_t lastUnderscorePos = filename.rfind('_');

        // Find the position of ".txt"
        size_t txtExtensionPos = filename.find(".txt");
        std::string new_filename = "";
        // Extract the number between the last underscore and ".txt"
        if (lastUnderscorePos != std::string::npos && txtExtensionPos != std::string::npos) {
            std::string numberStr = filename.substr(lastUnderscorePos + 1, txtExtensionPos - lastUnderscorePos - 1);

        // split the filename by / and change the last element to this_custom_direction.txt
        std::vector<std::string> split_filename;
        std::string delimiter = "/";
        size_t pos = 0;
        std::string token;
        while ((pos = filename.find(delimiter)) != std::string::npos) {
            token = filename.substr(0, pos);
            split_filename.push_back(token);
            filename.erase(0, pos + delimiter.length());
        }
        split_filename.push_back("customDirection_" + numberStr + ".txt");
        
        for (auto& split : split_filename) {
            new_filename += split + "/";
        }
        new_filename.pop_back();
        // LogInfo << new_filename << std::endl;
        // check if the file exists      
        }
        else{
            LogError << "Could not find underscore or .txt extension in the given string." << std::endl;
        }
        infile = std::ifstream(new_filename);
        if (!infile.good()) {
            LogInfo<<new_filename<<std::endl;
            LogInfo << "Direction file does not exist" << std::endl;
            file_idx_to_true_xyz[file_idx] = {0, 0, 0};
        }
        else {
        // This is to account for the fact that the file might have more than 3 lines
        std::vector<float> all_directions;
        while (std::getline(infile, line)) {
            std::istringstream iss(line);
            iss = std::istringstream(line);
            float val;
            iss >> val;
            all_directions.push_back(val);
        }
        true_x = all_directions[all_directions.size()-3];
        true_y = all_directions[all_directions.size()-2];
        true_z = all_directions[all_directions.size()-1];

        // LogInfo << true_x << " " << true_y << " " << true_z << std::endl;
        file_idx_to_true_xyz[file_idx] = {true_x, true_y, true_z};
        }
        ++file_idx;
    }

    return file_idx_to_true_xyz;
}

// std::map<int, int> file_idx_to_true_interaction(std::vector<std::string> filenames) {
//     std::map<int, int> file_idx_to_true_interaction;
//     std::string line;
//     int n_events_offset = 0;
//     int file_idx = 0;
//     for (auto& filename : filenames) {
//         std::ifstream infile(filename);
//         // check if CC or ES are included into the file name
//         if (filename.find("CC") != std::string::npos) {
//             file_idx_to_true_interaction[file_idx] = 0;
//         }
//         else if (filename.find("ES") != std::string::npos) {
//             file_idx_to_true_interaction[file_idx] = 1;
//         }
//         else {
//             LogInfo << filename << std::endl;
//             LogError << "Could not find CC or ES in the given string." << std::endl;
//             file_idx_to_true_interaction[file_idx] = -1;
//         }
//         ++file_idx;
//     }

//     return file_idx_to_true_interaction;
// }

std::vector<cluster> filter_main_tracks(std::vector<cluster>& clusters) { // valid only if the clusters are ordered by event and for clean sn data
    int best_idx = INT_MAX;

    std::vector<cluster> main_tracks;
    int event = clusters[0].get_tp(0)->event;

    for (int index = 0; index < clusters.size(); index++) {
        if (clusters[index].get_tp(0)->event != event) {
            if (best_idx < clusters.size() ){
                if (clusters[best_idx].get_min_distance_from_true_pos() < 5) {
                    main_tracks.push_back(clusters[best_idx]);
                }
            }

            event = clusters[index].get_tp(0)->event;
            if (clusters[index].get_true_label() == 1){
                best_idx = index;
            }
            else {
                best_idx = INT_MAX;
            }
        }
        else {
            if (best_idx < clusters.size() ){
                if (clusters[index].get_true_label() == 1 and clusters[index].get_min_distance_from_true_pos() < clusters[best_idx].get_min_distance_from_true_pos()){
                    best_idx = index;
                }
            }
            else {
                if (clusters[index].get_true_label() == 1){
                    best_idx = index;
                }
            }
        }
    }
    if (best_idx < clusters.size() ){
        if (clusters[best_idx].get_min_distance_from_true_pos() < 5) {
            main_tracks.push_back(clusters[best_idx]);
        }
    }

    return main_tracks;
}

std::vector<cluster> filter_out_main_track(std::vector<cluster>& clusters) { // valid only if the clusters are ordered by event and for clean sn data
    int best_idx = INT_MAX;
    std::vector<int> bad_idx_list;
    int event = clusters[0].get_tp(0)->event;

    for (int index = 0; index < clusters.size(); index++) {
        if (clusters[index].get_tp(0)->event != event) {
            if (best_idx < clusters.size() ){
                if (clusters[best_idx].get_min_distance_from_true_pos() < 5) {
                    bad_idx_list.push_back(best_idx);                    
                }
            }

            event = clusters[index].get_tp(0)->event;
            if (clusters[index].get_true_label() == 1){
                best_idx = index;
            }
            else {
                best_idx = INT_MAX;
            }
        }
        else {
            if (best_idx < clusters.size() ){
                if (clusters[index].get_true_label() == 1 and clusters[index].get_min_distance_from_true_pos() < clusters[best_idx].get_min_distance_from_true_pos()){
                    best_idx = index;
                }
            }
            else {
                if (clusters[index].get_true_label() == 1){
                    best_idx = index;
                }
            }
        }
    }
    if (best_idx < clusters.size() ){
        if (clusters[best_idx].get_min_distance_from_true_pos() < 5) {
            bad_idx_list.push_back(best_idx);
        }
    }



    std::vector<cluster> blips;

    for (int i=0; i<clusters.size(); i++) {
        if (std::find(bad_idx_list.begin(), bad_idx_list.end(), i) == bad_idx_list.end()) {
            blips.push_back(clusters[i]);
        }
    }
    
    LogInfo << "Number of blips: " << blips.size() << std::endl;
    return blips;
}

void assing_different_label_to_main_tracks(std::vector<cluster>& clusters, int new_label) {
    int best_idx = INT_MAX;
    int event = clusters[0].get_tp(0)->event;
    std::vector<int> bad_event_list;
    for (int index = 0; index < clusters.size(); index++) {
        if (clusters[index].get_tp(0)->event != event) {
            if (best_idx < clusters.size() ){
                if (clusters[best_idx].get_min_distance_from_true_pos() < 5) {
                    clusters[best_idx].set_true_label(100+clusters[best_idx].get_true_interaction());
                }
                else{
                    bad_event_list.push_back(event);
                }
            }

            event = clusters[index].get_tp(0)->event;
            if (clusters[index].get_true_label() == 1){
                best_idx = index;
            }
            else {
                best_idx = INT_MAX;
            }
        }
        else {
            if (best_idx < clusters.size() ){
                if (clusters[index].get_true_label() == 1 and clusters[index].get_min_distance_from_true_pos() < clusters[best_idx].get_min_distance_from_true_pos()){
                    best_idx = index;
                }
            }
            else {
                if (clusters[index].get_true_label() == 1){
                    best_idx = index;
                }
            }
        }
    }
    if (best_idx < clusters.size() ){
        if (clusters[best_idx].get_min_distance_from_true_pos() < 5) {
            clusters[best_idx].set_true_label(100+clusters[best_idx].get_true_interaction());
        }
        else{
            bad_event_list.push_back(event);
        }
    }

    LogInfo << "Number of bad events: " << bad_event_list.size() << std::endl;

    for (int i=0; i<clusters.size(); i++) {
        if ((std::find(bad_event_list.begin(), bad_event_list.end(), clusters[i].get_tp(0)->event) != bad_event_list.end()) and clusters[i].get_true_label() == 1) {
            clusters[i].set_true_label(new_label);
        }
    }

}

void write_clusters_to_root(std::vector<cluster>& clusters, std::string root_filename) {
    // create folder if it does not exist
    std::string folder = root_filename.substr(0, root_filename.find_last_of("/"));
    std::string command = "mkdir -p " + folder;
    system(command.c_str());
    // create the root file
    TFile *f = new TFile(root_filename.c_str(), "recreate");
    TTree *tree = new TTree("tree", "tree");
    // prepare objects to save the data
    // std::vector<std::vector<int>> matrix;
    // std::vector<TriggerPrimitive> matrix;
    int nrows;
    int event;
    float true_dir_x;
    float true_dir_y;
    float true_dir_z;
    float true_energy;
    int true_label;
    int reco_pos_x; 
    int reco_pos_y;
    int reco_pos_z;
    float min_distance_from_true_pos;
    float supernova_tp_fraction;
    int true_interaction;

    // create the branches
    // tree->Branch("matrix", &matrix);
    tree->Branch("nrows", &nrows);
    tree->Branch("event", &event);
    tree->Branch("true_dir_x", &true_dir_x);
    tree->Branch("true_dir_y", &true_dir_y);
    tree->Branch("true_dir_z", &true_dir_z);
    tree->Branch("true_energy", &true_energy);
    tree->Branch("true_label", &true_label);
    tree->Branch("reco_pos_x", &reco_pos_x);
    tree->Branch("reco_pos_y", &reco_pos_y);
    tree->Branch("reco_pos_z", &reco_pos_z);
    tree->Branch("min_distance_from_true_pos", &min_distance_from_true_pos);
    tree->Branch("supernova_tp_fraction", &supernova_tp_fraction);
    tree->Branch("true_interaction", &true_interaction);

    // fill the tree
    for (auto& g : clusters) {
        // matrix = g.get_tps();
        nrows = g.get_size();
        event = g.get_tp(0)->event;
        true_dir_x = g.get_true_dir()[0];
        true_dir_y = g.get_true_dir()[1];
        true_dir_z = g.get_true_dir()[2];
        true_energy = g.get_true_energy();
        true_label = g.get_true_label();
        reco_pos_x = g.get_reco_pos()[0];
        reco_pos_y = g.get_reco_pos()[1];
        reco_pos_z = g.get_reco_pos()[2];
        min_distance_from_true_pos = g.get_min_distance_from_true_pos();
        supernova_tp_fraction = g.get_supernova_tp_fraction();
        true_interaction = g.get_true_interaction();
        tree->Fill();
    }
    // write the tree
    tree->Write();
    f->Close();

    return;   
}

std::vector<cluster> read_clusters_from_root(std::string root_filename){
    LogInfo << "Reading clusters from: " << root_filename << std::endl;
    std::vector<cluster> clusters;
    TFile *f = new TFile();
    f = TFile::Open(root_filename.c_str());
    // print the list of objects in the file
    f->ls();
    TTree *tree = (TTree*)f->Get("tree");
    
    // std::vector<TriggerPrimitive> matrix;
    // std::vector<TriggerPrimitive>* matrix_ptr = &matrix;

    int nrows;
    int event;
    float true_dir_x;
    float true_dir_y;
    float true_dir_z;
    float true_energy;
    int true_label;
    int reco_pos_x; 
    int reco_pos_y;
    int reco_pos_z;
    float min_distance_from_true_pos;
    float supernova_tp_fraction;
    int true_interaction;
    // tree->SetBranchAddress("matrix", &matrix_ptr);
    // tree->SetBranchAddress("matrix", &matrix);
    tree->SetBranchAddress("nrows", &nrows);
    tree->SetBranchAddress("event", &event);
    tree->SetBranchAddress("true_dir_x", &true_dir_x);
    tree->SetBranchAddress("true_dir_y", &true_dir_y);
    tree->SetBranchAddress("true_dir_z", &true_dir_z);
    tree->SetBranchAddress("true_energy", &true_energy);
    tree->SetBranchAddress("true_label", &true_label);
    tree->SetBranchAddress("reco_pos_x", &reco_pos_x);
    tree->SetBranchAddress("reco_pos_y", &reco_pos_y);
    tree->SetBranchAddress("reco_pos_z", &reco_pos_z);
    tree->SetBranchAddress("min_distance_from_true_pos", &min_distance_from_true_pos);
    tree->SetBranchAddress("supernova_tp_fraction", &supernova_tp_fraction);
    tree->SetBranchAddress("true_interaction", &true_interaction);
    for (int i = 0; i < tree->GetEntries(); i++) {
        tree->GetEntry(i);
        // cluster g(matrix);
        // g.set_true_dir({true_dir_x, true_dir_y, true_dir_z});
        // g.set_true_energy(true_energy);
        // g.set_true_label(true_label);
        // g.set_reco_pos({reco_pos_x, reco_pos_y, reco_pos_z});
        // g.set_min_distance_from_true_pos(min_distance_from_true_pos);
        // g.set_supernova_tp_fraction(supernova_tp_fraction);       
        // g.set_true_interaction(true_interaction);
        // clusters.push_back(g);
    }
    f->Close();
    return clusters;
}

std::map<int, std::vector<cluster>> create_event_mapping(std::vector<cluster>& clusters){
    std::map<int, std::vector<cluster>> event_mapping;
    for (auto& g : clusters) {
    // check if the event is already in the map
        if (event_mapping.find(g.get_tp(0)->event) == event_mapping.end()) {
            std::vector<cluster> temp;
            temp.push_back(g);
            event_mapping[g.get_tp(0)->event] = temp;
        }
        else {
            event_mapping[g.get_tp(0)->event].push_back(g);
        }
    }
    return event_mapping;
}

std::map<int, std::vector<TriggerPrimitive>> create_background_event_mapping(std::vector<TriggerPrimitive>& bkg_tps){
    std::map<int, std::vector<TriggerPrimitive>> event_mapping;
    for (auto& tp : bkg_tps) {
    // check if the event is already in the map
        if (event_mapping.find(tp.event) == event_mapping.end()) {
            std::vector<TriggerPrimitive> temp;
            temp.push_back(tp);
            event_mapping[tp.event] = temp;
        }
        else {
            event_mapping[tp.event].push_back(tp);
        }
    }
    return event_mapping;
}

