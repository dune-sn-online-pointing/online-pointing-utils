#include "Backtracking.h"
#include "TriggerPrimitive.hpp"

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

// read the tps from the files and save them in a vector
void read_tpstream(std::string filename,
                 std::vector<TriggerPrimitive>& tps,
                 std::vector<TrueParticle>& true_particles,
                 std::vector<Neutrino>& neutrinos,
                 int supernova_option,
                 int event_number,
                 double time_tolerance_ticks,
                 int channel_tolerance) {

    if (debugMode) LogInfo << " Reading file: " << filename << std::endl;

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

    if (verboseMode) LogInfo << " For this file, interaction type: " << this_interaction << std::endl;

    std::string TPtree_path = "triggerAnaDumpTPs/TriggerPrimitives/tpmakerTPC__TriggerAnaTree1x2x2"; // TODO make flexible for 1x2x6 and maybe else
    TTree *TPtree = dynamic_cast<TTree*>(file->Get(TPtree_path.c_str()));
    if (!TPtree) {
        LogError << " Tree not found: " << TPtree_path << std::endl;
        return; // can still go to next file
    }
    
    int first_tp_entry_in_event = -1;
    int last_tp_entry_in_event = -1;

    UInt_t this_event_number = 0;
    if (TPtree->SetBranchAddress("Event", &this_event_number) < 0) {
        LogWarning << "Failed to bind branch 'Event'" << std::endl;
    }

    get_first_and_last_event(TPtree, &this_event_number, event_number, first_tp_entry_in_event, last_tp_entry_in_event);

    if (debugMode) LogInfo << "First entry having this event number: " << first_tp_entry_in_event << std::endl;
    if (debugMode) LogInfo << "Last entry having this event number: " << last_tp_entry_in_event << std::endl;

    // Check if we found the event in TPs tree - if not, skip this event (can happen with backgrounds)
    if (first_tp_entry_in_event == -1) {
        if (verboseMode) LogInfo << "Event " << event_number << " has no TPs in file " << filename << " (skipping)" << std::endl;
        return; // Return with empty vectors - this is normal for some background events
    }

    if (verboseMode) LogInfo << "Number of TPs in event " << event_number << ": " << last_tp_entry_in_event - first_tp_entry_in_event + 1 << std::endl;

    tps.reserve(last_tp_entry_in_event - first_tp_entry_in_event + 1);

    UShort_t this_version = 0;
    uint64_t this_time_start = 0;
    UInt_t this_channel = 0;          // Raw channel read from file (may be detector-local)
    UInt_t this_adc_integral = 0;
    UShort_t this_adc_peak = 0;
    UShort_t this_detid = 0;          // Detector (APA) id if present
    UShort_t this_samples_over_threshold = 0;
    UShort_t this_samples_to_peak = 0;
    // int this_event_number = 0;

    bindBranch(TPtree,"version", &this_version); // dropped v1, there are no samples with it
    bindBranch(TPtree,"time_start", &this_time_start);
    bindBranch(TPtree,"channel", &this_channel);
    bindBranch(TPtree,"adc_integral", &this_adc_integral);
    bindBranch(TPtree,"adc_peak", &this_adc_peak);
    bindBranch(TPtree,"detid", &this_detid);
    bindBranch(TPtree,"Event", &this_event_number);
    bindBranch(TPtree,"samples_over_threshold", &this_samples_over_threshold);
    bindBranch(TPtree,"samples_to_peak", &this_samples_to_peak);

    if (verboseMode) LogInfo << " Reading tree of TriggerPrimitives" << std::endl;

    // Counters and guards for ToT-based filtering
    size_t tot_filtered_count = 0;
    size_t tot_nonzero_seen = 0; // if no TP has ToT>0, we skip applying the ToT<2 filter

    // Loop over the entries in the tree
    for (Long64_t iTP = first_tp_entry_in_event; iTP <= last_tp_entry_in_event; ++iTP) {
        TPtree->GetEntry(iTP);
    
        // Determine if the channel appears detector-local; if so promote to global numbering
        // Global scheme is: global = detid * APA::total_channels + local
        uint64_t effective_channel = this_channel;
        if (this_detid > 0 && this_channel >= 0 && this_channel < APA::total_channels) {
            effective_channel = (uint64_t)this_detid * APA::total_channels + (uint64_t)this_channel;
        }
        // Heuristic safeguard: if channel already large (>= total_channels) but detid==0 we assume it's already global and leave unchanged

        // Track whether any non-zero ToT is observed; we'll apply the ToT<2 filter only if ToT info is present and non-trivial
        if (this_samples_over_threshold > 0) {
            ++tot_nonzero_seen;
        }

        TriggerPrimitive this_tp = TriggerPrimitive(
            this_version,
            0, // flag
            this_detid,
            effective_channel,
            this_samples_over_threshold,
            this_time_start,
            this_samples_to_peak,
            this_adc_integral,
            this_adc_peak
        );

        this_tp.SetEvent(this_event_number);
        
        tps.emplace_back(this_tp); // add to collection of TPs
    }

    // Apply ToT<2 filter only if ToT info is actually populated (some non-zero values seen)
    if (!tps.empty()) {
        if (tot_nonzero_seen > 0) {
            // Filter in-place: keep only TPs with ToT >= 2
            auto before = tps.size();
            tps.erase(std::remove_if(tps.begin(), tps.end(), [&](const TriggerPrimitive& tp){
                return tp.GetSamplesOverThreshold() < 2;
            }), tps.end());
            tot_filtered_count = before - tps.size();

            if (verboseMode) LogInfo << " Found " << tps.size() << " TPs in file " << filename << " after ToT>=2 filter"
                    << " (filtered " << tot_filtered_count << ")" << std::endl;
        } else {
            // No meaningful ToT data; skip the filter to avoid dropping everything
            // This is expected for older file formats or certain simulation types
            if (verboseMode) LogInfo << " Event " << this_event_number << ": ToT field absent or all zeros for " << tps.size() 
                       << " TPs; skipping ToT<2 filter (keeping all TPs from file " << filename << ")" << std::endl;
            // Only warn if we have very few TPs AND they all have ToT=0, which might indicate a problem
            if (tps.size() < 10 && tps.size() > 0) {
                LogWarning << " Event " << this_event_number << ": Low TP count (" << tps.size() 
                           << ") with all ToT=0 in file " << filename << " - this may be normal for background events" << std::endl;
            }
        }
    } else {
        LogWarning << " Found no TPs in file " << filename << " (nothing to filter)" << std::endl;
    }

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

    get_first_and_last_event(MCparticlestree, &event, this_event_number, first_mcparticle_entry_in_event, last_mcparticle_entry_in_event);

    if (verboseMode) LogInfo << "Number of MC particles in event " << event_number << ": " << last_mcparticle_entry_in_event - first_mcparticle_entry_in_event + 1 << std::endl;

    // we will have as many true particles as entries in this tree, 
    // including both geant and gen particles

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

    if (!SetBranchWithFallback(MCparticlestree, {"Px", "px"}, &px, "MC particles Px")) {
        return;
    }
    if (!SetBranchWithFallback(MCparticlestree, {"Py", "py"}, &py, "MC particles Py")) {
        return;
    }
    if (!SetBranchWithFallback(MCparticlestree, {"Pz", "pz"}, &pz, "MC particles Pz")) {
        return;
    }
    if (!SetBranchWithFallback(MCparticlestree, {"en", "energy"}, &energy, "MC particles energy")) {
        return;
    }

    MCparticlestree->SetBranchAddress("pdg", &pdg);
    MCparticlestree->SetBranchAddress("Event", &event);
    MCparticlestree->SetBranchAddress("g4_track_id", &track_id);
    MCparticlestree->SetBranchAddress("truth_block_id", &truth_id);
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

    if (verboseMode) LogInfo << " Found " << true_particles.size() << " geant particles in file " << filename << std::endl;

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

    UInt_t event_truth = 0;
    MCtruthtree->SetBranchAddress("Event", &event_truth);

    get_first_and_last_event(MCtruthtree, &event_truth, this_event_number, first_mctruth_entry_in_event, last_mctruth_entry_in_event);

    if (verboseMode) LogInfo << "Number of MC truths in event " << event_number << ": " << last_mctruth_entry_in_event - first_mctruth_entry_in_event + 1 << std::endl;

    // this is a scope vector, just to put somewhere the generator name before
    // associating to the final true particles 
    std::vector <TrueParticle> mc_true_particles; 
    
    if (verboseMode) LogInfo << " Reading tree of MCtruths, there are " << MCtruthtree->GetEntries() << " entries" << std::endl;
    MCtruthtree->SetBranchAddress("generator_name", &generator_name);
    MCtruthtree->SetBranchAddress("x", &x);
    MCtruthtree->SetBranchAddress("y", &y);
    MCtruthtree->SetBranchAddress("z", &z);
    if (!SetBranchWithFallback(MCtruthtree, {"Px", "px"}, &px, "MC truth Px")) {
        return;
    }
    if (!SetBranchWithFallback(MCtruthtree, {"Py", "py"}, &py, "MC truth Py")) {
        return;
    }
    if (!SetBranchWithFallback(MCtruthtree, {"Pz", "pz"}, &pz, "MC truth Pz")) {
        return;
    }
    if (!SetBranchWithFallback(MCtruthtree, {"en", "energy"}, &energy, "MC truth energy")) {
        return;
    }
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

            if (verboseMode) LogInfo << " Neutrino energy is " << energy*1e3 << " MeV" << std::endl;
        }
        else { // particles from neutrino interaction or backgrounds
            
            // if status code is 0, particle is not tracked (initial state)
            if (status_code == 0)  continue;

            // LogInfo << " Found a true particle with generator " << *generator_name << std::endl;
            mc_true_particles.emplace_back(
                TrueParticle(
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
                    "", // process not available from mctruth tree
                    -1, // track_id not available from mctruth, will be set later from mcparticles
                    block_id
                )
            );
            // LogInfo << "  Status code: " << status_code << " and energy " << energy << std::endl;
        }
    }

    if (verboseMode) LogInfo << " There are " << mc_true_particles.size() << " true particles" << std::endl;
    if (verboseMode) LogInfo << " There are " << neutrinos.size() << " neutrinos" << std::endl;
    
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

    get_first_and_last_event(simidestree, &event_number_simides, this_event_number, first_simide_entry_in_event, last_simide_entry_in_event);

    // Int_t event_number_simides;
    UInt_t ChannelID;
    UShort_t Timestamp;
    Int_t trackID;
    float x_simide;

    simidestree->SetBranchAddress("Event", &event_number_simides);

    if (!SetBranchWithFallback(simidestree, {"ChannelID", "channel"}, &ChannelID, "SimIDEs channel")) {
        return;
    }

    if (!SetBranchWithFallback(simidestree, {"Timestamp", "timestamp"}, &Timestamp, "SimIDEs timestamp")) {
        return;
    }

    if (!SetBranchWithFallback(simidestree, {"trackID", "origTrackID"}, &trackID, "SimIDEs track ID")) {
        return;
    }
    simidestree->SetBranchAddress("x", &x_simide);

    if (verboseMode) LogInfo << " Reading tree of SimIDEs to find channels and timestamps associated to MC particles" << std::endl;
    if (verboseMode) LogInfo << " Number of SimIDEs in this event: " << last_simide_entry_in_event - first_simide_entry_in_event + 1 << std::endl;

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
            // Use SimIDE timestamp directly (clock ticks) to match TP time_start units
            // Store SimIDE times scaled into TPC tick domain (Timestamp * conversion_tdc_to_tpc)
            // (Later we'll optionally apply an offset correction to align with TP time origin.)
            // OVERFLOW FIX: Cast to larger type before multiplication to prevent 16-bit overflow
            uint64_t timestampU64 = (uint64_t)Timestamp;
            double tConverted = (double)(timestampU64 * conversion_tdc_to_tpc);
            
            // DIAGNOSTIC: Check for potential overflow (when raw timestamp > 2048 for 32x conversion)
            if (Timestamp > 2048) {
                double wouldOverflow = (double)((uint16_t)(Timestamp * conversion_tdc_to_tpc)); // simulate 16-bit overflow
                // LogInfo << "[OVERFLOW-FIX] Raw timestamp " << Timestamp << " -> corrected: " << tConverted << " (would have been: " << wouldOverflow << " with overflow)" << std::endl;
            }
            
            particle->SetTimeStart(std::min(particle->GetTimeStart(), tConverted));
            particle->SetTimeEnd(std::max(particle->GetTimeEnd(), tConverted));
            particle->AddChannel(ChannelID);
            match_count++;
        } else {
            if (verboseMode) LogWarning << "TrackID " << trackID << " not found in MC particles." << std::endl;
        }
    } // end of simides, not used anywhere anymore

    if (verboseMode) LogInfo << " Matched " << std::setprecision(2) << std::fixed << float(match_count)/(last_simide_entry_in_event-first_simide_entry_in_event+1)*100. << " %" << " SimIDEs to true particles" << std::endl;

    int truepart_with_simideInfo = 0;
    for (auto& particle : true_particles) {
        if (particle.GetChannels().size() > 0) {
            truepart_with_simideInfo++;
        }
    }

    if (verboseMode) LogInfo << " Number of geant particles with SimIDEs info: " << float(truepart_with_simideInfo)/true_particles.size()*100. << " %" << std::endl;
    if (verboseMode) LogInfo << " If not 100%, it's ok. Some particles (nuclei) don't produce SimIDEs" << std::endl;

    // NEW: Apply direct TP-SimIDE matching to replace/supplement trueParticle-based matching
    if (verboseMode) LogInfo << " Applying direct TP-SimIDE matching for event " << this_event_number << std::endl;
    const double effective_time_tolerance = (time_tolerance_ticks >= 0.0) ? time_tolerance_ticks : 5000.0;
    match_tps_to_simides_direct(tps, true_particles, file, this_event_number, effective_time_tolerance, channel_tolerance);

    file->Close(); // don't need anymore

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Connect trueparticles to mctruths using the truth_id

    if (verboseMode) LogInfo << " Connecting MC particles to mctruths, there are "<< neutrinos.size() << " neutrinos and " << true_particles.size() << " other particles" << std::endl;
    
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
                if (debugMode)  LogInfo << " Found a match, generator name: " << mc_true_particle.GetGeneratorName() << std::endl;
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

    if (verboseMode) LogInfo << " Matched MC particles to mctruths: "  << float(matched_MCparticles_counter)/true_particles.size()*100. << " %" << std::endl;
    
    // Update embedded generator names in TPs after MC truth association
    for (auto& tp : tps) {
        const TrueParticle* true_particle = tp.GetTrueParticle();
        if (true_particle != nullptr) {
            // Update the generator name with the corrected value from MC truth
            tp.SetGeneratorName(true_particle->GetGeneratorName());
        }
    }
    if (verboseMode) LogInfo << " Updated embedded generator names in TPs" << std::endl;
    
    // sort the TPs by time
    // C++ 17 has the parameter std::execution::par that handles parallelization, can try that out TODO
    std::clock_t start_sorting = std::clock();
    std::sort(tps.begin(), tps.end(), [](const TriggerPrimitive& a, const TriggerPrimitive& b) {
        return a.GetTimeStart() < b.GetTimeStart();
    });
    std::clock_t end_sorting = std::clock();
    double elapsed_time = double(end_sorting - start_sorting) / CLOCKS_PER_SEC;
    if (verboseMode) LogInfo << "Sorting TPs took " << elapsed_time << " seconds" << std::endl;

}

// TODO change this, one argument should be nentries_event
void get_first_and_last_event(TTree* tree, UInt_t* branch_value, int which_event, int& first_entry, int& last_entry) {
    first_entry = -1;
    last_entry = -1;

    if (debugMode) LogInfo << " Looking for event number " << which_event << std::endl;
    for (int iEntry = 0; iEntry < tree->GetEntries(); ++iEntry) {
        tree->GetEntry(iEntry);
        if (*branch_value == which_event) {
            if (first_entry == -1) {
                first_entry = iEntry;
            }
            last_entry = iEntry;
        } else if (first_entry != -1) {
            // Since entries are ordered, once we see a different event after finding the first, we stop
            break;
        }
    }
}

// Direct TP-SimIDE matching function based on time and channel proximity
void match_tps_to_simides_direct(
    std::vector<TriggerPrimitive>& tps,
    std::vector<TrueParticle>& true_particles,
    TFile* file,
    int event_number,
    double time_tolerance_ticks,
    int channel_tolerance)
{
    if (verboseMode) LogInfo << "Starting direct TP-SimIDE matching for event " << event_number << std::endl;
    
    // Fetch any previously estimated time-offset correction for this event
    // double event_time_offset = 0.0;
    // {
    //     auto it = g_event_time_offsets.find(event_number);
    //     if (it != g_event_time_offsets.end()) event_time_offset = it->second;
    // }
    // if (event_time_offset != 0.0) {
    //     LogInfo << "[DIRECT] Using event time-offset correction (ticks) = " << event_time_offset << std::endl;
    // }

    // Clear any existing truth links
    for (auto& tp : tps) {
        if (tp.GetEvent() == event_number) {
            tp.SetTrueParticle(nullptr);
        }
    }
    
    // Read SimIDEs tree
    std::string simidestree_path = "triggerAnaDumpTPs/simides";
    TTree *simidestree = dynamic_cast<TTree*>(file->Get(simidestree_path.c_str()));
    if (!simidestree) {
        LogError << "SimIDEs tree not found: " << simidestree_path << std::endl;
        return;
    }
    
    // Get SimIDE entries for this event
    int first_simide_entry = -1;
    int last_simide_entry = -1;
    UInt_t event_number_simides = 0;
    simidestree->SetBranchAddress("Event", &event_number_simides);
    get_first_and_last_event(simidestree, &event_number_simides, event_number, first_simide_entry, last_simide_entry);
    
    if (verboseMode)  LogInfo << "SimIDEs in event " << event_number << ": " 
            << ((first_simide_entry != -1) ? std::to_string(last_simide_entry - first_simide_entry + 1) : "0") << std::endl;

    if (first_simide_entry == -1) {
        LogWarning << "No SimIDEs found for event " << event_number << std::endl;
        return;
    }
    
    // Set up SimIDE branch addresses
    UInt_t simide_channel;
    UShort_t simide_timestamp;
    Int_t simide_track_id;
    Float_t simide_energy;  // Energy in MeV

    if (!SetBranchWithFallback(simidestree, {"ChannelID", "channel"}, &simide_channel, "SimIDEs channel")) {
        return;
    }
    if (!SetBranchWithFallback(simidestree, {"Timestamp", "timestamp"}, &simide_timestamp, "SimIDEs timestamp")) {
        return;
    }
    if (!SetBranchWithFallback(simidestree, {"trackID", "origTrackID"}, &simide_track_id, "SimIDEs track ID")) {
        return;
    }
    
    // Try to read the energy branch (optional - may not be available in all files)
    bool has_energy = (simidestree->GetBranch("energy") != nullptr);
    if (has_energy) {
        simidestree->SetBranchAddress("energy", &simide_energy);
        if (verboseMode) LogInfo << "SimIDE energy branch found - will accumulate energy to TPs" << std::endl;
    } else {
        LogWarning << "SimIDE energy branch not found - TP simide_energy will remain 0" << std::endl;
    }
    
    // Build lookup map for true particles by trackID
    std::unordered_map<int, TrueParticle*> track_to_particle;
    for (auto& particle : true_particles) {
        if (particle.GetEvent() == event_number) {
            track_to_particle[std::abs(particle.GetTrackId())] = &particle;
        }
    }
    
    // Structure to hold SimIDE information
    struct SimIDEInfo {
        int channel;
        double time_tpc_ticks;
        int track_id;
        TrueParticle* particle;
        double energy;  // Energy in MeV
    };
    std::vector<SimIDEInfo> simides_in_event;
    
    // Read all SimIDEs for this event
    for (Long64_t iSimIde = first_simide_entry; iSimIde <= last_simide_entry; ++iSimIde) {
        simidestree->GetEntry(iSimIde);
        
        // Convert SimIDE timestamp to TPC ticks (with overflow protection)
        uint64_t timestampU64 = (uint64_t)simide_timestamp;
        double time_tpc = (double)(timestampU64 * conversion_tdc_to_tpc);
        // Apply event time-offset so SimIDE times are comparable to TP times
        double time_tpc_aligned = time_tpc;// - event_time_offset;
        
        // Find associated particle
        auto particle_it = track_to_particle.find(std::abs(simide_track_id));
        if (particle_it != track_to_particle.end()) {
            double energy_mev = has_energy ? simide_energy : 0.0;
            simides_in_event.push_back({
                (int)simide_channel,
                time_tpc_aligned,
                simide_track_id,
                particle_it->second,
                energy_mev
            });
        }
    }
    
    if (verboseMode) LogInfo << "Found " << simides_in_event.size() << " SimIDEs linked to particles in event " << event_number << std::endl;
    
    // SimIDE time and channel ranges (diagnostic output commented out for selected events)
    double min_simide_time = std::numeric_limits<double>::max();
    double max_simide_time = -std::numeric_limits<double>::max();
    int min_simide_channel = INT_MAX;
    int max_simide_channel = INT_MIN;
    if (!simides_in_event.empty()) {
        for (const auto& simide : simides_in_event) {
            min_simide_time = std::min(min_simide_time, simide.time_tpc_ticks);
            max_simide_time = std::max(max_simide_time, simide.time_tpc_ticks);
            min_simide_channel = std::min(min_simide_channel, simide.channel);
            max_simide_channel = std::max(max_simide_channel, simide.channel);
        }
        // Debug: diagnostic range output for specific events (commented out)
        // if (event_number == 4 || event_number == 6 || event_number == 8 || event_number == 10) {
        //     LogInfo << "[SIMIDE-RANGE] Event " << event_number << " time: [" << min_simide_time << "," << max_simide_time 
        //             << "] channels: [" << min_simide_channel << "," << max_simide_channel << "]" << std::endl;
        // }
    }
    
    // Determine APA coverage of SimIDEs to optionally filter TPs when SimIDEs use global channels
    bool simides_use_global_channels = false;
    std::unordered_set<int> simide_apa_set;
    for (const auto& s : simides_in_event) {
        if (s.channel >= APA::total_channels) simides_use_global_channels = true;
        if (s.channel >= APA::total_channels) simide_apa_set.insert(s.channel / APA::total_channels);
    }
    // Debug: show when SimIDEs use global channels (commented out)
    // if (simides_use_global_channels && !simide_apa_set.empty()) {
    //     std::ostringstream apas;
    //     bool first = true; for (int a : simide_apa_set) { if (!first) apas << ","; apas << a; first = false; }
    //     LogInfo << "[DIRECT] SimIDEs appear global; restricting TP candidates to APAs {" << apas.str() << "}" << std::endl;
    // }

    // Helper to infer plane from a channel index (local indexing rules: U:[0,800), V:[800,1600), X:[1600,2560))
    auto infer_plane_from_channel = [](int channel) -> char {
        int local = channel % APA::total_channels;
        if (local < 800) return 'U';
        if (local < 1600) return 'V';
        return 'X';
    };

    // Match TPs to SimIDEs (prefer plane-consistent matches)
    int matched_tp_count = 0;
    int total_tp_count = 0; // number of TPs considered as candidates
    int skipped_tp_outside_windows = 0; // skipped due to APA/time filters
    int matched_same_plane = 0; // diagnostics: how many matches used plane-consistency
    
    for (auto& tp : tps) {
        if (tp.GetEvent() != event_number) continue;
        // If SimIDEs are global, ignore TPs that are outside SimIDE APA set to avoid spurious matches
        if (simides_use_global_channels && !simide_apa_set.empty()) {
            int tp_apa = tp.GetChannel() / APA::total_channels;
            if (simide_apa_set.find(tp_apa) == simide_apa_set.end()) {
                skipped_tp_outside_windows++;
                continue; // skip this TP: different APA than SimIDEs
            }
        }
        // Also restrict to TPs that lie near the SimIDE time span (within tolerance)
        if (min_simide_time <= max_simide_time) {
            double t = tp.GetTimeStart();
            if (t < min_simide_time - time_tolerance_ticks || t > max_simide_time + time_tolerance_ticks) {
                skipped_tp_outside_windows++;
                continue;
            }
        }
        total_tp_count++;
        
        // Find best matching SimIDE based on channel/time proximity with plane preference
        TrueParticle* best_match_same_plane = nullptr;
        double best_score_same_plane = std::numeric_limits<double>::max();
        int best_channel_diff_same_plane = channel_tolerance + 1;
        double best_time_diff_same_plane = time_tolerance_ticks + 1;

        TrueParticle* best_match_any_plane = nullptr;
        double best_score_any_plane = std::numeric_limits<double>::max();
        int best_channel_diff_any_plane = channel_tolerance + 1;
        double best_time_diff_any_plane = time_tolerance_ticks + 1;

        // Determine TP plane (prefer explicit view if available, else infer from channel)
        char tp_plane_char = 'U';
        {
            std::string tp_view = tp.GetView();
            if (!tp_view.empty()) tp_plane_char = tp_view[0];
            else tp_plane_char = infer_plane_from_channel(tp.GetChannel());
        }
        
        for (const auto& simide : simides_in_event) {
            // Handle both local and global channel numbering for SimIDEs
            int tp_global_channel = tp.GetChannel();
            int tp_local_channel = tp_global_channel % 2560;
            
            int channel_diff;
            if (simide.channel < 2560) {
                // SimIDE uses local channel numbering - compare with TP local channel
                channel_diff = std::abs(tp_local_channel - simide.channel);
            } else {
                // SimIDE uses global channel numbering - compare with TP global channel  
                channel_diff = std::abs(tp_global_channel - simide.channel);
            }
            
            // Calculate time difference
            double time_diff = std::abs(tp.GetTimeStart() - simide.time_tpc_ticks);
            
            // Check if within tolerance
            if (channel_diff <= channel_tolerance && time_diff <= time_tolerance_ticks) {
                // Accumulate SimIDE energy to this TP (sum all SimIDEs that overlap)
                tp.AddSimideEnergy(simide.energy);
                
                // Score based on combined time and channel proximity (favor tighter channel matches)
                // Increased channel weight improves spatial consistency in presence of wide time windows
                double score = time_diff + (channel_diff * 20.0);

                // Determine SimIDE plane
                char simide_plane_char = infer_plane_from_channel(simide.channel);
                bool same_plane = (simide_plane_char == tp_plane_char);

                if (same_plane) {
                    if (score < best_score_same_plane) {
                        best_score_same_plane = score;
                        best_match_same_plane = simide.particle;
                        best_channel_diff_same_plane = channel_diff;
                        best_time_diff_same_plane = time_diff;
                    }
                } else {
                    if (score < best_score_any_plane) {
                        best_score_any_plane = score;
                        best_match_any_plane = simide.particle;
                        best_channel_diff_any_plane = channel_diff;
                        best_time_diff_any_plane = time_diff;
                    }
                }
            }
        }
        // Prefer a same-plane match if available; otherwise, fall back to best any-plane match
        bool used_same_plane = false;
        if (best_match_same_plane) {
            tp.SetTrueParticle(best_match_same_plane);
            matched_tp_count++;
            matched_same_plane++;
            used_same_plane = true;

            // Debug: per-match details for specific events (commented out)
            // if (event_number == 4 || event_number == 6 || event_number == 8 || event_number == 10) {
            //     int tp_local_ch = tp.GetChannel() % 2560;
            //     LogInfo << "[DIRECT-MATCH] TP ch=" << tp.GetChannel() << " (local=" << tp_local_ch << ") time=" << tp.GetTimeStart()
            //             << " -> particle_id=" << best_match_same_plane->GetTrackId()
            //             << " (ch_diff=" << best_channel_diff_same_plane << " time_diff=" << best_time_diff_same_plane << ")" << std::endl;
            // }
        }
        else if (best_match_any_plane) {
            tp.SetTrueParticle(best_match_any_plane);
            matched_tp_count++;

            // Debug: fallback any-plane match details (commented out)
            // if (event_number == 4 || event_number == 6 || event_number == 8 || event_number == 10) {
            //     int tp_local_ch = tp.GetChannel() % 2560;
            //     LogInfo << "[DIRECT-MATCH] (fallback-any-plane) TP ch=" << tp.GetChannel() << " (local=" << tp_local_ch << ") time=" << tp.GetTimeStart()
            //             << " -> particle_id=" << best_match_any_plane->GetTrackId()
            //             << " (ch_diff=" << best_channel_diff_any_plane << " time_diff=" << best_time_diff_any_plane << ")" << std::endl;
            // }
        }
        else {
            // Debug: diagnostic for failed matches (commented out)
            // if ((event_number == 4 || event_number == 6 || event_number == 8 || event_number == 10) && total_tp_count <= 10) {
            //     int tp_local_ch = tp.GetChannel() % 2560;
            //     LogInfo << "[NO-MATCH] TP ch=" << tp.GetChannel() << " (local=" << tp_local_ch << ") time=" << tp.GetTimeStart()
            //             << " -> no SimIDE match found within tolerances" << std::endl;
            // }
        }
    }
    
    double match_efficiency = total_tp_count > 0 ? (double)matched_tp_count / total_tp_count * 100.0 : 0.0;
    if (verboseMode) LogInfo << "Direct TP-SimIDE matching results for event " << event_number << ": "
            << matched_tp_count << "/" << total_tp_count << " TPs matched ("
            << std::fixed << std::setprecision(1) << match_efficiency << "%)" << std::endl;
    if (skipped_tp_outside_windows > 0) {
        if (verboseMode) LogInfo << "[DIRECT] Skipped " << skipped_tp_outside_windows << " TPs outside APA/time windows." << std::endl;
    }
    if (matched_tp_count > 0) {
        if (verboseMode) LogInfo << "[DIRECT] Plane-consistent matches: " << matched_same_plane << "/" << matched_tp_count << std::endl;
    }
    
    // Debug: TP channel ranges for comparison (commented out)
    // if ((event_number == 4 || event_number == 6 || event_number == 8 || event_number == 10) && total_tp_count > 0) {
    //     int min_tp_channel = std::numeric_limits<int>::max();
    //     int max_tp_channel = 0;
    //     int min_tp_local = std::numeric_limits<int>::max();
    //     int max_tp_local = 0;
    //     for (const auto& tp : tps) {
    //         if (tp.GetEvent() == event_number) {
    //             int global_ch = tp.GetChannel();
    //             int local_ch = global_ch % 2560;
    //             min_tp_channel = std::min(min_tp_channel, global_ch);
    //             max_tp_channel = std::max(max_tp_channel, global_ch);
    //             min_tp_local = std::min(min_tp_local, local_ch);
    //             max_tp_local = std::max(max_tp_local, local_ch);
    //         }
    //     }
    //     LogInfo << "[TP-RANGE] Event " << event_number << " global: [" << min_tp_channel << "," << max_tp_channel 
    //             << "] local: [" << min_tp_local << "," << max_tp_local << "]" << std::endl;
    // }
}

std::vector<float> calculate_position(TriggerPrimitive* tp) {
    // ...existing code from cluster.cpp...
    float x_signs = (int(tp->GetDetectorChannel()) % APA::total_channels < (APA::induction_channels * 2 + APA::collection_channels)) ? -1.0f : 1.0f;
    float x = ((int(tp->GetTimeStart()) ) * time_tick_in_cm + apa_width_in_cm/2) * x_signs; 
    float y = 0;
    float z = 0;
    if (tp->GetView() == "X") {
        float z_apa_offset = int( tp->GetDetector() / 2 ) * (apa_lenght_in_cm + offset_between_apa_in_cm);
        float z_channel_offset = ((int(tp->GetDetectorChannel()) - APA::induction_channels*2) % APA::collection_channels/2) * wire_pitch_in_cm_collection;
        z = wire_pitch_in_cm_collection + z_apa_offset + z_channel_offset;
    }
    return {x, y, z};
}

std::vector<std::vector<float>> validate_position_calculation(std::vector<TriggerPrimitive*> tps) {
    std::vector<std::vector<float>> positions;
    return positions;
}

float eval_y_knowing_z_U_plane(std::vector<TriggerPrimitive*> tps, float z, float x_sign) {
    z = z - int(tps.at(0)->GetDetectorChannel()) / (APA::total_channels*2) * (apa_lenght_in_cm + offset_between_apa_in_cm); // not sure about the 0 TODO
    float ordinate;
    std::vector<float> Y_pred;
    for (auto& tp : tps) {
        if ((int(tp->GetDetectorChannel()) / APA::total_channels) % 2 == 0) {
            if (x_sign < 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels < 400) {
                    if (z > (int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels > 399) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp->GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels > 399) {
                    if (z < (799 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (799 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels < 400) {
                    ordinate = (z + (int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            }
        } else if ((int(tp->GetDetectorChannel()) / APA::total_channels) % 2 == 1) {
            if (x_sign < 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels < 400) {
                    if (z < (399 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (399 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels > 399) {
                    ordinate = (z + (int(tp->GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels > 399) {
                    if (z > (int(tp->GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp->GetDetectorChannel()) % APA::total_channels - 400) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels < 400) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            }
        }
        // ordinate = (ordinate) - apa_height_in_cm if (tp[idx['channel']] / APA::total_channels) % 2 < 1 else apa_height_in_cm - (ordinate);
        if ((int(tp->GetDetectorChannel()) / APA::total_channels) % 2 < 1) {
            ordinate = (ordinate) - apa_height_in_cm;
        } else {
            ordinate = apa_height_in_cm - (ordinate);
        }
        Y_pred.push_back(ordinate);
    }
    // mean
    float Y_pred_mean = 0;
    for (auto& y : Y_pred) {
        Y_pred_mean += y;
    }
    Y_pred_mean = Y_pred_mean / Y_pred.size();

    return Y_pred_mean;
}

float eval_y_knowing_z_V_plane(std::vector<TriggerPrimitive*> tps, float z, float x_sign) {
    
    z = z - int(tps.at(0)->GetDetectorChannel()) / (APA::total_channels*2) * (apa_lenght_in_cm + offset_between_apa_in_cm);
    float ordinate;
    std::vector<float> Y_pred;
    for (auto& tp : tps) {
        if ((int(tp->GetDetectorChannel()) / APA::total_channels) % 2 == 0) {
            if (x_sign < 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels < 1200) {
                    if (z < (1199 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels - 800) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (1199 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels > 1199) {
                    ordinate = (z + (int(tp->GetDetectorChannel()) % APA::total_channels - 1200) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels > 1199) {
                    if (z > (int(tp->GetDetectorChannel()) % APA::total_channels - 1200) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels - 1200) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp->GetDetectorChannel()) % APA::total_channels - 1200) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels < 1200) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp->GetDetectorChannel()) % APA::total_channels - 800) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } 
        } else if ((int(tp->GetDetectorChannel()) / APA::total_channels) % 2 == 1) {
            if (x_sign < 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels < 1200) {
                    if (z > (int(tp->GetDetectorChannel()) % APA::total_channels - 800) * wire_pitch_in_cm_induction + backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels - 800) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = apa_lenght_in_cm - z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = ((int(tp->GetDetectorChannel()) % APA::total_channels - 800) * wire_pitch_in_cm_induction - z) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels > 1199) {
                    ordinate = (apa_lenght_in_cm - z + (int(tp->GetDetectorChannel()) % APA::total_channels - 1200) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            } else if (x_sign > 0) {
                if (int(tp->GetDetectorChannel()) % APA::total_channels > 1199) {
                    if (z < (1599 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction - backtracker_error_margin) {
                        float until_turn = (int(tp->GetDetectorChannel()) % APA::total_channels - 1200) * wire_pitch_in_cm_induction;
                        float all_the_way_behind = apa_lenght_in_cm;
                        float the_last_piece = z;
                        ordinate = (until_turn + all_the_way_behind + the_last_piece) * apa_angular_coeff;
                    } else {
                        ordinate = (z - (1599 - int(tp->GetDetectorChannel()) % APA::total_channels) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                    }
                } else if (int(tp->GetDetectorChannel()) % APA::total_channels < 1200) {
                    ordinate = (z + (int(tp->GetDetectorChannel()) % APA::total_channels - 800) * wire_pitch_in_cm_induction) * apa_angular_coeff;
                }
            }
        }
        if ((int(tp->GetDetectorChannel()) / APA::total_channels) % 2 < 1) {
            ordinate = (ordinate) - apa_height_in_cm;
        } else {
            ordinate = apa_height_in_cm - (ordinate);
        }

        Y_pred.push_back(ordinate);
    }
    // mean
    float Y_pred_mean = 0;
    for (auto& y : Y_pred) {
        Y_pred_mean += y;
    }
    Y_pred_mean = Y_pred_mean / Y_pred.size();

    return Y_pred_mean;
}

void write_tps(
    const std::string& out_filename,
    const std::vector<std::vector<TriggerPrimitive>>& tps_by_event,
    const std::vector<std::vector<TrueParticle>>& true_particles_by_event,
    const std::vector<std::vector<Neutrino>>& neutrinos_by_event)
{
    // Ensure output directory exists
    std::string folder = out_filename.substr(0, out_filename.find_last_of("/"));
    if (!ensureDirectoryExists(folder)) {
        LogError << "Cannot create or access directory for output file: " << folder << std::endl;
        return;
    }

    TFile outFile(out_filename.c_str(), "RECREATE");
    if (outFile.IsZombie()) {
        LogError << "Cannot create output file: " << out_filename << std::endl;
        return;
    }
    TDirectory* tpsDir = outFile.mkdir("tps");
    tpsDir->cd();

    // TPs tree with embedded truth
    TTree tpsTree("tps", "Trigger Primitives with embedded truth");
    
    // TP basic variables
    int evt = 0; 
    UShort_t version=0; 
    UInt_t detid=0; 
    UInt_t channel=0; 
    UInt_t adc_integral=0; 
    UShort_t adc_peak=0; 
    UShort_t det=0; 
    Int_t det_channel=0; 
    ULong64_t tstart=0; 
    ULong64_t s_over=0; 
    ULong64_t s_to_peak=0;
    std::string view;
    Double_t simide_energy = 0.0;
    
    // Truth variables (always stored: generator_name from MC truth)
    std::string gen_name;
    
    // MARLEY-specific particle truth (only meaningful when gen_name contains "marley")
    Int_t particle_pdg = 0;
    std::string particle_process;
    Float_t particle_energy = 0.0f;
    Float_t particle_x = 0.0f, particle_y = 0.0f, particle_z = 0.0f;
    Float_t particle_px = 0.0f, particle_py = 0.0f, particle_pz = 0.0f;
    
    // Neutrino info (only for MARLEY with neutrino association)
    std::string neutrino_interaction;
    Float_t neutrino_x = 0.0f, neutrino_y = 0.0f, neutrino_z = 0.0f;
    Float_t neutrino_px = 0.0f, neutrino_py = 0.0f, neutrino_pz = 0.0f;
    Float_t neutrino_energy = 0.0f;
    
    // TP basic branches
    tpsTree.Branch("event", &evt, "event/I");
    tpsTree.Branch("version", &version, "version/s");
    tpsTree.Branch("detid", &detid, "detid/i");
    tpsTree.Branch("channel", &channel, "channel/i");
    tpsTree.Branch("samples_over_threshold", &s_over, "samples_over_threshold/l");
    tpsTree.Branch("time_start", &tstart, "time_start/l");
    tpsTree.Branch("samples_to_peak", &s_to_peak, "samples_to_peak/l");
    tpsTree.Branch("adc_integral", &adc_integral, "adc_integral/i");
    tpsTree.Branch("adc_peak", &adc_peak, "adc_peak/s");
    tpsTree.Branch("detector", &det, "detector/s");
    tpsTree.Branch("detector_channel", &det_channel, "detector_channel/I");
    tpsTree.Branch("view", &view);
    tpsTree.Branch("simide_energy", &simide_energy, "simide_energy/D");
    
    // Truth branches (always: generator_name from MC truth)
    tpsTree.Branch("generator_name", &gen_name);
    
    // MARLEY-specific particle truth branches
    tpsTree.Branch("particle_pdg", &particle_pdg, "particle_pdg/I");
    tpsTree.Branch("particle_process", &particle_process);
    tpsTree.Branch("particle_energy", &particle_energy, "particle_energy/F");
    tpsTree.Branch("particle_x", &particle_x, "particle_x/F");
    tpsTree.Branch("particle_y", &particle_y, "particle_y/F");
    tpsTree.Branch("particle_z", &particle_z, "particle_z/F");
    tpsTree.Branch("particle_px", &particle_px, "particle_px/F");
    tpsTree.Branch("particle_py", &particle_py, "particle_py/F");
    tpsTree.Branch("particle_pz", &particle_pz, "particle_pz/F");
    
    // Neutrino branches
    tpsTree.Branch("neutrino_interaction", &neutrino_interaction);
    tpsTree.Branch("neutrino_x", &neutrino_x, "neutrino_x/F");
    tpsTree.Branch("neutrino_y", &neutrino_y, "neutrino_y/F");
    tpsTree.Branch("neutrino_z", &neutrino_z, "neutrino_z/F");
    tpsTree.Branch("neutrino_px", &neutrino_px, "neutrino_px/F");
    tpsTree.Branch("neutrino_py", &neutrino_py, "neutrino_py/F");
    tpsTree.Branch("neutrino_pz", &neutrino_pz, "neutrino_pz/F");
    tpsTree.Branch("neutrino_energy", &neutrino_energy, "neutrino_energy/F");

    // Metadata tree
    TTree metaTree("metadata", "File metadata");
    int n_events = tps_by_event.size();
    int n_tps_total = 0;
    for (const auto& v : tps_by_event) n_tps_total += v.size();
    metaTree.Branch("n_events", &n_events, "n_events/I");
    metaTree.Branch("n_tps_total", &n_tps_total, "n_tps_total/I");
    metaTree.Fill();

    // Fill TPs with embedded truth
    for (size_t ev = 0; ev < tps_by_event.size(); ++ev) {
        const auto& v = tps_by_event[ev];
        for (const auto& tp : v) {
            // Basic TP info
            evt = tp.GetEvent(); 
            version = TriggerPrimitive::s_trigger_primitive_version; 
            detid = 0; 
            channel = tp.GetChannel(); 
            s_over = tp.GetSamplesOverThreshold(); 
            tstart = tp.GetTimeStart(); 
            s_to_peak = tp.GetSamplesToPeak(); 
            adc_integral = tp.GetAdcIntegral(); 
            adc_peak = tp.GetAdcPeak(); 
            det = tp.GetDetector(); 
            det_channel = tp.GetDetectorChannel(); 
            view = tp.GetView();
            simide_energy = tp.GetSimideEnergy();
            
            // Truth info (embedded in TP)
            gen_name = tp.GetGeneratorName();
            particle_pdg = tp.GetParticlePDG();
            particle_process = tp.GetParticleProcess();
            particle_energy = tp.GetParticleEnergy();
            particle_x = tp.GetParticleX();
            particle_y = tp.GetParticleY();
            particle_z = tp.GetParticleZ();
            particle_px = tp.GetParticlePx();
            particle_py = tp.GetParticlePy();
            particle_pz = tp.GetParticlePz();
            neutrino_interaction = tp.GetNeutrinoInteraction();
            neutrino_x = tp.GetNeutrinoX();
            neutrino_y = tp.GetNeutrinoY();
            neutrino_z = tp.GetNeutrinoZ();
            neutrino_px = tp.GetNeutrinoPx();
            neutrino_py = tp.GetNeutrinoPy();
            neutrino_pz = tp.GetNeutrinoPz();
            neutrino_energy = tp.GetNeutrinoEnergy();
            
            tpsTree.Fill();
        }
    }

    tpsDir->cd();
    tpsTree.Write(); 
    metaTree.Write();
    outFile.Close();
    
    // Report absolute output path for consistency
    std::error_code _ec_abs;
    auto abs_p = std::filesystem::absolute(std::filesystem::path(out_filename), _ec_abs);
    if (verboseMode) LogInfo << "Wrote TPs file: " << (_ec_abs ? out_filename : abs_p.string()) << std::endl;
}
