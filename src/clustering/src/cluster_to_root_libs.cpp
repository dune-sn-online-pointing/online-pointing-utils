#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <climits>
#include <unordered_map>

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


// TODO change this, one argument should be nentries_event
void get_first_and_last_event(TTree* tree, int* branch_value, int which_event, int& first_entry, int& last_entry) {
    first_entry = -1;
    last_entry = -1;

    // LogInfo << " Looking for event number " << which_event << std::endl;
    for (int iEntry = 0; iEntry < tree->GetEntries(); ++iEntry) {
        tree->GetEntry(iEntry);
        if (*branch_value == which_event) {
            if (first_entry == -1) {
                first_entry = iEntry;
            }
            last_entry = iEntry;
        }
        // case only one event
        if (which_event == 1 && iEntry == tree->GetEntries() - 1) {
            // first_entry = iEntry;
            last_entry = iEntry;
            break;
        }
        // at least one was found, since they are ordered, if number changes, stop
        if ( first_entry != -1 && *branch_value != which_event) {
            last_entry = iEntry - 1;
            break;
        }
    }
}

// read the tps from the files and save them in a vector
void file_reader(std::string filename, std::vector<TriggerPrimitive>& tps, std::vector <TrueParticle> &true_particles, std::vector <Neutrino>& neutrinos, int supernova_option, int event_number) {
    
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
        return; // can still go to next file
    }
    
    int first_tp_entry_in_event = -1;
    int last_tp_entry_in_event = -1;

    UInt_t this_event_number = 0;
    TPtree->SetBranchAddress("Event", &this_event_number);


    get_first_and_last_event(TPtree, (int*)&this_event_number, event_number, first_tp_entry_in_event, last_tp_entry_in_event);

    // LogInfo << "First entry having this event number: " << first_tp_entry_in_event << std::endl;
    // LogInfo << "Last entry having this event number: " << last_tp_entry_in_event << std::endl;
    LogInfo << "Number of TPs in event " << event_number << ": " << last_tp_entry_in_event - first_tp_entry_in_event + 1 << std::endl;

    tps.reserve(last_tp_entry_in_event - first_tp_entry_in_event + 1);

    UShort_t this_version = 0;
    uint64_t this_time_start = 0;
    Int_t this_channel = 0;
    UInt_t this_adc_integral = 0;
    UShort_t this_adc_peak = 0;
    UShort_t this_detid = 0;
    // int this_event_number = 0;
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
    for (Long64_t iTP = first_tp_entry_in_event; iTP <= last_tp_entry_in_event; ++iTP) {
        TPtree->GetEntry(iTP);

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

    // find first and last entry of the event
    int first_mcparticle_entry_in_event = -1;
    int last_mcparticle_entry_in_event = -1;

    UInt_t event = 0;
    MCparticlestree->SetBranchAddress("Event", &event);

    get_first_and_last_event(MCparticlestree, (int*)&event, this_event_number, first_mcparticle_entry_in_event, last_mcparticle_entry_in_event);

    LogInfo << "Number of MC particles in event " << event_number << ": " << last_mcparticle_entry_in_event - first_mcparticle_entry_in_event + 1 << std::endl;

    // we will have as many true particles as entries in this tree, 
    // including both geant and gen particles
    // true_particles.reserve(last_mcparticle_entry_in_event - first_mcparticle_entry_in_event + 1);

    std::map<int, std::vector<int>> truthId_to_mcparticleIdx;
    std::string* generator_name = new std::string();
    
    // TODO tidy up this mess
    Double_t x = 0.0, y = 0.0, z = 0.0;
    Double_t px = 0.0, py = 0.0, pz = 0.0;
    Double_t energy = 0.0;
    int pdg = 0;
    //UInt_t event = 0;
    int block_id = 0, track_id = 0;
    int truth_id = 0;
    int status_code = -1;

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
    MCparticlestree->SetBranchAddress("status_code", &status_code);


    for (Long64_t iMCpart = first_mcparticle_entry_in_event; iMCpart <= last_mcparticle_entry_in_event; ++iMCpart) {
        MCparticlestree->GetEntry(iMCpart);

        if (status_code == 0) continue; // skip initial state particles, not tracked
        if (pdg == PDG::nue) continue; // skip neutrinos, they are in the mctruth tree
        
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

        // LogInfo << "......" << std::endl;
        // true_particles.back().Print();
        // LogInfo << "......" << std::endl;
    }

    LogInfo << " Found " << true_particles.size() << " geant particles in file " << filename << std::endl;

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Read MC truth

    std::string MCtruthtree_path = "triggerAnaDumpTPs/mctruths"; 
    TTree *MCtruthtree = dynamic_cast<TTree*>(file->Get(MCtruthtree_path.c_str()));
    if (!MCtruthtree) {
        LogError << " Tree not found: " << MCtruthtree_path << std::endl;
        return;
    }

    // find first and last entry of the event
    int first_mctruth_entry_in_event = -1;
    int last_mctruth_entry_in_event = -1;

    event = 0; // reuse?
    MCtruthtree->SetBranchAddress("Event", &event);

    get_first_and_last_event(MCtruthtree, (int*)&event, this_event_number, first_mctruth_entry_in_event, last_mctruth_entry_in_event);

    LogInfo << "Number of MC truths in event " << event_number << ": " << last_mctruth_entry_in_event - first_mctruth_entry_in_event + 1 << std::endl;
    // LogInfo << "First entry having this event number: " << first_mctruth_entry_in_event << std::endl;
    // LogInfo << "Last entry having this event number: " << last_mctruth_entry_in_event << std::endl;

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
    MCtruthtree->SetBranchAddress("status_code", &status_code);
    
    for (Long64_t iMCtruth = first_mctruth_entry_in_event; iMCtruth <= last_mctruth_entry_in_event; ++iMCtruth) {
        
        MCtruthtree->GetEntry(iMCtruth);
        // neutrinos.reserve(MCtruthtree->GetEntries());
        // LogInfo << "  Generator is " << *generator_name << std::endl;
        
        if ( pdg == PDG::nue) { 
            // if status code is not 0, it's a final state neutrino
            if (status_code != 0)  continue;

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

            LogInfo << " Neutrino energy is " << energy*1e3 << " MeV" << std::endl;
        }
        else { // particles from neutrino interaction or backgrounds
            
            // if status code is 0, particle is not tracked (initial state)
            if (status_code == 0)  continue;

            // LogInfo << " Found a true particle with generator " << *generator_name << std::endl;
            mc_true_particles.emplace_back(
                TrueParticle(
                    event,
                    // std::string(static_cast<const char*>(MCtruthtree->GetLeaf("generator_name")->GetValuePointer())),
                    *generator_name,
                    block_id
                )
            );
            // LogInfo << "  Status code: " << status_code << " and energy " << energy << std::endl;
        }
    }

    LogInfo << " There are " << mc_true_particles.size() << " true particles" << std::endl;
    LogInfo << " There are " << neutrinos.size() << " neutrinos" << std::endl;
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Read simides, used to find the channel and time and later associate TPs to MCparticles 

    std::string simidestree_path = "triggerAnaDumpTPs/simides"; 
    TTree *simidestree = dynamic_cast<TTree*>(file->Get(simidestree_path.c_str()));
    if (!simidestree) {
        LogError << "Tree not found: " << simidestree_path << std::endl;
        return;
    }   

    // find first and last entry of the event
    int n_simides_in_event = 0;
    int first_simide_entry_in_event = -1;
    int last_simide_entry_in_event = -1;

    UInt_t event_number_simides = 0;
    simidestree->SetBranchAddress("Event", &event_number_simides);

    get_first_and_last_event(simidestree, (int*)&event_number_simides, this_event_number, first_simide_entry_in_event, last_simide_entry_in_event);

    // Int_t event_number_simides;
    UInt_t ChannelID;
    UShort_t Timestamp;
    Int_t trackID;
    float x_simide;

    simidestree->SetBranchAddress("Event", &event_number_simides);
    simidestree->SetBranchAddress("ChannelID", &ChannelID);
    simidestree->SetBranchAddress("Timestamp", &Timestamp);
    simidestree->SetBranchAddress("trackID", &trackID);
    simidestree->SetBranchAddress("x", &x_simide);

    LogInfo << " Reading tree of SimIDEs to find channels and timestamps associated to MC particles" << std::endl;
    LogInfo << " Number of SimIDEs in this event: " << last_simide_entry_in_event - first_simide_entry_in_event + 1 << std::endl;

    // Optimization: build a map for fast lookup of true_particles by (event, trackID)
    struct EventTrackKey {
        int event;
        int trackId;
        bool operator==(const EventTrackKey& other) const {
            return event == other.event && trackId == other.trackId;
        }
    };
    struct EventTrackKeyHash {
        std::size_t operator()(const EventTrackKey& k) const {
            return std::hash<int>()(k.event) ^ (std::hash<int>()(k.trackId) << 1);
        }
    };
    std::unordered_map<EventTrackKey, TrueParticle*, EventTrackKeyHash> particle_map;
    for (auto& particle : true_particles) {
        particle_map[{particle.GetEvent(), std::abs(particle.GetTrackId())}] = &particle;
    }

    int match_count = 0;
    for (Long64_t iSimIde = first_simide_entry_in_event; iSimIde <= last_simide_entry_in_event; ++iSimIde) {
        simidestree->GetEntry(iSimIde);
        EventTrackKey key{static_cast<int>(event_number_simides), std::abs(trackID)};
        auto it = particle_map.find(key);
        if (it != particle_map.end()) {
            auto* particle = it->second;
            particle->SetTimeStart(std::min(particle->GetTimeStart(), (double)Timestamp*conversion_tdc_to_tpc));
            particle->SetTimeEnd(std::max(particle->GetTimeEnd(), (double)Timestamp*conversion_tdc_to_tpc));
            particle->AddChannel(ChannelID);
            match_count++;
        } else {
            LogWarning << "TrackID " << trackID << " not found in MC particles." << std::endl;
        }
    } // end of simides, not used anywhere anymore

    LogInfo << " Matched ";
    std::cout << std::setprecision(2) << std::fixed << float(match_count)/(last_simide_entry_in_event-first_simide_entry_in_event+1)*100. << " %" << " SimIDEs to true particles" << std::endl;

    int truepart_with_simideInfo = 0;
    for (auto& particle : true_particles) {
        if (particle.GetChannels().size() > 0) {
            truepart_with_simideInfo++;
        }
    }

    LogInfo << " Number of geant particles with SimIDEs info: " << float(truepart_with_simideInfo)/true_particles.size()*100. << " %" << std::endl;
    LogInfo << " If not 100%, it's ok. Some particles (nuclei) don't produce SimIDEs" << std::endl;

    file->Close(); // don't need anymore

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Connect trueparticles to mctruths using the truth_id

    LogInfo << " Connecting MC particles to mctruths, there are "<< neutrinos.size() << " neutrinos and " << true_particles.size() << " other particles" << std::endl;
    
    int matched_MCparticles_counter = 0;

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
                matched_MCparticles_counter++;
                // LogInfo << "Found a match for truth ID " << particle.GetTruthId() << std::endl;
                break;
            }
        }

        if (!found) {
            static std::set<int> warned_truth_ids;
            if (warned_truth_ids.find(particle.GetTruthId()) == warned_truth_ids.end()) {
                LogError << "TruthID " << particle.GetTruthId() << " not found in MC truths or neutrinos." << std::endl;
                warned_truth_ids.insert(particle.GetTruthId());
            }
        }
    }

    LogInfo << " Matched MC particles to mctruths: "  << float(matched_MCparticles_counter)/true_particles.size()*100. << " %" << std::endl;
    
    // sort the TPs by time
    // C++ 17 has the parameter std::execution::par that handles parallelization, can try that out TODO
    std::clock_t start_sorting = std::clock();
    std::sort(tps.begin(), tps.end(), [](const TriggerPrimitive& a, const TriggerPrimitive& b) {
        return a.GetTimeStart() < b.GetTimeStart();
    });
    std::clock_t end_sorting = std::clock();
    double elapsed_time = double(end_sorting - start_sorting) / CLOCKS_PER_SEC;
    LogInfo << "Sorting TPs took " << elapsed_time << " seconds" << std::endl;

}

// PBC is periodic boundary condition
bool channel_condition_with_pbc(TriggerPrimitive *tp1, TriggerPrimitive* tp2, int channel_limit) {
    if ( tp1->GetDetector() !=  tp2->GetDetector()
        || tp1->GetView() !=  tp2->GetView())
        return false;

    double diff = std::abs(tp1->GetDetectorChannel() - tp2->GetDetectorChannel());
    int channels_in_this_view = APA::channels_in_view[tp1->GetView()];
    if (diff <= channel_limit) 
        return true;
    
    // only for U and V
    if (tp1->GetView() == "U" || tp1->GetView() == "V")
        if (diff >= channels_in_this_view - channel_limit)
            return true;
    
    
    return false;
}

// this is supposed to do one event at the time
// TODO add number to save fraction of TPs removed with the cut
std::vector<cluster> cluster_maker(std::vector<TriggerPrimitive*> all_tps, int ticks_limit, int channel_limit, int min_tps_to_cluster, int adc_integral_cut) {
    LogInfo << "Creating clusters from TPs" << std::endl;

    std::vector<std::vector<TriggerPrimitive*>> buffer;
    std::vector<cluster> clusters;

    // Reset the buffer for each event
    buffer.clear();

    for (int iTP = 0; iTP < all_tps.size(); iTP++) {
        TriggerPrimitive* tp = all_tps.at(iTP);
        
        // if  (iTP % 100 == 0)
        //     GenericToolbox::displayProgressBar(iTP, all_tps.size(), "Clustering...");
        
        // if (tp->GetEvent() != event) continue;
        
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
                    if (channel_condition_with_pbc(tp, tp2, channel_limit)) {
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

    return clusters;
}


std::vector<cluster> filter_main_tracks(std::vector<cluster>& clusters) { // valid only if the clusters are ordered by event and for clean sn data
    int best_idx = INT_MAX;

    std::vector<cluster> main_tracks;
    UInt_t event = clusters[0].get_tp(0)->GetEvent();

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
    UInt_t event = clusters[0].get_tp(0)->GetEvent();

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


void write_clusters_to_root(std::vector<cluster>& clusters, std::string root_filename, std::string view) {
    // create folder if it does not exist
    std::string folder = root_filename.substr(0, root_filename.find_last_of("/"));
    std::string command = "mkdir -p " + folder;
    system(command.c_str());
    // create the root file if it does not exist, otherwise just update it
    TFile *clusters_file = new TFile(root_filename.c_str(), "UPDATE"); // TODO handle better
    TTree *clusters_tree =  (TTree*)clusters_file->Get(Form("clusters_tree_%s", view.c_str()));

    int event;
    int n_tps;
    float true_dir_x;
    float true_dir_y;
    float true_dir_z;
    float true_neutrino_energy;
    float true_particle_energy;
    std::string true_label;
    std::string *true_label_point = new std::string();
    std::string true_interaction;
    std::string *true_interaction_point = new std::string();
    int reco_pos_x; 
    int reco_pos_y;
    int reco_pos_z;
    float min_distance_from_true_pos;
    float supernova_tp_fraction;
    float generator_tp_fraction;
    double total_charge;
    double total_energy;    
    double conversion_factor;
    
    // TP information
    std::vector<int>* tp_detector_channel = nullptr;
    std::vector<int>* tp_detector = nullptr;
    std::vector<int>* tp_samples_over_threshold = nullptr;
    std::vector<int>* tp_time_start = nullptr;
    std::vector<int>* tp_samples_to_peak = nullptr;
    std::vector<int>* tp_adc_peak = nullptr;
    std::vector<int>* tp_adc_integral = nullptr;

    if (!clusters_tree) {
        clusters_tree = new TTree(Form("clusters_tree_%s", view.c_str()), "Tree of clusters");
        LogInfo << "Tree not found, creating it" << std::endl;
        // create the branches
        clusters_tree->Branch("event", &event, "event/I");
        clusters_tree->Branch("n_tps", &n_tps, "n_tps/I");
        clusters_tree->Branch("true_dir_x", &true_dir_x, "true_dir_x/F");
        clusters_tree->Branch("true_dir_y", &true_dir_y, "true_dir_y/F");
        clusters_tree->Branch("true_dir_z", &true_dir_z, "true_dir_z/F");
        clusters_tree->Branch("true_neutrino_energy", &true_neutrino_energy, "true_neutrino_energy/F");
        clusters_tree->Branch("true_particle_energy", &true_particle_energy, "true_particle_energy/F");
        clusters_tree->Branch("true_label", &true_label);
        clusters_tree->Branch("reco_pos_x", &reco_pos_x, "reco_pos_x/I");
        clusters_tree->Branch("reco_pos_y", &reco_pos_y, "reco_pos_y/I");
        clusters_tree->Branch("reco_pos_z", &reco_pos_z, "reco_pos_z/I");
        clusters_tree->Branch("min_distance_from_true_pos", &min_distance_from_true_pos, "min_distance_from_true_pos/F");
        clusters_tree->Branch("supernova_tp_fraction", &supernova_tp_fraction, "supernova_tp_fraction/F");
        clusters_tree->Branch("generator_tp_fraction", &generator_tp_fraction, "generator_tp_fraction/F");
        clusters_tree->Branch("true_interaction", &true_interaction);
        clusters_tree->Branch("total_charge", &total_charge, "total_charge/D");
        clusters_tree->Branch("total_energy", &total_energy, "total_energy/D");
        clusters_tree->Branch("conversion_factor", &conversion_factor, "conversion_factor/D");

        clusters_tree->Branch("tp_detector_channel", &tp_detector_channel);
        clusters_tree->Branch("tp_detector", &tp_detector);
        clusters_tree->Branch("tp_samples_over_threshold", &tp_samples_over_threshold);
        clusters_tree->Branch("tp_samples_to_peak", &tp_samples_to_peak);
        clusters_tree->Branch("tp_time_start", &tp_time_start);
        clusters_tree->Branch("tp_adc_peak", &tp_adc_peak);
        clusters_tree->Branch("tp_adc_integral", &tp_adc_integral);

        // LogInfo << "Tree created" << std::endl;
    }
    else 
    {   
        // LogInfo << "Tree already exists, updating it" << std::endl;
        // If the tree exists, set the branches to the existing tree
        clusters_tree->SetBranchAddress("event", &event);
        clusters_tree->SetBranchAddress("n_tps", &n_tps);
        clusters_tree->SetBranchAddress("true_dir_x", &true_dir_x);
        clusters_tree->SetBranchAddress("true_dir_y", &true_dir_y);
        clusters_tree->SetBranchAddress("true_dir_z", &true_dir_z);
        clusters_tree->SetBranchAddress("true_neutrino_energy", &true_neutrino_energy);
        clusters_tree->SetBranchAddress("true_particle_energy", &true_particle_energy);
        clusters_tree->SetBranchAddress("true_label", &true_label_point);
        clusters_tree->SetBranchAddress("reco_pos_x", &reco_pos_x);
        clusters_tree->SetBranchAddress("reco_pos_y", &reco_pos_y);
        clusters_tree->SetBranchAddress("reco_pos_z", &reco_pos_z);
        clusters_tree->SetBranchAddress("min_distance_from_true_pos", &min_distance_from_true_pos);
        clusters_tree->SetBranchAddress("supernova_tp_fraction", &supernova_tp_fraction);
        clusters_tree->SetBranchAddress("generator_tp_fraction", &generator_tp_fraction);
        clusters_tree->SetBranchAddress("true_interaction", &true_interaction_point);
        clusters_tree->SetBranchAddress("total_charge", &total_charge);
        clusters_tree->SetBranchAddress("total_energy", &total_energy);
        clusters_tree->SetBranchAddress("conversion_factor", &conversion_factor);

        clusters_tree->SetBranchAddress("tp_detector_channel", &tp_detector_channel);
        clusters_tree->SetBranchAddress("tp_detector", &tp_detector);
        clusters_tree->SetBranchAddress("tp_samples_over_threshold", &tp_samples_over_threshold);
        clusters_tree->SetBranchAddress("tp_samples_to_peak", &tp_samples_to_peak);
        clusters_tree->SetBranchAddress("tp_time_start", &tp_time_start);
        clusters_tree->SetBranchAddress("tp_adc_peak", &tp_adc_peak);
        clusters_tree->SetBranchAddress("tp_adc_integral", &tp_adc_integral);
    }
    

    // fill the tree
    for (auto& cluster : clusters) {
        event = cluster.get_event();
        n_tps = cluster.get_size();
        true_dir_x = cluster.get_true_dir()[0];
        true_dir_y = cluster.get_true_dir()[1];
        true_dir_z = cluster.get_true_dir()[2];
        true_neutrino_energy = cluster.get_true_neutrino_energy();
        true_particle_energy = cluster.get_true_particle_energy();
        true_label = cluster.get_true_label();
        true_label_point = &true_label;
        reco_pos_x = cluster.get_reco_pos()[0];
        reco_pos_y = cluster.get_reco_pos()[1];
        reco_pos_z = cluster.get_reco_pos()[2];
        min_distance_from_true_pos = cluster.get_min_distance_from_true_pos();
        supernova_tp_fraction = cluster.get_supernova_tp_fraction();
        generator_tp_fraction = cluster.get_generator_tp_fraction();
        true_interaction = cluster.get_true_interaction();
        true_interaction_point = &true_interaction;
        total_charge = cluster.get_total_charge();
        total_energy = cluster.get_total_energy();
        conversion_factor = adc_to_energy_conversion_factor; // should be in settings, still keep as metadata
        // TODO create different tree for metadata? Currently in filename

        tp_detector_channel->clear();
        tp_detector->clear();
        tp_samples_over_threshold->clear();
        tp_time_start->clear();
        tp_samples_to_peak->clear();
        tp_adc_peak->clear();
        tp_samples_to_peak->clear();
        tp_adc_integral->clear();
        for (auto& tp : cluster.get_tps()) {
            tp_detector_channel->push_back(tp->GetDetectorChannel());
            tp_detector->push_back(tp->GetDetector());
            tp_samples_over_threshold->push_back(tp->GetSamplesOverThreshold());
            tp_time_start->push_back(tp->GetTimeStart());
            tp_samples_to_peak->push_back(tp->GetSamplesToPeak());
            tp_adc_peak->push_back(tp->GetAdcPeak());
            tp_samples_to_peak->push_back(tp->GetSamplesToPeak());
            tp_adc_integral->push_back(tp->GetAdcIntegral());
        }
        clusters_tree->Fill();
    }
    // write the tree
    clusters_tree->Write("", TObject::kOverwrite);
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
    UInt_t event;
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
    float generator_tp_fraction;
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
    clusters_tree->SetBranchAddress("generator_tp_fraction", &generator_tp_fraction);
    clusters_tree->SetBranchAddress("true_interaction", &true_interaction);
    for (int i = 0; i < clusters_tree->GetEntries(); i++) {
        clusters_tree->GetEntry(i);
        // cluster g(matrix);
        // cluster.set_true_dir({true_dir_x, true_dir_y, true_dir_z});
        // cluster.set_true_energy(true_energy);
        // cluster.set_true_label(true_label);
        // cluster.set_reco_pos({reco_pos_x, reco_pos_y, reco_pos_z});
        // cluster.set_min_distance_from_true_pos(min_distance_from_true_pos);
        // cluster.set_supernova_tp_fraction(supernova_tp_fraction);       
        // cluster.set_true_interaction(true_interaction);
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

