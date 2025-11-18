#include "Clustering.h"

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

void read_tps(const std::string& in_filename, 
        std::map<int, std::vector<TriggerPrimitive>>& tps_by_event, 
        std::map<int, std::vector<TrueParticle>>& true_particles_by_event, 
        std::map<int, std::vector<Neutrino>>& neutrinos_by_event){
    
    if (verboseMode) LogInfo << "Reading TPs from: " << in_filename << std::endl;
    
    TFile inFile(in_filename.c_str(), "READ"); 
    if (inFile.IsZombie()) { LogError << "Cannot open: " << in_filename << std::endl; return; }

    // Read TPs tree from root level (no longer in "tps" directory)
    if (auto* tpTree = dynamic_cast<TTree*>(inFile.Get("tps"))) {
        
        // TP basic variables
        int event=0; 
        UShort_t version=0; 
        UInt_t detid=0, channel=0; 
        ULong64_t s_over=0, tstart=0, s_to_peak=0; 
        UInt_t adc_integral=0; 
        UShort_t adc_peak=0, det=0; 
        Int_t det_channel=0; 
        std::string* view=nullptr;
        Double_t simide_energy=0.0;
        
        // Truth variables
        std::string* gen_name=nullptr;
        Int_t particle_pdg=0;
        std::string* particle_process=nullptr;
        Float_t particle_energy=0.0f;
        Float_t particle_x=0.0f, particle_y=0.0f, particle_z=0.0f;
        Float_t particle_px=0.0f, particle_py=0.0f, particle_pz=0.0f;
        std::string* neutrino_interaction=nullptr;
        Float_t neutrino_x=0.0f, neutrino_y=0.0f, neutrino_z=0.0f;
        Float_t neutrino_px=0.0f, neutrino_py=0.0f, neutrino_pz=0.0f;
        Float_t neutrino_energy=0.0f;
        
        // Set branch addresses for TP basics
        tpTree->SetBranchAddress("event", &event); 
        tpTree->SetBranchAddress("version", &version); 
        tpTree->SetBranchAddress("detid", &detid); 
        tpTree->SetBranchAddress("channel", &channel);
        tpTree->SetBranchAddress("samples_over_threshold", &s_over);
        tpTree->SetBranchAddress("time_start", &tstart);
        tpTree->SetBranchAddress("samples_to_peak", &s_to_peak);
        tpTree->SetBranchAddress("adc_integral", &adc_integral);
        tpTree->SetBranchAddress("adc_peak", &adc_peak);
        tpTree->SetBranchAddress("detector", &det);
        tpTree->SetBranchAddress("detector_channel", &det_channel);
        tpTree->SetBranchAddress("view", &view);
        if (tpTree->GetBranch("simide_energy")) {
            tpTree->SetBranchAddress("simide_energy", &simide_energy);
        }
        
        // Set branch addresses for truth
        tpTree->SetBranchAddress("generator_name", &gen_name);
        if (tpTree->GetBranch("particle_pdg")) tpTree->SetBranchAddress("particle_pdg", &particle_pdg);
        if (tpTree->GetBranch("particle_process")) tpTree->SetBranchAddress("particle_process", &particle_process);
        if (tpTree->GetBranch("particle_energy")) tpTree->SetBranchAddress("particle_energy", &particle_energy);
        if (tpTree->GetBranch("particle_x")) tpTree->SetBranchAddress("particle_x", &particle_x);
        if (tpTree->GetBranch("particle_y")) tpTree->SetBranchAddress("particle_y", &particle_y);
        if (tpTree->GetBranch("particle_z")) tpTree->SetBranchAddress("particle_z", &particle_z);
        if (tpTree->GetBranch("particle_px")) tpTree->SetBranchAddress("particle_px", &particle_px);
        if (tpTree->GetBranch("particle_py")) tpTree->SetBranchAddress("particle_py", &particle_py);
        if (tpTree->GetBranch("particle_pz")) tpTree->SetBranchAddress("particle_pz", &particle_pz);
        if (tpTree->GetBranch("neutrino_interaction")) tpTree->SetBranchAddress("neutrino_interaction", &neutrino_interaction);
        if (tpTree->GetBranch("neutrino_x")) tpTree->SetBranchAddress("neutrino_x", &neutrino_x);
        if (tpTree->GetBranch("neutrino_y")) tpTree->SetBranchAddress("neutrino_y", &neutrino_y);
        if (tpTree->GetBranch("neutrino_z")) tpTree->SetBranchAddress("neutrino_z", &neutrino_z);
        if (tpTree->GetBranch("neutrino_px")) tpTree->SetBranchAddress("neutrino_px", &neutrino_px);
        if (tpTree->GetBranch("neutrino_py")) tpTree->SetBranchAddress("neutrino_py", &neutrino_py);
        if (tpTree->GetBranch("neutrino_pz")) tpTree->SetBranchAddress("neutrino_pz", &neutrino_pz);
        if (tpTree->GetBranch("neutrino_energy")) tpTree->SetBranchAddress("neutrino_energy", &neutrino_energy);

        for (Long64_t i=0;i<tpTree->GetEntries();++i){ 

            tpTree->GetEntry(i); 

            TriggerPrimitive tp(version, 0, detid, channel, s_over, tstart, s_to_peak, adc_integral, adc_peak); 
            tp.SetEvent(event); 
            tp.SetDetector(det);
            tp.SetDetectorChannel(det_channel);
            tp.SetSimideEnergy(simide_energy);
            
            // Set embedded truth
            if (gen_name) tp.SetGeneratorName(*gen_name);
            tp.SetParticlePDG(particle_pdg);
            if (particle_process) tp.SetParticleProcess(*particle_process);
            tp.SetParticleEnergy(particle_energy);
            tp.SetParticlePosition(particle_x, particle_y, particle_z);
            tp.SetParticleMomentum(particle_px, particle_py, particle_pz);
            if (neutrino_interaction) {
                tp.SetNeutrinoInfo(*neutrino_interaction, neutrino_x, neutrino_y, neutrino_z,
                                  neutrino_px, neutrino_py, neutrino_pz, neutrino_energy);
            }
            
            tps_by_event[event].push_back(tp);
        }
    }

    // Note: true_particles_by_event and neutrinos_by_event are no longer populated
    // Truth information is now embedded directly in TPs
    // These maps are kept as function parameters for backward compatibility but will be empty

    inFile.Close();
}

// PBC is periodic boundary condition
bool channel_condition_with_pbc(TriggerPrimitive *tp1, TriggerPrimitive* tp2, int channel_limit) {
    if ( tp1->GetDetector() !=  tp2->GetDetector()
        || tp1->GetView() !=  tp2->GetView())
        return false;

    double diff = std::abs(tp1->GetDetectorChannel() - tp2->GetDetectorChannel());
    int channels_in_this_view = APA::channels_in_view[tp1->GetView()];
    
    // For X plane, check if TPs are in the same TPC volume
    // X plane: 960 channels split into 2 volumes of 480 each
    // Channels 1600-2079 → volume 0, channels 2080-2559 → volume 1
    if (tp1->GetView() == "X") {
        int ch1 = tp1->GetDetectorChannel() % 2560;
        int ch2 = tp2->GetDetectorChannel() % 2560;
        
        // Determine which volume each TP is in
        bool tp1_in_vol0 = (ch1 >= 1600 && ch1 < 2080);
        bool tp1_in_vol1 = (ch1 >= 2080 && ch1 < 2560);
        bool tp2_in_vol0 = (ch2 >= 1600 && ch2 < 2080);
        bool tp2_in_vol1 = (ch2 >= 2080 && ch2 < 2560);
        
        // Reject if TPs are in different volumes
        if ((tp1_in_vol0 && tp2_in_vol1) || (tp1_in_vol1 && tp2_in_vol0)) {
            return false;
        }
    }
    
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
std::vector<Cluster> make_cluster(std::vector<TriggerPrimitive*> all_tps, int ticks_limit, int channel_limit, int min_tps_to_cluster, int adc_integral_cut) {
    
    if (verboseMode) LogInfo << "Creating clusters from TPs" << std::endl;
    
    if (verboseMode) LogInfo << "Ticks limit: " << ticks_limit << " TPC ticks" << std::endl;

    int ticks_limit_tdc = toTDCticks(ticks_limit);
    if (verboseMode) LogInfo << "Ticks limit: " << ticks_limit_tdc << " TDC ticks" << std::endl;

    std::vector<std::vector<TriggerPrimitive*>> buffer;
    std::vector<Cluster> clusters;

    // Reset the buffer for each event
    buffer.clear();

    // ...existing code...


    // Helper: non-overlap time gap between two TP time intervals in TPC ticks (overlap -> 0)
    auto interval_gap_ticks = [&](TriggerPrimitive* a, TriggerPrimitive* b) -> int {
        const int a_start = a->GetTimeStart();
        const int a_end   = a_start + toTDCticks(a->GetSamplesOverThreshold());
        const int b_start = b->GetTimeStart();
        const int b_end   = b_start + toTDCticks(b->GetSamplesOverThreshold());
        const int gap1 = a_start - b_end;  // a starts after b ends
        const int gap2 = b_start - a_end;  // b starts after a ends
        const int gap = std::max(0, std::max(gap1, gap2));
    // ...existing code...
        return gap;
    };



    for (int iTP = 0; iTP < all_tps.size(); iTP++) {
        
        
        TriggerPrimitive* tp1 = all_tps.at(iTP);

        if (debugMode) LogInfo << "Processing TP: " << tp1->GetTimeStart() << " " << tp1->GetDetectorChannel() << std::endl;

        bool appended = false;

        for (auto& candidate : buffer) {
            // Check if tp1 can be added to this candidate
            // Rule A (strict): If any TP in candidate is on the SAME channel as tp1 and
            // the time gap > ticks_limit, this candidate must be REJECTED.
            // Rule B: Otherwise, allow append if there exists at least one TP in the candidate
            // such that (time gap <= ticks_limit) AND (channel_condition_with_pbc holds).

            bool reject_due_to_same_channel = false;
            bool can_append = false;

            for (auto& tp2 : candidate) {
                const bool same_channel = (tp1->GetDetectorChannel() == tp2->GetDetectorChannel());
                const int gap = interval_gap_ticks(tp1, tp2);

                if (same_channel) {
                    if (gap > ticks_limit_tdc) {
                        reject_due_to_same_channel = true;
                        break; // No need to check more TPs in this candidate
                    }
                    // gap <= ticks_limit_tdc on same channel is sufficient to allow append
                    can_append = true;
                } else {
                    if (gap <= ticks_limit_tdc && channel_condition_with_pbc(tp1, tp2, channel_limit)) {
                        // ...existing code...


                        can_append = true;
                    }
                }
            }

            if (reject_due_to_same_channel) {
                if (debugMode) LogInfo << "Rejecting candidate due to same-channel time gap > limit" << std::endl;
                continue; // Try next candidate
            }

            if (can_append) {
                candidate.push_back(tp1);
                appended = true;
                if (debugMode) LogInfo << "Appended TP to candidate Cluster" << std::endl;
                break;
            }
        }

        // If not appended to any candidate, create a new Cluster in the buffer
        if (!appended) {
            if (debugMode) LogInfo << "Creating new candidate Cluster" << std::endl;
            buffer.push_back({tp1});
        }
    }

    // Process remaining candidates in the buffer
    for (auto& candidate : buffer) {
        if (candidate.size() >= min_tps_to_cluster) {
            int adc_integral = 0;
            for (auto& tp1 : candidate) {
                adc_integral += tp1->GetAdcIntegral();
            }
            // ENERGY CUT LOGIC IN THE APP, NOT HERE
            // if (adc_integral > adc_integral_cut) {
                // check validity of tps in candidate
                if (debugMode) LogInfo << "Candidate Cluster has " << candidate.size() << " TPs" << std::endl;
                clusters.emplace_back(Cluster(std::move(candidate)));
                if (debugMode) LogInfo << "Cluster created with " << clusters.back().get_tps().size() << " TPs" << std::endl;
            // }
            // else {
            //     if (verboseMode) LogInfo << "Candidate Cluster rejected due to adc_integral cut: " << adc_integral << " <= " << adc_integral_cut << std::endl;
            // }
        }
    }

    // ...existing code...


    if (verboseMode) LogInfo << "Finished clustering. Number of clusters: " << clusters.size() << std::endl;

    return clusters;
}


std::vector<Cluster> filter_main_tracks(std::vector<Cluster>& clusters) { // valid only if the clusters are ordered by event and for clean sn data
    int best_idx = INT_MAX;

    std::vector<Cluster> main_tracks;
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

std::vector<Cluster> filter_out_main_track(std::vector<Cluster>& clusters) { // valid only if the clusters are ordered by event and for clean sn data
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



    std::vector<Cluster> blips;

    for (int i=0; i<clusters.size(); i++) {
        if (std::find(bad_idx_list.begin(), bad_idx_list.end(), i) == bad_idx_list.end()) {
            blips.push_back(clusters[i]);
        }
    }
    
    if (verboseMode) LogInfo << "Number of blips: " << blips.size() << std::endl;
    return blips;
}


void write_clusters(std::vector<Cluster>& clusters, TFile* clusters_file, std::string view) {
    // File is already open and managed by caller
    if (!clusters_file || clusters_file->IsZombie()) {
        LogError << "Invalid TFile pointer provided to write_clusters" << std::endl;
        return;
    }
    // Use the current directory (set by caller with cd())
    TDirectory* clusters_dir = gDirectory;
    
    // Check if tree exists and if it has the new momentum branches
    TTree *old_tree = (TTree*)clusters_dir->Get(Form("clusters_tree_%s", view.c_str()));
    TTree *clusters_tree = nullptr;
    
    if (old_tree) {
        bool has_momentum = (old_tree->GetBranch("true_mom_x") != nullptr);
        if (!has_momentum) {
            // Old tree doesn't have momentum - create a new tree which will overwrite on Write()
            LogWarning << "Existing tree lacks momentum branches - creating new tree with updated structure" << std::endl;
            clusters_tree = nullptr; // Force creation of new tree below
        } else {
            clusters_tree = old_tree; // Use existing tree
        }
    }

    int event;
    int n_tps;
    float true_pos_x;
    float true_pos_y;
    float true_pos_z;
    float true_neutrino_mom_x;
    float true_neutrino_mom_y;
    float true_neutrino_mom_z;
    float true_neutrino_energy;
    float true_particle_energy;
    std::string true_label;
    std::string *true_label_point = nullptr;
    bool is_es_interaction;
    float true_mom_x;
    float true_mom_y;
    float true_mom_z;
    float supernova_tp_fraction;
    float generator_tp_fraction;
    float marley_tp_fraction;
    double total_charge;
    double total_energy;    
    int true_pdg;
    bool is_main_cluster;
    int cluster_id;
    
    // TP information (allocate vectors once and reuse/clear per entry)
    std::vector<int>* tp_detector_channel = new std::vector<int>();
    std::vector<int>* tp_detector = new std::vector<int>();
    std::vector<int>* tp_samples_over_threshold = new std::vector<int>();
    std::vector<int>* tp_time_start = new std::vector<int>();
    std::vector<int>* tp_samples_to_peak = new std::vector<int>();
    std::vector<int>* tp_adc_peak = new std::vector<int>();
    std::vector<int>* tp_adc_integral = new std::vector<int>();
    std::vector<double>* tp_simide_energy = new std::vector<double>();

    if (!clusters_tree) {
        clusters_tree = new TTree(Form("clusters_tree_%s", view.c_str()), "Tree of clusters");
        if (verboseMode) LogInfo << "Tree not found, creating it" << std::endl;
        // create the branches
        clusters_tree->Branch("event", &event, "event/I");
        clusters_tree->Branch("n_tps", &n_tps, "n_tps/I");
        clusters_tree->Branch("true_pos_x", &true_pos_x, "true_pos_x/F");
        clusters_tree->Branch("true_pos_y", &true_pos_y, "true_pos_y/F");
        clusters_tree->Branch("true_pos_z", &true_pos_z, "true_pos_z/F");
        clusters_tree->Branch("true_neutrino_mom_x", &true_neutrino_mom_x, "true_neutrino_mom_x/F");
        clusters_tree->Branch("true_neutrino_mom_y", &true_neutrino_mom_y, "true_neutrino_mom_y/F");
        clusters_tree->Branch("true_neutrino_mom_z", &true_neutrino_mom_z, "true_neutrino_mom_z/F");
        clusters_tree->Branch("true_mom_x", &true_mom_x, "true_mom_x/F");
        clusters_tree->Branch("true_mom_y", &true_mom_y, "true_mom_y/F");
        clusters_tree->Branch("true_mom_z", &true_mom_z, "true_mom_z/F");
        clusters_tree->Branch("true_neutrino_energy", &true_neutrino_energy, "true_neutrino_energy/F");
        clusters_tree->Branch("true_particle_energy", &true_particle_energy, "true_particle_energy/F");
        clusters_tree->Branch("true_label", &true_label);
        clusters_tree->Branch("supernova_tp_fraction", &supernova_tp_fraction, "supernova_tp_fraction/F");
        clusters_tree->Branch("generator_tp_fraction", &generator_tp_fraction, "generator_tp_fraction/F");
        clusters_tree->Branch("marley_tp_fraction", &marley_tp_fraction, "marley_tp_fraction/F");
        clusters_tree->Branch("is_es_interaction", &is_es_interaction, "is_es_interaction/O");
        clusters_tree->Branch("total_charge", &total_charge, "total_charge/D");
        clusters_tree->Branch("total_energy", &total_energy, "total_energy/D");
        clusters_tree->Branch("true_pdg", &true_pdg, "true_pdg/I");
        clusters_tree->Branch("is_main_cluster", &is_main_cluster, "is_main_cluster/O");
        clusters_tree->Branch("cluster_id", &cluster_id, "cluster_id/I");

        clusters_tree->Branch("tp_detector_channel", &tp_detector_channel);
        clusters_tree->Branch("tp_detector", &tp_detector);
        clusters_tree->Branch("tp_samples_over_threshold", &tp_samples_over_threshold);
        clusters_tree->Branch("tp_samples_to_peak", &tp_samples_to_peak);
        clusters_tree->Branch("tp_time_start", &tp_time_start);
        clusters_tree->Branch("tp_adc_peak", &tp_adc_peak);
        clusters_tree->Branch("tp_adc_integral", &tp_adc_integral);
        clusters_tree->Branch("tp_simide_energy", &tp_simide_energy);

        // LogInfo << "Tree created" << std::endl;
    }
    else {
        // Existing tree: bind branch addresses to current output variables so Fill() works
        true_label_point = &true_label; // point to stack string for fill
        clusters_tree->SetBranchAddress("event", &event);
        clusters_tree->SetBranchAddress("n_tps", &n_tps);
        clusters_tree->SetBranchAddress("true_pos_x", &true_pos_x);
        clusters_tree->SetBranchAddress("true_pos_y", &true_pos_y);
        clusters_tree->SetBranchAddress("true_pos_z", &true_pos_z);
        clusters_tree->SetBranchAddress("true_neutrino_mom_x", &true_neutrino_mom_x);
        clusters_tree->SetBranchAddress("true_neutrino_mom_y", &true_neutrino_mom_y);
        clusters_tree->SetBranchAddress("true_neutrino_mom_z", &true_neutrino_mom_z);
        clusters_tree->SetBranchAddress("true_mom_x", &true_mom_x);
        clusters_tree->SetBranchAddress("true_mom_y", &true_mom_y);
        clusters_tree->SetBranchAddress("true_mom_z", &true_mom_z);
        clusters_tree->SetBranchAddress("true_neutrino_energy", &true_neutrino_energy);
        clusters_tree->SetBranchAddress("true_particle_energy", &true_particle_energy);
        clusters_tree->SetBranchAddress("true_label", &true_label_point);
        clusters_tree->SetBranchAddress("supernova_tp_fraction", &supernova_tp_fraction);
        clusters_tree->SetBranchAddress("generator_tp_fraction", &generator_tp_fraction);
        clusters_tree->SetBranchAddress("is_es_interaction", &is_es_interaction);
        clusters_tree->SetBranchAddress("total_charge", &total_charge);
        clusters_tree->SetBranchAddress("total_energy", &total_energy);
        clusters_tree->SetBranchAddress("true_pdg", &true_pdg);
        clusters_tree->SetBranchAddress("is_main_cluster", &is_main_cluster);
        if (clusters_tree->GetBranch("cluster_id")) {
            clusters_tree->SetBranchAddress("cluster_id", &cluster_id);
        }
        clusters_tree->SetBranchAddress("tp_detector_channel", &tp_detector_channel);
        clusters_tree->SetBranchAddress("tp_detector", &tp_detector);
        clusters_tree->SetBranchAddress("tp_samples_over_threshold", &tp_samples_over_threshold);
        clusters_tree->SetBranchAddress("tp_samples_to_peak", &tp_samples_to_peak);
        clusters_tree->SetBranchAddress("tp_time_start", &tp_time_start);
        clusters_tree->SetBranchAddress("tp_adc_peak", &tp_adc_peak);
        clusters_tree->SetBranchAddress("tp_adc_integral", &tp_adc_integral);
        // Optional: only set if branch exists (for backward compatibility)
        if (clusters_tree->GetBranch("tp_simide_energy")) {
            clusters_tree->SetBranchAddress("tp_simide_energy", &tp_simide_energy);
        }
    }
    

    // fill the tree
    for (auto& Cluster : clusters) {
        event = Cluster.get_event();
        n_tps = Cluster.get_size();
        true_pos_x = Cluster.get_true_pos()[0];
        true_pos_y = Cluster.get_true_pos()[1];
        true_pos_z = Cluster.get_true_pos()[2];
        true_neutrino_mom_x = Cluster.get_true_neutrino_momentum()[0];
        true_neutrino_mom_y = Cluster.get_true_neutrino_momentum()[1];
        true_neutrino_mom_z = Cluster.get_true_neutrino_momentum()[2];
        true_mom_x = Cluster.get_true_momentum()[0];
        true_mom_y = Cluster.get_true_momentum()[1];
        true_mom_z = Cluster.get_true_momentum()[2];
        true_neutrino_energy = Cluster.get_true_neutrino_energy();
        true_particle_energy = Cluster.get_true_particle_energy();
        true_label = Cluster.get_true_label();
        true_label_point = &true_label;
        supernova_tp_fraction = Cluster.get_supernova_tp_fraction();
        // Compute fraction of TPs in this Cluster with a non-UNKNOWN generator
        // Also compute marley-specific fraction
        int cluster_truth_count = 0;
        int marley_count = 0;
        {
            const auto& cl_tps = Cluster.get_tps();
            for (auto* tp : cl_tps) {
                std::string gen_name = tp->GetGeneratorName();
                if (gen_name != "UNKNOWN") {
                    cluster_truth_count++;
                }
                // Case-insensitive check for MARLEY
                std::string gen_lower = gen_name;
                std::transform(gen_lower.begin(), gen_lower.end(), gen_lower.begin(), ::tolower);
                if (gen_lower.find("marley") != std::string::npos) {
                    marley_count++;
                }
            }
            generator_tp_fraction = cl_tps.empty() ? 0.f : static_cast<float>(cluster_truth_count) / static_cast<float>(cl_tps.size());
            marley_tp_fraction = cl_tps.empty() ? 0.f : static_cast<float>(marley_count) / static_cast<float>(cl_tps.size());
            // std::cout << "Fraction of TPs with non-null generator: " << generator_tp_fraction << std::endl;
        }
        
        // If TPs don't have truth info (cluster_truth_count==0), use the cluster's stored value instead
        if (cluster_truth_count == 0) {
            marley_tp_fraction = Cluster.get_supernova_tp_fraction();
            generator_tp_fraction = Cluster.get_generator_tp_fraction();
        }
        
        is_es_interaction = Cluster.get_is_es_interaction();
        total_charge = Cluster.get_total_charge();
        total_energy = Cluster.get_total_energy();
        true_pdg = Cluster.get_true_pdg();
        is_main_cluster = Cluster.get_is_main_cluster();
        cluster_id = Cluster.get_cluster_id();
        // TODO create different tree for metadata? Currently in filename

        if (tp_detector_channel) tp_detector_channel->clear();
        if (tp_detector) tp_detector->clear();
    if (tp_samples_over_threshold) tp_samples_over_threshold->clear();
    if (tp_samples_to_peak) tp_samples_to_peak->clear();
        if (tp_time_start) tp_time_start->clear();
        if (tp_samples_to_peak) tp_samples_to_peak->clear();
        if (tp_adc_peak) tp_adc_peak->clear();
        if (tp_adc_integral) tp_adc_integral->clear();
        if (tp_simide_energy) tp_simide_energy->clear();
        for (auto& tp : Cluster.get_tps()) {
            tp_detector_channel->push_back(tp->GetDetectorChannel());
            tp_detector->push_back(tp->GetDetector());
            tp_samples_over_threshold->push_back(tp->GetSamplesOverThreshold());
            tp_time_start->push_back(tp->GetTimeStart());
            tp_samples_to_peak->push_back(tp->GetSamplesToPeak());
            tp_adc_peak->push_back(tp->GetAdcPeak());
            tp_adc_integral->push_back(tp->GetAdcIntegral());
            tp_simide_energy->push_back(tp->GetSimideEnergy());
            if (debugMode && tp_samples_to_peak->size() <= 5) {
                LogDebug << "[write_clusters_with_match_id] cluster_id=" << cluster_id
                         << " tp_index=" << (tp_samples_to_peak->size() - 1)
                         << " samples_to_peak=" << tp->GetSamplesToPeak()
                         << " adc_peak=" << tp->GetAdcPeak() << std::endl;
            }
        }
        clusters_tree->Fill();
    }
    // write the tree
    // Write inside 'clusters' directory
    clusters_dir->cd();
    clusters_tree->Write("", TObject::kOverwrite);
    // File close is managed by caller

    return;   
}

void write_clusters_with_match_id(std::vector<Cluster>& clusters, std::map<int, int>& cluster_to_match, TFile* clusters_file, std::string view,
                                   std::map<int, int>* x_to_u_map, std::map<int, int>* x_to_v_map) {
    // Similar to write_clusters but adds match_id and match_type branches
    // For X plane, also adds matching_clusterId_U and matching_clusterId_V
    if (!clusters_file || clusters_file->IsZombie()) {
        LogError << "Invalid TFile pointer provided to write_clusters_with_match_id" << std::endl;
        return;
    }
    
    // Use the current directory (set by caller with cd())
    TDirectory* clusters_dir = gDirectory;
    
    TTree *clusters_tree = new TTree(Form("clusters_tree_%s", view.c_str()), "Tree of clusters with match info");
    
    int event;
    int n_tps;
    float true_pos_x, true_pos_y, true_pos_z;
    float true_neutrino_mom_x, true_neutrino_mom_y, true_neutrino_mom_z;
    float true_neutrino_energy, true_particle_energy;
    std::string true_label;
    bool is_es_interaction;
    float true_mom_x, true_mom_y, true_mom_z;
    float supernova_tp_fraction, generator_tp_fraction, marley_tp_fraction;
    double total_charge, total_energy;
    int true_pdg;
    bool is_main_cluster;
    int cluster_id;
    int match_id;
    int match_type;
    int matching_clusterId_U;  // Only for X plane
    int matching_clusterId_V;  // Only for X plane
    
    std::vector<int>* tp_detector_channel = new std::vector<int>();
    std::vector<int>* tp_detector = new std::vector<int>();
    std::vector<int>* tp_samples_over_threshold = new std::vector<int>();
    std::vector<int>* tp_time_start = new std::vector<int>();
    std::vector<int>* tp_samples_to_peak = new std::vector<int>();
    std::vector<int>* tp_adc_peak = new std::vector<int>();
    std::vector<int>* tp_adc_integral = new std::vector<int>();
    std::vector<double>* tp_simide_energy = new std::vector<double>();

    // Create branches (including match info)
    clusters_tree->Branch("event", &event, "event/I");
    clusters_tree->Branch("n_tps", &n_tps, "n_tps/I");
    clusters_tree->Branch("true_pos_x", &true_pos_x, "true_pos_x/F");
    clusters_tree->Branch("true_pos_y", &true_pos_y, "true_pos_y/F");
    clusters_tree->Branch("true_pos_z", &true_pos_z, "true_pos_z/F");
    clusters_tree->Branch("true_neutrino_mom_x", &true_neutrino_mom_x, "true_neutrino_mom_x/F");
    clusters_tree->Branch("true_neutrino_mom_y", &true_neutrino_mom_y, "true_neutrino_mom_y/F");
    clusters_tree->Branch("true_neutrino_mom_z", &true_neutrino_mom_z, "true_neutrino_mom_z/F");
    clusters_tree->Branch("true_mom_x", &true_mom_x, "true_mom_x/F");
    clusters_tree->Branch("true_mom_y", &true_mom_y, "true_mom_y/F");
    clusters_tree->Branch("true_mom_z", &true_mom_z, "true_mom_z/F");
    clusters_tree->Branch("true_neutrino_energy", &true_neutrino_energy, "true_neutrino_energy/F");
    clusters_tree->Branch("true_particle_energy", &true_particle_energy, "true_particle_energy/F");
    clusters_tree->Branch("true_label", &true_label);
    clusters_tree->Branch("supernova_tp_fraction", &supernova_tp_fraction, "supernova_tp_fraction/F");
    clusters_tree->Branch("generator_tp_fraction", &generator_tp_fraction, "generator_tp_fraction/F");
    clusters_tree->Branch("marley_tp_fraction", &marley_tp_fraction, "marley_tp_fraction/F");
    clusters_tree->Branch("is_es_interaction", &is_es_interaction, "is_es_interaction/O");
    clusters_tree->Branch("total_charge", &total_charge, "total_charge/D");
    clusters_tree->Branch("total_energy", &total_energy, "total_energy/D");
    clusters_tree->Branch("true_pdg", &true_pdg, "true_pdg/I");
    clusters_tree->Branch("is_main_cluster", &is_main_cluster, "is_main_cluster/O");
    clusters_tree->Branch("cluster_id", &cluster_id, "cluster_id/I");
    clusters_tree->Branch("match_id", &match_id, "match_id/I");
    clusters_tree->Branch("match_type", &match_type, "match_type/I");
    
    // Add matching cluster ID branches only for X plane
    if (view == "X" && x_to_u_map && x_to_v_map) {
        clusters_tree->Branch("matching_clusterId_U", &matching_clusterId_U, "matching_clusterId_U/I");
        clusters_tree->Branch("matching_clusterId_V", &matching_clusterId_V, "matching_clusterId_V/I");
    }
    
    clusters_tree->Branch("tp_detector_channel", &tp_detector_channel);
    clusters_tree->Branch("tp_detector", &tp_detector);
    clusters_tree->Branch("tp_samples_over_threshold", &tp_samples_over_threshold);
    clusters_tree->Branch("tp_samples_to_peak", &tp_samples_to_peak);
    clusters_tree->Branch("tp_time_start", &tp_time_start);
    clusters_tree->Branch("tp_adc_peak", &tp_adc_peak);
    clusters_tree->Branch("tp_adc_integral", &tp_adc_integral);
    clusters_tree->Branch("tp_simide_energy", &tp_simide_energy);

    // Fill the tree
    for (auto& Cluster : clusters) {
        event = Cluster.get_event();
        n_tps = Cluster.get_size();
        true_pos_x = Cluster.get_true_pos()[0];
        true_pos_y = Cluster.get_true_pos()[1];
        true_pos_z = Cluster.get_true_pos()[2];
        true_neutrino_mom_x = Cluster.get_true_neutrino_momentum()[0];
        true_neutrino_mom_y = Cluster.get_true_neutrino_momentum()[1];
        true_neutrino_mom_z = Cluster.get_true_neutrino_momentum()[2];
        true_mom_x = Cluster.get_true_momentum()[0];
        true_mom_y = Cluster.get_true_momentum()[1];
        true_mom_z = Cluster.get_true_momentum()[2];
        true_neutrino_energy = Cluster.get_true_neutrino_energy();
        true_particle_energy = Cluster.get_true_particle_energy();
        true_label = Cluster.get_true_label();
        
        // Compute fractions
        int cluster_truth_count = 0;
        int marley_count = 0;
        {
            const auto& cl_tps = Cluster.get_tps();
            for (auto* tp : cl_tps) {
                std::string gen_name = tp->GetGeneratorName();
                if (gen_name != "UNKNOWN") {
                    cluster_truth_count++;
                }
                std::string gen_lower = gen_name;
                std::transform(gen_lower.begin(), gen_lower.end(), gen_lower.begin(), ::tolower);
                if (gen_lower.find("marley") != std::string::npos) {
                    marley_count++;
                }
            }
            generator_tp_fraction = cl_tps.empty() ? 0.f : static_cast<float>(cluster_truth_count) / static_cast<float>(cl_tps.size());
            marley_tp_fraction = cl_tps.empty() ? 0.f : static_cast<float>(marley_count) / static_cast<float>(cl_tps.size());
        }
        
        // If TPs don't have truth info, use cluster's stored value
        if (cluster_truth_count == 0) {
            marley_tp_fraction = Cluster.get_supernova_tp_fraction();
            generator_tp_fraction = Cluster.get_generator_tp_fraction();
        }
        
        supernova_tp_fraction = Cluster.get_supernova_tp_fraction();
        is_es_interaction = Cluster.get_is_es_interaction();
        total_charge = Cluster.get_total_charge();
        total_energy = Cluster.get_total_energy();
        true_pdg = Cluster.get_true_pdg();
        is_main_cluster = Cluster.get_is_main_cluster();
        cluster_id = Cluster.get_cluster_id();
        
        // Set match info
        auto it = cluster_to_match.find(cluster_id);
        if (it != cluster_to_match.end()) {
            match_id = it->second;
            match_type = 3;  // Currently only 3-plane matches
        } else {
            match_id = -1;
            match_type = -1;  // No match
        }
        
        // Set matching cluster IDs for X plane
        if (view == "X" && x_to_u_map && x_to_v_map) {
            auto u_it = x_to_u_map->find(cluster_id);
            matching_clusterId_U = (u_it != x_to_u_map->end()) ? u_it->second : -1;
            
            auto v_it = x_to_v_map->find(cluster_id);
            matching_clusterId_V = (v_it != x_to_v_map->end()) ? v_it->second : -1;
        }

        // Fill TP vectors
        if (tp_detector_channel) tp_detector_channel->clear();
        if (tp_detector) tp_detector->clear();
        if (tp_samples_over_threshold) tp_samples_over_threshold->clear();
        if (tp_time_start) tp_time_start->clear();
        if (tp_samples_to_peak) tp_samples_to_peak->clear();
        if (tp_adc_peak) tp_adc_peak->clear();
        if (tp_adc_integral) tp_adc_integral->clear();
        if (tp_simide_energy) tp_simide_energy->clear();
        
        for (auto& tp : Cluster.get_tps()) {
            tp_detector_channel->push_back(tp->GetDetectorChannel());
            tp_detector->push_back(tp->GetDetector());
            tp_samples_over_threshold->push_back(tp->GetSamplesOverThreshold());
            tp_time_start->push_back(tp->GetTimeStart());
            tp_samples_to_peak->push_back(tp->GetSamplesToPeak());
            tp_adc_peak->push_back(tp->GetAdcPeak());
            tp_adc_integral->push_back(tp->GetAdcIntegral());
            tp_simide_energy->push_back(tp->GetSimideEnergy());
        }
        clusters_tree->Fill();
    }
    
    // Write tree
    clusters_dir->cd();
    clusters_tree->Write("", TObject::kOverwrite);
    
    // Clean up vectors
    delete tp_detector_channel;
    delete tp_detector;
    delete tp_samples_over_threshold;
    delete tp_time_start;
    delete tp_samples_to_peak;
    delete tp_adc_peak;
    delete tp_adc_integral;
    delete tp_simide_energy;
    
    return;
}

std::vector<Cluster> read_clusters(std::string root_filename){
    if (verboseMode) LogInfo << "Reading clusters from: " << root_filename << std::endl;
    std::vector<Cluster> clusters;
    TFile *f = TFile::Open(root_filename.c_str());
    if (!f || f->IsZombie()) {
        LogError << "Cannot open file: " << root_filename << std::endl;
        return clusters;
    }
    
    // Find clusters directory
    TDirectory* clusters_dir = dynamic_cast<TDirectory*>(f->Get("clusters"));
    if (!clusters_dir) {
        LogWarning << "No 'clusters' directory found, trying file root" << std::endl;
        clusters_dir = f;
    }
    
    // Iterate through all trees in the directory
    TIter nextKey(clusters_dir->GetListOfKeys());
    TKey* key;
    while ((key = (TKey*)nextKey())) {
        if (std::string(key->GetClassName()) != "TTree") continue;
        
        TTree* tree = dynamic_cast<TTree*>(key->ReadObj());
        if (!tree) continue;
        
        if (verboseMode) LogInfo << "  Found tree: " << tree->GetName() << " with " << tree->GetEntries() << " entries" << std::endl;
        
    // Set up branch addresses for CURRENT schema (as written by write_clusters)
    Int_t event = 0;
    Int_t n_tps = 0;
    Float_t true_pos_x = 0, true_pos_y = 0, true_pos_z = 0;
    Float_t true_neutrino_mom_x = 0, true_neutrino_mom_y = 0, true_neutrino_mom_z = 0;
    Float_t true_mom_x = 0, true_mom_y = 0, true_mom_z = 0;
    Float_t true_neutrino_energy = 0;
    Float_t true_particle_energy = 0;
    std::string* true_label = nullptr;
    Bool_t is_es_interaction = false;
    Int_t true_pdg = 0;
    Bool_t is_main_cluster = false;
    Int_t cluster_id = -1;
    std::vector<int>* tp_channel = nullptr;
    std::vector<int>* tp_detector = nullptr;
    std::vector<int>* tp_time_start = nullptr;
    std::vector<int>* tp_s_over = nullptr;
    std::vector<int>* tp_adc_integral = nullptr;
    
    if (tree->GetBranch("event")) tree->SetBranchAddress("event", &event);
    if (tree->GetBranch("n_tps")) tree->SetBranchAddress("n_tps", &n_tps);
    if (tree->GetBranch("true_pos_x")) tree->SetBranchAddress("true_pos_x", &true_pos_x);
    if (tree->GetBranch("true_pos_y")) tree->SetBranchAddress("true_pos_y", &true_pos_y);
    if (tree->GetBranch("true_pos_z")) tree->SetBranchAddress("true_pos_z", &true_pos_z);
    if (tree->GetBranch("true_neutrino_mom_x")) tree->SetBranchAddress("true_neutrino_mom_x", &true_neutrino_mom_x);
    if (tree->GetBranch("true_neutrino_mom_y")) tree->SetBranchAddress("true_neutrino_mom_y", &true_neutrino_mom_y);
    if (tree->GetBranch("true_neutrino_mom_z")) tree->SetBranchAddress("true_neutrino_mom_z", &true_neutrino_mom_z);
    if (tree->GetBranch("true_mom_x")) tree->SetBranchAddress("true_mom_x", &true_mom_x);
    if (tree->GetBranch("true_mom_y")) tree->SetBranchAddress("true_mom_y", &true_mom_y);
    if (tree->GetBranch("true_mom_z")) tree->SetBranchAddress("true_mom_z", &true_mom_z);
    if (tree->GetBranch("true_neutrino_energy")) tree->SetBranchAddress("true_neutrino_energy", &true_neutrino_energy);
    if (tree->GetBranch("true_particle_energy")) tree->SetBranchAddress("true_particle_energy", &true_particle_energy);
    if (tree->GetBranch("true_label")) tree->SetBranchAddress("true_label", &true_label);
    if (tree->GetBranch("is_es_interaction")) tree->SetBranchAddress("is_es_interaction", &is_es_interaction);
    if (tree->GetBranch("true_pdg")) tree->SetBranchAddress("true_pdg", &true_pdg);
    if (tree->GetBranch("is_main_cluster")) tree->SetBranchAddress("is_main_cluster", &is_main_cluster);
    if (tree->GetBranch("cluster_id")) tree->SetBranchAddress("cluster_id", &cluster_id);
    if (tree->GetBranch("tp_detector")) tree->SetBranchAddress("tp_detector", &tp_detector);
    if (tree->GetBranch("tp_detector_channel")) tree->SetBranchAddress("tp_detector_channel", &tp_channel);
    if (tree->GetBranch("tp_time_start")) tree->SetBranchAddress("tp_time_start", &tp_time_start);
    if (tree->GetBranch("tp_samples_over_threshold")) tree->SetBranchAddress("tp_samples_over_threshold", &tp_s_over);
    if (tree->GetBranch("tp_adc_integral")) tree->SetBranchAddress("tp_adc_integral", &tp_adc_integral);        // Read all entries
        for (Long64_t i = 0; i < tree->GetEntries(); i++) {
            tree->GetEntry(i);
            
            if (!tp_channel || tp_channel->empty()) {
                if (debugMode) LogDebug << "    Skipping entry " << i << " (no TPs)" << std::endl;
                continue;
            }
            
            if (verboseMode) LogInfo << "    Entry " << i << ": " << tp_channel->size() << " TPs, event " << event << std::endl;
            
            // Create TriggerPrimitives from the vectors
            std::vector<TriggerPrimitive*> tps;
            for (size_t j = 0; j < tp_channel->size(); j++) {
                int channel = (*tp_channel)[j];
                int time_start = (*tp_time_start)[j];
                int s_over_threshold = (*tp_s_over)[j];
                int adc_integral = (*tp_adc_integral)[j];
                
                // Create TP (version=0, flag=0, detid=0 are defaults, adc_peak=0, samples_to_peak=0)
                TriggerPrimitive* tp = new TriggerPrimitive(0, 0, 0, channel, s_over_threshold, time_start, 0, adc_integral, 0);
                tp->SetEvent(event);
                
                // Determine view from tree name
                std::string tree_name = tree->GetName();
                if (tree_name.find("_X") != std::string::npos) {
                    tp->SetView(0); // Collection
                } else if (tree_name.find("_U") != std::string::npos) {
                    tp->SetView(1); // Induction U
                } else if (tree_name.find("_V") != std::string::npos) {
                    tp->SetView(2); // Induction V
                }
                
                tps.push_back(tp);
            }
            
            if (verboseMode) LogInfo << "    Creating cluster from " << tps.size() << " TPs..." << std::endl;
            
            // Create cluster
            Cluster cluster(tps);
            cluster.set_is_main_cluster(is_main_cluster);
            cluster.set_cluster_id(cluster_id);
            cluster.set_true_neutrino_energy(true_neutrino_energy);
            cluster.set_true_particle_energy(true_particle_energy);
            cluster.set_true_pos({true_pos_x, true_pos_y, true_pos_z});
            cluster.set_true_neutrino_momentum({true_neutrino_mom_x, true_neutrino_mom_y, true_neutrino_mom_z});
            cluster.set_true_momentum({true_mom_x, true_mom_y, true_mom_z});
            if (true_label) cluster.set_true_label(*true_label);
            cluster.set_is_es_interaction(is_es_interaction);
            cluster.set_true_pdg(true_pdg);
            
            clusters.push_back(cluster);
        }
    }
    
    f->Close();
    if (verboseMode) LogInfo << "  Read " << clusters.size() << " total clusters from all trees" << std::endl;
    return clusters;
}

std::vector<Cluster> read_clusters_from_tree(std::string root_filename, std::string view, std::string directory){
    LogInfo << "Reading " << view << " clusters from: " << root_filename << " (directory: " << directory << ")" << std::endl;
    std::vector<Cluster> clusters;
    TFile *f = TFile::Open(root_filename.c_str());
    if (!f || f->IsZombie()) {
        LogError << "Cannot open file: " << root_filename << std::endl;
        return clusters;
    }
    
    // Find specified directory
    TDirectory* clusters_dir = dynamic_cast<TDirectory*>(f->Get(directory.c_str()));
    if (!clusters_dir) {
        LogWarning << "No '" << directory << "' directory found, trying file root" << std::endl;
        clusters_dir = f;
    }
    
    // Look for specific tree by view name
    std::string tree_name = "clusters_tree_" + view;
    TTree* tree = dynamic_cast<TTree*>(clusters_dir->Get(tree_name.c_str()));
    if (!tree) {
        LogError << "  Tree " << tree_name << " not found in file" << std::endl;
        f->Close();
        delete f;
        return clusters;
    }
    
    if (verboseMode) LogInfo << "  Found tree: " << tree->GetName() << " with " << tree->GetEntries() << " entries" << std::endl;
    
    // Set up branch addresses for CURRENT schema (as written by write_clusters)
    Int_t event = 0;
    Int_t n_tps = 0;
    Float_t true_pos_x = 0, true_pos_y = 0, true_pos_z = 0;
    Float_t true_neutrino_mom_x = 0, true_neutrino_mom_y = 0, true_neutrino_mom_z = 0;
    Float_t true_mom_x = 0, true_mom_y = 0, true_mom_z = 0;
    Float_t true_neutrino_energy = 0;
    Float_t true_particle_energy = 0;
    Float_t marley_tp_fraction = 0;
    std::string* true_label = nullptr;
    Bool_t is_main_cluster = false;
    Bool_t is_es_interaction = false;
    Int_t true_pdg = 0;
    Int_t cluster_id = -1;
    std::vector<int>* tp_channel = nullptr;
    std::vector<int>* tp_detector = nullptr;
    std::vector<int>* tp_time_start = nullptr;
    std::vector<int>* tp_s_over = nullptr;
    std::vector<int>* tp_samples_to_peak = nullptr;
    std::vector<int>* tp_adc_peak = nullptr;
    std::vector<int>* tp_adc_integral = nullptr;
    std::vector<double>* tp_simide_energy = nullptr;
    
    if (tree->GetBranch("event")) tree->SetBranchAddress("event", &event);
    if (tree->GetBranch("n_tps")) tree->SetBranchAddress("n_tps", &n_tps);
    if (tree->GetBranch("true_pos_x")) tree->SetBranchAddress("true_pos_x", &true_pos_x);
    if (tree->GetBranch("true_pos_y")) tree->SetBranchAddress("true_pos_y", &true_pos_y);
    if (tree->GetBranch("true_pos_z")) tree->SetBranchAddress("true_pos_z", &true_pos_z);
    if (tree->GetBranch("true_neutrino_mom_x")) tree->SetBranchAddress("true_neutrino_mom_x", &true_neutrino_mom_x);
    if (tree->GetBranch("true_neutrino_mom_y")) tree->SetBranchAddress("true_neutrino_mom_y", &true_neutrino_mom_y);
    if (tree->GetBranch("true_neutrino_mom_z")) tree->SetBranchAddress("true_neutrino_mom_z", &true_neutrino_mom_z);
    if (tree->GetBranch("true_mom_x")) tree->SetBranchAddress("true_mom_x", &true_mom_x);
    if (tree->GetBranch("true_mom_y")) tree->SetBranchAddress("true_mom_y", &true_mom_y);
    if (tree->GetBranch("true_mom_z")) tree->SetBranchAddress("true_mom_z", &true_mom_z);
    if (tree->GetBranch("true_neutrino_energy")) tree->SetBranchAddress("true_neutrino_energy", &true_neutrino_energy);
    if (tree->GetBranch("true_particle_energy")) tree->SetBranchAddress("true_particle_energy", &true_particle_energy);
    if (tree->GetBranch("marley_tp_fraction")) tree->SetBranchAddress("marley_tp_fraction", &marley_tp_fraction);
    if (tree->GetBranch("true_label")) tree->SetBranchAddress("true_label", &true_label);
    if (tree->GetBranch("is_main_cluster")) tree->SetBranchAddress("is_main_cluster", &is_main_cluster);
    if (tree->GetBranch("is_es_interaction")) tree->SetBranchAddress("is_es_interaction", &is_es_interaction);
    if (tree->GetBranch("true_pdg")) tree->SetBranchAddress("true_pdg", &true_pdg);
    if (tree->GetBranch("cluster_id")) tree->SetBranchAddress("cluster_id", &cluster_id);
    if (tree->GetBranch("tp_detector")) tree->SetBranchAddress("tp_detector", &tp_detector);
    if (tree->GetBranch("tp_detector_channel")) tree->SetBranchAddress("tp_detector_channel", &tp_channel);
    if (tree->GetBranch("tp_time_start")) tree->SetBranchAddress("tp_time_start", &tp_time_start);
    if (tree->GetBranch("tp_samples_over_threshold")) tree->SetBranchAddress("tp_samples_over_threshold", &tp_s_over);
    if (tree->GetBranch("tp_samples_to_peak")) tree->SetBranchAddress("tp_samples_to_peak", &tp_samples_to_peak);
    if (tree->GetBranch("tp_adc_peak")) tree->SetBranchAddress("tp_adc_peak", &tp_adc_peak);
    if (tree->GetBranch("tp_adc_integral")) tree->SetBranchAddress("tp_adc_integral", &tp_adc_integral);
    if (tree->GetBranch("tp_simide_energy")) tree->SetBranchAddress("tp_simide_energy", &tp_simide_energy);
    
    // Read all entries
    for (Long64_t i = 0; i < tree->GetEntries(); i++) {
        tree->GetEntry(i);
        
        if (!tp_channel || tp_channel->empty()) {
            if (debugMode) LogDebug << "    Skipping entry " << i << " (no TPs)" << std::endl;
            continue;
        }
        
        if (verboseMode) LogInfo << "    Entry " << i << ": " << tp_channel->size() << " TPs, event " << event << ", cluster_id " << cluster_id << std::endl;
        
        // Create TriggerPrimitives from the vectors
        std::vector<TriggerPrimitive*> tps;
        for (size_t j = 0; j < tp_channel->size(); j++) {
            int channel = (*tp_channel)[j];
            int detector = (*tp_detector)[j];
            int time_start = (*tp_time_start)[j];
            int s_over_threshold = (*tp_s_over)[j];
            int samples_to_peak = 0;
            if (tp_samples_to_peak && j < tp_samples_to_peak->size()) {
                samples_to_peak = (*tp_samples_to_peak)[j];
            }
            int adc_peak = (tp_adc_peak && j < tp_adc_peak->size()) ? (*tp_adc_peak)[j] : 0;
            int adc_integral = (*tp_adc_integral)[j];
            double simide_energy = 0.0;
            if (tp_simide_energy && j < tp_simide_energy->size()) {
                simide_energy = (*tp_simide_energy)[j];
            }

            if (debugMode) {
                LogDebug << "      TP " << j
                         << " channel=" << channel
                         << " detector=" << detector
                         << " time_start=" << time_start
                         << " sot=" << s_over_threshold
                         << " samples_to_peak=" << samples_to_peak
                         << " adc_peak=" << adc_peak
                         << " adc_integral=" << adc_integral
                         << " simide_energy=" << simide_energy
                         << std::endl;
            }
            
            // Create TP - Note: event number set via SetEvent() after construction
            // because TriggerPrimitive constructor doesn't take event as parameter
            TriggerPrimitive* tp = new TriggerPrimitive(0, 0, 0, channel, s_over_threshold, time_start, samples_to_peak, adc_integral, adc_peak);
            
            // Set simide energy
            tp->SetSimideEnergy(simide_energy);
            
            // IMPORTANT: Set event BEFORE adding to vector to avoid Cluster constructor check failures
            tp->SetEvent(event);

            tp->SetDetector(detector);
            
            // Set view from parameter
            if (view == "X") {
                tp->SetView(0); // Collection
            } else if (view == "U") {
                tp->SetView(1); // Induction U
            } else if (view == "V") {
                tp->SetView(2); // Induction V
            }
            
            tps.push_back(tp);
        }
        
        if (verboseMode) LogInfo << "    Creating cluster from " << tps.size() << " TPs..." << std::endl;
        
        // Create cluster
        Cluster cluster(tps);
        cluster.set_is_main_cluster(is_main_cluster);
        cluster.set_cluster_id(cluster_id);
        cluster.set_supernova_tp_fraction(marley_tp_fraction);
        cluster.set_is_es_interaction(is_es_interaction);
        cluster.set_true_neutrino_energy(true_neutrino_energy);
        cluster.set_true_particle_energy(true_particle_energy);
        cluster.set_true_pos({true_pos_x, true_pos_y, true_pos_z});
        cluster.set_true_neutrino_momentum({true_neutrino_mom_x, true_neutrino_mom_y, true_neutrino_mom_z});
        cluster.set_true_momentum({true_mom_x, true_mom_y, true_mom_z});
        if (true_label) cluster.set_true_label(*true_label);
        cluster.set_true_pdg(true_pdg);
        
        clusters.push_back(cluster);
    }
    
    f->Close();
    delete f;
    
    LogInfo << "  Loaded " << clusters.size() << " " << view << " clusters" << std::endl;
    return clusters;
}

std::map<int, std::vector<Cluster>> create_event_mapping(std::vector<Cluster>& clusters){
    std::map<int, std::vector<Cluster>> event_mapping;
    for (auto& g : clusters) {
    // check if the event is already in the map
        if (event_mapping.find(g.get_tp(0)->GetEvent()) == event_mapping.end()) {
            std::vector<Cluster> temp;
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

