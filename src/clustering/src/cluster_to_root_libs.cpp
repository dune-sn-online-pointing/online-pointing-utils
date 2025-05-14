#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <climits>

// #include <TLeaf.h>

#include "cluster_to_root_libs.h"
#include "cluster.h"
#include "TriggerPrimitive.hpp"
// #include "position_calculator.h"

#include "Logger.h"
#include "GenericToolbox.Utils.h"

LoggerInit([]{
    Logger::getUserHeader() << "[" << FILENAME << "]";
});

// read the tps from the files and save them in a vector
void file_reader(std::vector<std::string> filenames, std::vector<TriggerPrimitive>& tps, std::vector <TrueParticle> &true_particles, std::vector <Neutrino>& neutrinos, int supernova_option, int max_events_per_filename) {
    std::string line;
    int n_events_offset = 0;
    int file_idx = 0;
    // std::vector<TriggerPrimitive> tps;
    
    for (auto& filename : filenames) {

        LogInfo << " Reading file: " << filename << std::endl;
        
        TFile *file = TFile::Open(filename.c_str());
        if (!file || file->IsZombie()) {
            LogError << " Error opening file: " << filename << std::endl;
            return;
        }
        
        std::string this_interaction = "UNKNOWN";
        // if in the path there is _es_, then se interaction_type to "ES", same with _cc_ and "CC"
        // this is maybe not the best way to  do it, might find another way from metadata
        if (filename.find("_es_") != std::string::npos || filename.find("_ES_") != std::string::npos) {
            this_interaction = "ES";
        } else if (filename.find("_cc_") != std::string::npos || filename.find("_CC_") != std::string::npos) {
            this_interaction = "CC";
        } else {
            this_interaction = "UNKNOWN"; // not sure TODO
        }

        LogInfo << " For this file, interaction type: " << this_interaction << std::endl;

        std::string TPtree_path = "triggerAnaDumpTPs/TriggerPrimitives/tpmakerTPC__TriggerAnaTree1x2x2"; // TODO make flexible for 1x2x6 and maybe else
        TTree *TPtree = dynamic_cast<TTree*>(file->Get(TPtree_path.c_str()));
        if (!TPtree) {
            LogError << " Tree not found: " << TPtree_path << std::endl;
            continue; // can still go to next file
        }
        
        
        tps.reserve(TPtree->GetEntries());  

        UShort_t this_version = 0;
        uint64_t this_time_start = 0;
        Int_t this_channel = 0;
        UInt_t this_adc_integral = 0;
        UShort_t this_adc_peak = 0;
        UShort_t this_detid = 0;
        int this_event_number = 0;
        TPtree->SetBranchAddress("version", &this_version);
        TPtree->SetBranchAddress("time_start", &this_time_start);
        TPtree->SetBranchAddress("channel", &this_channel);
        TPtree->SetBranchAddress("adc_integral", &this_adc_integral);
        TPtree->SetBranchAddress("adc_peak", &this_adc_peak);
        TPtree->SetBranchAddress("detid", &this_detid);
        TPtree->SetBranchAddress("Event", &this_event_number);
        
        // For version 1
        uint64_t this_time_over_threshold = 0;
        uint64_t this_time_peak = 0;
        TPtree->SetBranchAddress("time_over_threshold", &this_time_over_threshold);
        TPtree->SetBranchAddress("time_peak", &this_time_peak);

        // For version 2
        uint64_t this_samples_over_threshold = 0;
        uint64_t this_samples_to_peak = 0;
        TPtree->SetBranchAddress("samples_over_threshold", &this_samples_over_threshold);
        TPtree->SetBranchAddress("samples_to_peak", &this_samples_to_peak);
        
        LogInfo << "  If the TOT or peak branches are not found, it is because the TPs are not that version" << std::endl;

        LogInfo << " Reading tree of TriggerPrimitives" << std::endl;

        // Loop over the entries in the tree
        for (Long64_t i = 0; i < TPtree->GetEntries(); ++i) {
            TPtree->GetEntry(i);

            // in v1 the variables were different, so adapt to version 2
            if (this_version == 1){
                int clock_to_TPC_ticks = 32; // TODO move to namespace
                this_samples_over_threshold = (this_time_over_threshold / clock_to_TPC_ticks); // converting from clock to TPC ticks TODO better
                this_samples_to_peak = (this_time_peak - this_time_start ) / clock_to_TPC_ticks;    // converting from clock to TPC ticks TODO better
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
            this_tp.SetEvent(this_event_number);
            
            tps.emplace_back(this_tp); // add to collection of TPs
        }
        
        if (tps.size() > 0)
            LogInfo << " Found " << tps.size() << " TPs in file " << filename << std::endl;
        else 
            LogWarning << " Found no TPs in file " << filename << std::endl;

        ////////////////////////////////////////////////////////////////////////////////////////////////
        // Read mcparticles (geant)

        std::string MCparticlestree_path = "triggerAnaDumpTPs/mcparticles";
        TTree *MCparticlestree = dynamic_cast<TTree*>(file->Get(MCparticlestree_path.c_str()));
        if (!MCparticlestree) {
            LogError << "Tree not found: " << MCparticlestree_path << std::endl;
            return;
        }

        // we will have as many true particles as entries in this tree, 
        // including both geant and gen particles
        true_particles.reserve(MCparticlestree->GetEntries());

        std::map<int, std::vector<int>> truthId_to_mcparticleIdx;
        std::string* generator_name = new std::string();
        
        // TODO tidy up this mess
        Double_t x = 0.0, y = 0.0, z = 0.0;
        Double_t px = 0.0, py = 0.0, pz = 0.0;
        Double_t energy = 0.0;
        int pdg = 0, event = 0, block_id = 0, track_id = 0;
        int truth_id = 0;

        std::string* process = new std::string();
        MCparticlestree->SetBranchAddress("process", &process);
        MCparticlestree->SetBranchAddress("generator_name", &generator_name);
        MCparticlestree->SetBranchAddress("x", &x);
        MCparticlestree->SetBranchAddress("y", &y);
        MCparticlestree->SetBranchAddress("z", &z);
        MCparticlestree->SetBranchAddress("Px", &px);
        MCparticlestree->SetBranchAddress("Py", &py);
        MCparticlestree->SetBranchAddress("Pz", &pz);
        MCparticlestree->SetBranchAddress("en", &energy);
        MCparticlestree->SetBranchAddress("pdg", &pdg);
        MCparticlestree->SetBranchAddress("Event", &event);
        // MCparticlestree->SetBranchAddress("block_id", &block_id);
        MCparticlestree->SetBranchAddress("track_id", &track_id);
        MCparticlestree->SetBranchAddress("truth_id", &truth_id);


        for (Long64_t i = 0; i < MCparticlestree->GetEntries(); ++i) {
            MCparticlestree->GetEntry(i);
            
            true_particles.emplace_back( TrueParticle(
                event,
                x,
                y,
                z,
                px,
                py,
                pz,
                energy*1e3, // converting to MeV
                *generator_name,
                pdg,
                *process,
                track_id,
                truth_id
            ));

            // truthId_to_mcparticleIdx[truth_id].push_back(i);

            // true_particles.back().Print();
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////
        // Read MC truth

        std::string MCtruthtree_path = "triggerAnaDumpTPs/mctruths"; 
        TTree *MCtruthtree = dynamic_cast<TTree*>(file->Get(MCtruthtree_path.c_str()));
        if (!MCtruthtree) {
            LogError << " Tree not found: " << MCtruthtree_path << std::endl;
            return;
        }

        // this is a scope vector, just to put somewhere the generator name before
        // associating to the final true particles 
        std::vector <TrueParticle> mc_true_particles; 
        
        LogInfo << " Reading tree of MCtruths, there are " << MCtruthtree->GetEntries() << " entries" << std::endl;

        MCtruthtree->SetBranchAddress("generator_name", &generator_name);
        MCtruthtree->SetBranchAddress("x", &x);
        MCtruthtree->SetBranchAddress("y", &y);
        MCtruthtree->SetBranchAddress("z", &z);
        MCtruthtree->SetBranchAddress("Px", &px);
        MCtruthtree->SetBranchAddress("Py", &py);
        MCtruthtree->SetBranchAddress("Pz", &pz);
        MCtruthtree->SetBranchAddress("en", &energy);
        MCtruthtree->SetBranchAddress("pdg", &pdg);
        MCtruthtree->SetBranchAddress("Event", &event);
        MCtruthtree->SetBranchAddress("block_id", &block_id);
        
        for (Long64_t i = 0; i < MCtruthtree->GetEntries(); ++i) {
            
            MCtruthtree->GetEntry(i);


            // neutrinos.reserve(MCtruthtree->GetEntries());

            // LogInfo << "  Generator is " << *generator_name << std::endl;
            
            if ( pdg == PDG::nue) { 
                // LogInfo << "This is a nu_e" << std::endl;
                // Add to the vector of Neutrinos
                neutrinos.emplace_back(Neutrino(
                    event,
                    this_interaction,
                    x,
                    y,
                    z,
                    px,
                    py,
                    pz,
                    energy*1e3, // converting to MeV
                    block_id
                ));
                
            }
            else { // particles from neutrino interaction or backgrounds
                // LogInfo << " Found a true particle with generator " << *generator_name << std::endl;
                mc_true_particles.emplace_back(
                    TrueParticle(
                        event,
                        // std::string(static_cast<const char*>(MCtruthtree->GetLeaf("generator_name")->GetValuePointer())),
                        *generator_name,
                        block_id
                    )
                );
            }
        }

        LogInfo << " Loaded all  MC truths, there are " << mc_true_particles.size() << " true particles" << std::endl;
        LogInfo << " Loaded all  neutrinos, there are " << neutrinos.size() << " neutrinos" << std::endl;
        
        
        ///////////////////////////////////////////////////////////////////////////////////////////////
        // Read simides, used to find the channel and time and later associate TPs to MCparticles 

        std::string simidestree_path = "triggerAnaDumpTPs/simides"; 
        TTree *simidestree = dynamic_cast<TTree*>(file->Get(simidestree_path.c_str()));
        if (!simidestree) {
            LogError << "Tree not found: " << simidestree_path << std::endl;
            return;
        }   

        Int_t event_number_simides;
        UInt_t ChannelID;
        UShort_t Timestamp;
        Int_t trackID;

        simidestree->SetBranchAddress("Event", &event_number_simides);
        simidestree->SetBranchAddress("ChannelID", &ChannelID);
        simidestree->SetBranchAddress("Timestamp", &Timestamp);
        simidestree->SetBranchAddress("trackID", &trackID);

        LogInfo << " Reading tree of SimIDEs to find channels and timestamps associated to MC particles" << std::endl;
        LogInfo << " Number of SimIDEs: " << simidestree->GetEntries() << std::endl;

        int match_count = 0;
        for (Long64_t i = 0; i < simidestree->GetEntries(); ++i) {
            simidestree->GetEntry(i);
            if (i % 10000 == 0)
                GenericToolbox::displayProgressBar(i, simidestree->GetEntries(), "Matching SimIDEs to true particles...");

            // find the true particle with the same trackID and add the timestamp as min(timestamp, time_start)
            // and max(timestamp, time_end)
            bool found = false;
            for (auto& particle : true_particles) {
                if (particle.GetEvent() == event_number_simides 
                    && particle.GetTrackId() == trackID) 
                {
                    // LogInfo << "Track id " << trackID << " found in true particle having trackID " << particle.GetTrackId() << std::endl;
                    // LogInfo << "  ChannelID: " << ChannelID << std::endl;
                    // LogInfo << "  Timestamp: " << Timestamp << std::endl;
                    particle.SetTimeStart(std::min(particle.GetTimeStart(), (double)Timestamp));
                    particle.SetTimeEnd(std::max(particle.GetTimeEnd(), (double)Timestamp));
                    // Add the channels to the list of channels in the true particle
                    particle.AddChannel(ChannelID);
                    found = true;
                    match_count++;
                    break;
                }
            }
            if (!found) {
                // LogWarning << "TrackID " << trackID << " not found in MC particles." << std::endl;
            }
        } // end of simides, not used anywhere anymore

        LogInfo << " Matched " << float(match_count)/simidestree->GetEntries()*100. << "% SimIDEs to true particles" << std::endl;

        file->Close(); // don't need anymore

        /////////////////////////////////////////////////////////////////////////////////////////////////
        // Connect trueparticles to mctruths using the truth_id

        LogInfo << " Connecting MC particles to mctruths, there are " << true_particles.size() << " particles" << std::endl;
        LogInfo << " There are " << neutrinos.size() << " neutrinos" << std::endl;
        
        for (auto& particle : true_particles) {    
            // if a particle  is associated to a neutrino, add the neutrino to the particle
            bool found = false;
            for (auto& neutrino : neutrinos) {
                if (neutrino.GetEvent() == particle.GetEvent() 
                && neutrino.GetTruthId() == particle.GetTruthId()) 
                {
                    // LogInfo << " This particle comes from a neutrino" << std::endl;
                    particle.SetNeutrino(&neutrino);
                    // particle.SetGeneratorName("marley");
                    found = true;
                    break;
                }
            }
            
            // associate MC particles to their MC truth
            for (auto& mc_true_particle : mc_true_particles) {
                if (mc_true_particle.GetEvent() == particle.GetEvent() 
                && mc_true_particle.GetTruthId() == particle.GetTruthId()) 
            {
                    // LogInfo << " Found a match, generator name: " << mc_true_particle.GetGeneratorName() << std::endl;
                    particle.SetGeneratorName(mc_true_particle.GetGeneratorName());
                    particle.SetProcess(mc_true_particle.GetProcess());
                    found = true;
                    // LogInfo << "Found a match for truth ID " << particle.GetTruthId() << std::endl;
                    break;
                }
            }

            if (!found) {
                LogWarning << "TruthID " << particle.GetTruthId() << " not found in MC truths." << std::endl;
            }
        }
    }
    
    // sort the TPs by time
    // C++ 17 has the parameter std::execution::par that handles parallelization, can try that out TODO
    std::clock_t start_sorting = std::clock();
    std::sort(tps.begin(), tps.end(), [](const TriggerPrimitive& a, const TriggerPrimitive& b) {
        return a.GetTimeStart() < b.GetTimeStart();
    });
    std::clock_t end_sorting = std::clock();
    double elapsed_time = double(end_sorting - start_sorting) / CLOCKS_PER_SEC;
    LogInfo << "Sorting TPs took " << elapsed_time << " seconds" << std::endl;
    LogInfo << "or " << elapsed_time/60 << " minutes" << std::endl;

}

// PBC is periodic boundary condition
bool channel_condition_with_pbc(double ch1, double ch2, int channel_limit) {
    if (int(ch1/APA::total_channels) != int(ch2/APA::total_channels)) {
        return false;
    }

    double diff = std::abs(ch1 - ch2);
    int n_chan;
    int mod_ch1 = int(ch1) % APA::total_channels;
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
        // LogInfo << "chan with PCB!" << std::endl;
        // LogInfo << int(ch1) << " " << int(ch2) << std::endl;
        // LogInfo << ch1 << " " << ch2 << std::endl;
        return true;
    }
    return false;
}

// TODO optimize and refactor this function
// TODO add number to save fraction of TPs removed with the cut
std::vector<cluster> cluster_maker(std::vector<TriggerPrimitive*> all_tps, int ticks_limit, int channel_limit, int min_tps_to_cluster, int adc_integral_cut) {
    LogInfo << "Creating clusters from TPs" << std::endl;

    std::vector<std::vector<TriggerPrimitive*>> buffer;
    std::vector<cluster> clusters;

    int last_event_number = -1, first_event_number = 1;
    // Loop over all TPs
    for (auto& tp : all_tps) {

        if (tp->GetEvent() <= first_event_number) 
            first_event_number = tp->GetEvent();

        if (tp->GetEvent() > last_event_number) 
            last_event_number = tp->GetEvent();
    }

    LogInfo << "First event number: " << first_event_number << std::endl;
    LogInfo << "Last event number: " << last_event_number << std::endl;

    // one event at the time, for online can do things differently
    for (int event = first_event_number; event <= last_event_number; event++) {
        LogInfo << "Processing event: " << event << std::endl;

        // Reset the buffer for each event
        buffer.clear();

        for (int iTP = 0; iTP < all_tps.size(); iTP++) {
            TriggerPrimitive* tp = all_tps.at(iTP);
            
            // GenericToolbox::displayProgressBar(iTP, all_tps.size(), "Clustering...");
            
            if (tp->GetEvent() != event) continue;
            
            // TODO add verbode mode
            // LogInfo << "Processing TP: " << tp->GetTimeStart() << " " << tp->GetDetectorChannel() << std::endl;
            // tp->Print();

            bool appended = false;

            for (auto& candidate : buffer) {
                // Calculate the maximum time in the current candidate
                double max_time = 0;
                for (auto& tp2 : candidate) {
                    max_time = std::max(max_time, tp2->GetTimeStart() + tp2->GetSamplesOverThreshold() * TPC_sample_length);
                }

                // Check time and channel conditions
                if ((tp->GetTimeStart() - max_time) <= ticks_limit) {
                    for (auto& tp2 : candidate) {
                        if (channel_condition_with_pbc(tp->GetDetectorChannel(), tp2->GetDetectorChannel(), channel_limit)) {
                            candidate.push_back(tp);
                            appended = true;
                            // LogInfo << "Appended TP to candidate cluster" << std::endl;
                            break;
                        }
                    }
                    if (appended) break;
                    // LogInfo << "Not appended to candidate cluster" << std::endl;
                }
            }

            // If not appended to any candidate, create a new cluster in the buffer
            if (!appended) {
                // LogInfo << "Creating new candidate cluster" << std::endl;
                buffer.push_back({tp});
            }
        }

        // Process remaining candidates in the buffer
        for (auto& candidate : buffer) {
            if (candidate.size() >= min_tps_to_cluster) {
                int adc_integral = 0;
                for (auto& tp : candidate) {
                    adc_integral += tp->GetAdcIntegral();
                }
                if (adc_integral > adc_integral_cut) {
                    // check validity of tps in candidate
                    // LogInfo << "Candidate cluster has " << candidate.size() << " TPs" << std::endl;
                    clusters.emplace_back(cluster(std::move(candidate)));
                    // LogInfo << "Cluster created with " << clusters.back().get_tps().size() << " TPs" << std::endl;
                }
                else {
                }
            }
        }

        LogInfo << "Finished clustering. Number of clusters: " << clusters.size() << std::endl;
    }

    return clusters;
}

// create a map connectig the file index to the true x y z TODO remove
// std::map<int, std::vector<float>> file_idx_to_true_xyz(std::vector<std::string> filenames) {
//     std::map<int, std::vector<float>> file_idx_to_true_xyz;
//     std::string line;
//     float true_x;
//     float true_y;
//     float true_z;
//     int n_events_offset = 0;
//     int file_idx = 0;
//     for (auto& filename : filenames) {
//         std::ifstream infile(filename);
//         // get the number between the last underscore and the .txt extension
//         // Find the last underscore
//         size_t lastUnderscorePos = filename.rfind('_');

//         // Find the position of ".txt"
//         size_t txtExtensionPos = filename.find(".txt");
//         std::string new_filename = "";
//         // Extract the number between the last underscore and ".txt"
//         if (lastUnderscorePos != std::string::npos && txtExtensionPos != std::string::npos) {
//             std::string numberStr = filename.substr(lastUnderscorePos + 1, txtExtensionPos - lastUnderscorePos - 1);

//         // split the filename by / and change the last element to this_custom_direction.txt
//         std::vector<std::string> split_filename;
//         std::string delimiter = "/";
//         size_t pos = 0;
//         std::string token;
//         while ((pos = filename.find(delimiter)) != std::string::npos) {
//             token = filename.substr(0, pos);
//             split_filename.push_back(token);
//             filename.erase(0, pos + delimiter.length());
//         }
//         split_filename.push_back("customDirection_" + numberStr + ".txt");
        
//         for (auto& split : split_filename) {
//             new_filename += split + "/";
//         }
//         new_filename.pop_back();
//         // LogInfo << new_filename << std::endl;
//         // check if the file exists      
//         }
//         else{
//             LogError << "Could not find underscore or .txt extension in the given string." << std::endl;
//         }
//         infile = std::ifstream(new_filename);
//         if (!infile.good()) {
//             LogInfo<<new_filename<<std::endl;
//             LogInfo << "Direction file does not exist" << std::endl;
//             file_idx_to_true_xyz[file_idx] = {0, 0, 0};
//         }
//         else {
//         // This is to account for the fact that the file might have more than 3 lines
//         std::vector<float> all_directions;
//         while (std::getline(infile, line)) {
//             std::istringstream iss(line);
//             iss = std::istringstream(line);
//             float val;
//             iss >> val;
//             all_directions.push_back(val);
//         }
//         true_x = all_directions[all_directions.size()-3];
//         true_y = all_directions[all_directions.size()-2];
//         true_z = all_directions[all_directions.size()-1];

//         // LogInfo << true_x << " " << true_y << " " << true_z << std::endl;
//         file_idx_to_true_xyz[file_idx] = {true_x, true_y, true_z};
//         }
//         ++file_idx;
//     }

//     return file_idx_to_true_xyz;
// }

std::vector<cluster> filter_main_tracks(std::vector<cluster>& clusters) { // valid only if the clusters are ordered by event and for clean sn data
    int best_idx = INT_MAX;

    std::vector<cluster> main_tracks;
    int event = clusters[0].get_tp(0)->GetEvent();

    for (int index = 0; index < clusters.size(); index++) {
        if (clusters[index].get_tp(0)->GetEvent() != event) {
            if (best_idx < clusters.size() ){
                if (clusters[best_idx].get_min_distance_from_true_pos() < 5) {
                    main_tracks.push_back(clusters[best_idx]);
                }
            }

            event = clusters[index].get_tp(0)->GetEvent();
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
    int event = clusters[0].get_tp(0)->GetEvent();

    for (int index = 0; index < clusters.size(); index++) {
        if (clusters[index].get_tp(0)->GetEvent() != event) {
            if (best_idx < clusters.size() ){
                if (clusters[best_idx].get_min_distance_from_true_pos() < 5) {
                    bad_idx_list.push_back(best_idx);                    
                }
            }

            event = clusters[index].get_tp(0)->GetEvent();
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

// void assing_different_label_to_main_tracks(std::vector<cluster>& clusters, int new_label) {
//     int best_idx = INT_MAX;
//     int event = clusters[0].get_tp(0)->GetEvent();
//     std::vector<int> bad_event_list;
//     for (int index = 0; index < clusters.size(); index++) {
//         if (clusters[index].get_tp(0)->GetEvent() != event) {
//             if (best_idx < clusters.size() ){
//                 if (clusters[best_idx].get_min_distance_from_true_pos() < 5) {
//                     clusters[best_idx].set_true_label(100+clusters[best_idx].get_true_interaction());
//                 }
//                 else{
//                     bad_event_list.push_back(event);
//                 }
//             }

//             event = clusters[index].get_tp(0)->GetEvent();
//             if (clusters[index].get_true_label() == 1){
//                 best_idx = index;
//             }
//             else {
//                 best_idx = INT_MAX;
//             }
//         }
//         else {
//             if (best_idx < clusters.size() ){
//                 if (clusters[index].get_true_label() == 1 and clusters[index].get_min_distance_from_true_pos() < clusters[best_idx].get_min_distance_from_true_pos()){
//                     best_idx = index;
//                 }
//             }
//             else {
//                 if (clusters[index].get_true_label() == 1){
//                     best_idx = index;
//                 }
//             }
//         }
//     }
//     if (best_idx < clusters.size() ){
//         if (clusters[best_idx].get_min_distance_from_true_pos() < 5) {
//             clusters[best_idx].set_true_label(100+clusters[best_idx].get_true_interaction());
//         }
//         else{
//             bad_event_list.push_back(event);
//         }
//     }

//     LogInfo << "Number of bad events: " << bad_event_list.size() << std::endl;

//     for (int i=0; i<clusters.size(); i++) {
//         if ((std::find(bad_event_list.begin(), bad_event_list.end(), clusters[i].get_tp(0)->GetEvent()) != bad_event_list.end()) and clusters[i].get_true_label() == 1) {
//             clusters[i].set_true_label(new_label);
//         }
//     }

// }

void write_clusters_to_root(std::vector<cluster>& clusters, std::string root_filename) {
    // create folder if it does not exist
    std::string folder = root_filename.substr(0, root_filename.find_last_of("/"));
    std::string command = "mkdir -p " + folder;
    system(command.c_str());
    // create the root file
    TFile *clusters_file = new TFile(root_filename.c_str(), "recreate");
    TTree *clusters_tree = new TTree("clusters", "clusters");
    // prepare objects to save the data
    // std::vector<std::vector<int>> matrix;
    // std::vector<TriggerPrimitive> matrix;
    // int nrows;
    int event;
    float true_dir_x;
    float true_dir_y;
    float true_dir_z;
    float true_energy;
    std::string true_label;
    std::string true_interaction;
    int reco_pos_x; 
    int reco_pos_y;
    int reco_pos_z;
    float min_distance_from_true_pos;
    float supernova_tp_fraction;
    double total_charge;
    double total_energy;    
    int conversion_factor;
    
    // TP information
    std::vector <int> tp_detector_channel;
    std::vector <int> tp_detector;
    std::vector <int> tp_samples_over_threshold;
    std::vector <int> tp_time_start;
    std::vector <int> tp_samples_to_peak;
    std::vector <int> tp_adc_peak;
    std::vector <int> tp_adc_integral;

    // create the branches
    clusters_tree->Branch("event", &event);
    clusters_tree->Branch("true_dir_x", &true_dir_x);
    clusters_tree->Branch("true_dir_y", &true_dir_y);
    clusters_tree->Branch("true_dir_z", &true_dir_z);
    clusters_tree->Branch("true_energy", &true_energy);
    clusters_tree->Branch("true_label", &true_label);
    clusters_tree->Branch("reco_pos_x", &reco_pos_x);
    clusters_tree->Branch("reco_pos_y", &reco_pos_y);
    clusters_tree->Branch("reco_pos_z", &reco_pos_z);
    clusters_tree->Branch("min_distance_from_true_pos", &min_distance_from_true_pos);
    clusters_tree->Branch("supernova_tp_fraction", &supernova_tp_fraction);
    clusters_tree->Branch("true_interaction", &true_interaction);
    clusters_tree->Branch("total_charge", &total_charge);
    clusters_tree->Branch("total_energy", &total_energy);
    clusters_tree->Branch("conversion_factor", &conversion_factor);


    clusters_tree->Branch("tp_detector_channel", &tp_detector_channel);
    clusters_tree->Branch("tp_detector", &tp_detector);
    clusters_tree->Branch("tp_samples_over_threshold", &tp_samples_over_threshold);
    clusters_tree->Branch("tp_samples_to_peak", &tp_samples_to_peak);
    clusters_tree->Branch("tp_time_start", &tp_time_start);
    clusters_tree->Branch("tp_adc_peak", &tp_adc_peak);
    clusters_tree->Branch("tp_adc_integral", &tp_adc_integral);
    

    // fill the tree
    for (auto& g : clusters) {
        event = g.get_event();
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
        total_charge = g.get_total_charge();
        total_energy = g.get_total_energy();
        conversion_factor = adc_to_energy_conversion_factor; // should be in settings, still keep as metadata
        // TODO create different tree for metadata? Currently in filename

        tp_detector_channel.clear();
        tp_detector.clear();
        tp_samples_over_threshold.clear();
        tp_time_start.clear();
        tp_samples_to_peak.clear();
        tp_adc_peak.clear();
        tp_samples_to_peak.clear();
        tp_adc_integral.clear();
        for (auto& tp : g.get_tps()) {
            tp_detector_channel.push_back(tp->GetDetectorChannel());
            tp_detector.push_back(tp->GetDetector());
            tp_samples_over_threshold.push_back(tp->GetSamplesOverThreshold());
            tp_time_start.push_back(tp->GetTimeStart());
            tp_samples_to_peak.push_back(tp->GetSamplesToPeak());
            tp_adc_peak.push_back(tp->GetAdcPeak());
            tp_samples_to_peak.push_back(tp->GetSamplesToPeak());
            tp_adc_integral.push_back(tp->GetAdcIntegral());
        }
        clusters_tree->Fill();
    }
    // write the tree
    clusters_tree->Write();
    clusters_file->Close();

    return;   
}

std::vector<cluster> read_clusters_from_root(std::string root_filename){
    LogInfo << "Reading clusters from: " << root_filename << std::endl;
    std::vector<cluster> clusters;
    TFile *f = new TFile();
    f = TFile::Open(root_filename.c_str());
    // print the list of objects in the file
    f->ls();
    TTree *clusters_tree = (TTree*)f->Get("clusters_tree");
    
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
    clusters_tree->SetBranchAddress("nrows", &nrows);
    clusters_tree->SetBranchAddress("event", &event);
    clusters_tree->SetBranchAddress("true_dir_x", &true_dir_x);
    clusters_tree->SetBranchAddress("true_dir_y", &true_dir_y);
    clusters_tree->SetBranchAddress("true_dir_z", &true_dir_z);
    clusters_tree->SetBranchAddress("true_energy", &true_energy);
    clusters_tree->SetBranchAddress("true_label", &true_label);
    clusters_tree->SetBranchAddress("reco_pos_x", &reco_pos_x);
    clusters_tree->SetBranchAddress("reco_pos_y", &reco_pos_y);
    clusters_tree->SetBranchAddress("reco_pos_z", &reco_pos_z);
    clusters_tree->SetBranchAddress("min_distance_from_true_pos", &min_distance_from_true_pos);
    clusters_tree->SetBranchAddress("supernova_tp_fraction", &supernova_tp_fraction);
    clusters_tree->SetBranchAddress("true_interaction", &true_interaction);
    for (int i = 0; i < clusters_tree->GetEntries(); i++) {
        clusters_tree->GetEntry(i);
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
        if (event_mapping.find(g.get_tp(0)->GetEvent()) == event_mapping.end()) {
            std::vector<cluster> temp;
            temp.push_back(g);
            event_mapping[g.get_tp(0)->GetEvent()] = temp;
        }
        else {
            event_mapping[g.get_tp(0)->GetEvent()].push_back(g);
        }
    }
    return event_mapping;
}

std::map<int, std::vector<TriggerPrimitive>> create_background_event_mapping(std::vector<TriggerPrimitive>& bkg_tps){
    std::map<int, std::vector<TriggerPrimitive>> event_mapping;
    for (auto& tp : bkg_tps) {
    // check if the event is already in the map
        if (event_mapping.find(tp.GetEvent()) == event_mapping.end()) {
            std::vector<TriggerPrimitive> temp;
            temp.push_back(tp);
            event_mapping[tp.GetEvent()] = temp;
        }
        else {
            event_mapping[tp.GetEvent()].push_back(tp);
        }
    }
    return event_mapping;
}

