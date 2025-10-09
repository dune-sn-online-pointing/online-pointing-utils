#include "Clustering.h"

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

void read_tps(const std::string& in_filename, 
        std::map<int, std::vector<TriggerPrimitive>>& tps_by_event, 
        std::map<int, std::vector<TrueParticle>>& true_particles_by_event, 
        std::map<int, std::vector<Neutrino>>& neutrinos_by_event){
    
    if (verboseMode) LogInfo << "Reading TPs from: " << in_filename << std::endl;
    
    TFile inFile(in_filename.c_str(), "READ"); 
    if (inFile.IsZombie()) { LogError << "Cannot open: " << in_filename << std::endl; return; }
    TDirectory* tpsDir = inFile.GetDirectory("tps"); 
    if (!tpsDir) { LogError << "Directory 'tps' not found in: " << in_filename << std::endl; return; }

    // Read neutrinos
    if (auto* nuTree = dynamic_cast<TTree*>(tpsDir->Get("neutrinos"))) {
        int event=0; std::string* interaction=nullptr; 
        float x=0,y=0,z=0,Px=0,Py=0,Pz=0; 
        int en=0; int truth_id=0; 
        nuTree->SetBranchAddress("event", &event); 
        nuTree->SetBranchAddress("interaction", &interaction); 
        nuTree->SetBranchAddress("x", &x); 
        nuTree->SetBranchAddress("y", &y);
        nuTree->SetBranchAddress("z", &z); 
        nuTree->SetBranchAddress("Px", &Px); 
        nuTree->SetBranchAddress("Py", &Py); 
        nuTree->SetBranchAddress("Pz", &Pz); 
        nuTree->SetBranchAddress("en", &en); 
        nuTree->SetBranchAddress("truth_id", &truth_id);
        for (Long64_t i=0;i<nuTree->GetEntries();++i){ 
            nuTree->GetEntry(i); 
            Neutrino n(event, *interaction, x,y,z,Px,Py,Pz,en,truth_id); 
            neutrinos_by_event[event].push_back(n);
        }
    }

    // Read true particles
    std::map<std::pair<int,int>, Neutrino*> nuLookup; // (event, truth_id) -> neutrino ptr
    for (auto& kv : neutrinos_by_event) { for (auto& n : kv.second) { nuLookup[{kv.first, n.GetTruthId()}] = &n; } }

    if (auto* tTree = dynamic_cast<TTree*>(tpsDir->Get("true_particles"))) {
        int event = 0;
        float x = 0, y = 0, z = 0, Px = 0, Py = 0, Pz = 0, en = 0;
        std::string* gen = nullptr;
        int pdg = 0;
        std::string* proc = nullptr;
        int track_id = 0;
        int truth_id = 0;
        double tstart = 0, tend = 0;
        std::vector<int>* channels = nullptr;
        int nu_truth_id = -1;
        tTree->SetBranchAddress("event", &event); 
        tTree->SetBranchAddress("x", &x); 
        tTree->SetBranchAddress("y", &y); 
        tTree->SetBranchAddress("z", &z);
        tTree->SetBranchAddress("Px", &Px);
        tTree->SetBranchAddress("Py", &Py);
        tTree->SetBranchAddress("Pz", &Pz);
        tTree->SetBranchAddress("en", &en);
        tTree->SetBranchAddress("generator_name", &gen);
        tTree->SetBranchAddress("pdg", &pdg);
        tTree->SetBranchAddress("process", &proc);
        tTree->SetBranchAddress("track_id", &track_id);
        tTree->SetBranchAddress("truth_id", &truth_id);
        tTree->SetBranchAddress("time_start", &tstart);
        tTree->SetBranchAddress("time_end", &tend);
        tTree->SetBranchAddress("channels", &channels);
        tTree->SetBranchAddress("neutrino_truth_id", &nu_truth_id);

        for (Long64_t i=0;i<tTree->GetEntries();++i){ 
            tTree->GetEntry(i); 
            TrueParticle tp(event,x,y,z,Px,Py,Pz,en,*gen,pdg,*proc,track_id,truth_id); 
            tp.SetTimeStart(tstart);
            tp.SetTimeEnd(tend);
            if (channels){ for (int ch : *channels) tp.AddChannel(ch); } 
            auto it = nuLookup.find({event, nu_truth_id}); 
            if (it != nuLookup.end()) tp.SetNeutrino(it->second);
            true_particles_by_event[event].push_back(tp);
        }
    }

    // Build lookup for true particles
    std::map<std::pair<int,int>, TrueParticle*> truthLookup; // (event, truth_id)
    for (auto& kv : true_particles_by_event) 
        for (auto& tp : kv.second) 
            truthLookup[{kv.first, tp.GetTruthId()}] = &tp;

    // Read TPs
    if (auto* tpTree = dynamic_cast<TTree*>(tpsDir->Get("tps"))) {
        
        int event=0; 
        UShort_t version=0; 
        UInt_t detid=0, channel=0; 
        ULong64_t s_over=0, tstart=0, s_to_peak=0; 
        UInt_t adc_integral=0; 
        UShort_t adc_peak=0, det=0; 
        Int_t det_channel=0; 
        std::string* view=nullptr, *gen_name=nullptr, *process=nullptr;
        Int_t truth_id=-1, track_id=-1, pdg=0, nu_truth_id=-1; 
        Float_t nu_energy=-1.0f;
        
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
        tpTree->SetBranchAddress("truth_id", &truth_id);
        tpTree->SetBranchAddress("track_id", &track_id);
        tpTree->SetBranchAddress("pdg", &pdg);
        tpTree->SetBranchAddress("generator_name", &gen_name);
        tpTree->SetBranchAddress("process", &process);
        tpTree->SetBranchAddress("neutrino_truth_id", &nu_truth_id);
        tpTree->SetBranchAddress("neutrino_energy", &nu_energy);

        for (Long64_t i=0;i<tpTree->GetEntries();++i){ 

            tpTree->GetEntry(i); 

            TriggerPrimitive tp(version, 0, detid, channel, s_over, tstart, s_to_peak, adc_integral, adc_peak); 
            tp.SetEvent(event); 
            tp.SetDetector(det);
            tp.SetDetectorChannel(det_channel); // SetView already in constructor
            
            auto it = truthLookup.find({event, truth_id}); 
            if (it != truthLookup.end()) tp.SetTrueParticle(it->second); 
            tps_by_event[event].push_back(tp);
        }
    }

    inFile.Close();
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
std::vector<Cluster> make_cluster(std::vector<TriggerPrimitive*> all_tps, int ticks_limit, int channel_limit, int min_tps_to_cluster, int adc_integral_cut) {
    
    if (verboseMode) LogInfo << "Creating clusters from TPs" << std::endl;

    std::vector<std::vector<TriggerPrimitive*>> buffer;
    std::vector<Cluster> clusters;

    // Reset the buffer for each event
    buffer.clear();

    for (int iTP = 0; iTP < all_tps.size(); iTP++) {
        
        TriggerPrimitive* tp1 = all_tps.at(iTP);

        if (debugMode) LogInfo << "Processing TP: " << tp1->GetTimeStart() << " " << tp1->GetDetectorChannel() << std::endl;

        bool appended = false;

        for (auto& candidate : buffer) {
            // Calculate the maximum time in the current candidate
            double max_time = 0;
            for (auto& tp2 : candidate) {
                max_time = std::max(max_time, tp2->GetTimeStart() + tp2->GetSamplesOverThreshold() * TPC_sample_length);
            }

            // Check time and channel conditions
            if ((tp1->GetTimeStart() - max_time) <= ticks_limit) {
                for (auto& tp2 : candidate) {
                    if (channel_condition_with_pbc(tp1, tp2, channel_limit)) {
                        candidate.push_back(tp1);
                        appended = true;
                        if (debugMode) LogInfo << "Appended TP to candidate Cluster" << std::endl;
                        break;
                    }
                }
                if (appended) break;
                if (debugMode) LogInfo << "Not appended to candidate Cluster" << std::endl;
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
            if (adc_integral > adc_integral_cut) {
                // check validity of tps in candidate
                // LogInfo << "Candidate Cluster has " << candidate.size() << " TPs" << std::endl;
                clusters.emplace_back(Cluster(std::move(candidate)));
                // LogInfo << "Cluster created with " << clusters.back().get_tps().size() << " TPs" << std::endl;
            }
            else {
            }
        }
    }

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


void write_clusters(std::vector<Cluster>& clusters, std::string root_filename, std::string view) {
    // create folder if it does not exist
    std::string folder = root_filename.substr(0, root_filename.find_last_of("/"));
    if (!ensureDirectoryExists(folder)) {
        LogError << "Cannot create or access directory for clusters output: " << folder << std::endl;
        return;
    }
    // create the root file if it does not exist, otherwise just update it
    TFile *clusters_file = new TFile(root_filename.c_str(), "UPDATE"); // TODO handle better
    // Ensure a fixed TDirectory inside the ROOT file for Cluster trees
    TDirectory* clusters_dir = clusters_file->GetDirectory("clusters");
    if (!clusters_dir) clusters_dir = clusters_file->mkdir("clusters");
    clusters_dir->cd();
    
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
    float true_dir_x;
    float true_dir_y;
    float true_dir_z;
    float true_neutrino_energy;
    float true_particle_energy;
    std::string true_label;
    std::string *true_label_point = nullptr;
    std::string true_interaction;
    std::string *true_interaction_point = nullptr;
    float true_mom_x;
    float true_mom_y;
    float true_mom_z;
    float min_distance_from_true_pos;
    float supernova_tp_fraction;
    float generator_tp_fraction;
    double total_charge;
    double total_energy;    
    double conversion_factor;
    
    // TP information (allocate vectors once and reuse/clear per entry)
    std::vector<int>* tp_detector_channel = new std::vector<int>();
    std::vector<int>* tp_detector = new std::vector<int>();
    std::vector<int>* tp_samples_over_threshold = new std::vector<int>();
    std::vector<int>* tp_time_start = new std::vector<int>();
    std::vector<int>* tp_samples_to_peak = new std::vector<int>();
    std::vector<int>* tp_adc_peak = new std::vector<int>();
    std::vector<int>* tp_adc_integral = new std::vector<int>();

    if (!clusters_tree) {
        clusters_tree = new TTree(Form("clusters_tree_%s", view.c_str()), "Tree of clusters");
        if (verboseMode) LogInfo << "Tree not found, creating it" << std::endl;
        // create the branches
        clusters_tree->Branch("event", &event, "event/I");
        clusters_tree->Branch("n_tps", &n_tps, "n_tps/I");
        clusters_tree->Branch("true_pos_x", &true_pos_x, "true_pos_x/F");
        clusters_tree->Branch("true_pos_y", &true_pos_y, "true_pos_y/F");
        clusters_tree->Branch("true_pos_z", &true_pos_z, "true_pos_z/F");
        clusters_tree->Branch("true_dir_x", &true_dir_x, "true_dir_x/F");
        clusters_tree->Branch("true_dir_y", &true_dir_y, "true_dir_y/F");
        clusters_tree->Branch("true_dir_z", &true_dir_z, "true_dir_z/F");
        clusters_tree->Branch("true_mom_x", &true_mom_x, "true_mom_x/F");
        clusters_tree->Branch("true_mom_y", &true_mom_y, "true_mom_y/F");
        clusters_tree->Branch("true_mom_z", &true_mom_z, "true_mom_z/F");
        clusters_tree->Branch("true_neutrino_energy", &true_neutrino_energy, "true_neutrino_energy/F");
        clusters_tree->Branch("true_particle_energy", &true_particle_energy, "true_particle_energy/F");
        clusters_tree->Branch("true_label", &true_label);
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
    else {
        // Existing tree: bind branch addresses to current output variables so Fill() works
    true_label_point = &true_label; // point to stack string for fill
    true_interaction_point = &true_interaction; // point to stack string for fill
        clusters_tree->SetBranchAddress("event", &event);
        clusters_tree->SetBranchAddress("n_tps", &n_tps);
        clusters_tree->SetBranchAddress("true_pos_x", &true_pos_x);
        clusters_tree->SetBranchAddress("true_pos_y", &true_pos_y);
        clusters_tree->SetBranchAddress("true_pos_z", &true_pos_z);
        clusters_tree->SetBranchAddress("true_dir_x", &true_dir_x);
        clusters_tree->SetBranchAddress("true_dir_y", &true_dir_y);
        clusters_tree->SetBranchAddress("true_dir_z", &true_dir_z);
        clusters_tree->SetBranchAddress("true_mom_x", &true_mom_x);
        clusters_tree->SetBranchAddress("true_mom_y", &true_mom_y);
        clusters_tree->SetBranchAddress("true_mom_z", &true_mom_z);
        clusters_tree->SetBranchAddress("true_neutrino_energy", &true_neutrino_energy);
        clusters_tree->SetBranchAddress("true_particle_energy", &true_particle_energy);
    clusters_tree->SetBranchAddress("true_label", &true_label_point);
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
    for (auto& Cluster : clusters) {
        event = Cluster.get_event();
        n_tps = Cluster.get_size();
        true_pos_x = Cluster.get_true_pos()[0];
        true_pos_y = Cluster.get_true_pos()[1];
        true_pos_z = Cluster.get_true_pos()[2];
        true_dir_x = Cluster.get_true_dir()[0];
        true_dir_y = Cluster.get_true_dir()[1];
        true_dir_z = Cluster.get_true_dir()[2];
        true_mom_x = Cluster.get_true_momentum()[0];
        true_mom_y = Cluster.get_true_momentum()[1];
        true_mom_z = Cluster.get_true_momentum()[2];
        true_neutrino_energy = Cluster.get_true_neutrino_energy();
        true_particle_energy = Cluster.get_true_particle_energy();
        true_label = Cluster.get_true_label();
        true_label_point = &true_label;
        min_distance_from_true_pos = Cluster.get_min_distance_from_true_pos();
        supernova_tp_fraction = Cluster.get_supernova_tp_fraction();
        // Compute fraction of TPs in this Cluster with a non-UNKNOWN generator
        {
            int cluster_truth_count = 0;
            const auto& cl_tps = Cluster.get_tps();
            for (auto* tp : cl_tps) {
                auto* tpTruth = tp->GetTrueParticle();
                if (tpTruth != nullptr && tpTruth->GetGeneratorName() != "UNKNOWN") {
                    cluster_truth_count++;
                }
            }
            generator_tp_fraction = cl_tps.empty() ? 0.f : static_cast<float>(cluster_truth_count) / static_cast<float>(cl_tps.size());
            // std::cout << "Fraction of TPs with non-null generator: " << generator_tp_fraction << std::endl;
        }
        true_interaction = Cluster.get_true_interaction();
        true_interaction_point = &true_interaction;
        total_charge = Cluster.get_total_charge();
        total_energy = Cluster.get_total_energy();
        conversion_factor = adc_to_energy_conversion_factor; // should be in settings, still keep as metadata
        // TODO create different tree for metadata? Currently in filename

        if (tp_detector_channel) tp_detector_channel->clear();
        if (tp_detector) tp_detector->clear();
        if (tp_samples_over_threshold) tp_samples_over_threshold->clear();
        if (tp_time_start) tp_time_start->clear();
        if (tp_samples_to_peak) tp_samples_to_peak->clear();
        if (tp_adc_peak) tp_adc_peak->clear();
        if (tp_adc_integral) tp_adc_integral->clear();
        for (auto& tp : Cluster.get_tps()) {
            tp_detector_channel->push_back(tp->GetDetectorChannel());
            tp_detector->push_back(tp->GetDetector());
            tp_samples_over_threshold->push_back(tp->GetSamplesOverThreshold());
            tp_time_start->push_back(tp->GetTimeStart());
            tp_samples_to_peak->push_back(tp->GetSamplesToPeak());
            tp_adc_peak->push_back(tp->GetAdcPeak());
            tp_adc_integral->push_back(tp->GetAdcIntegral());
        }
        clusters_tree->Fill();
    }
    // write the tree
    // Write inside 'clusters' directory
    clusters_dir->cd();
    clusters_tree->Write("", TObject::kOverwrite);
    clusters_file->Close();

    return;   
}

std::vector<Cluster> read_clusters(std::string root_filename){
    LogInfo << "Reading clusters from: " << root_filename << std::endl;
    std::vector<Cluster> clusters;
    TFile *f = new TFile();
    f = TFile::Open(root_filename.c_str());
    // print the list of objects in the file
    f->ls();
    TTree *clusters_tree = nullptr;
    // Prefer the new in-file folder structure
    if (auto* dir = f->GetDirectory("clusters")) {
        dir->cd();
        TIter nextKey(dir->GetListOfKeys());
        while (TKey* key = (TKey*)nextKey()) {
            if (std::string(key->GetClassName()) == "TTree") {
                clusters_tree = dynamic_cast<TTree*>(key->ReadObj());
                if (clusters_tree) break;
            }
        }
    }
    // Fallback to legacy names if nothing found
    if (!clusters_tree) {
        clusters_tree = (TTree*)f->Get("clusters_tree_X");
    }
    if (!clusters_tree) {
        clusters_tree = (TTree*)f->Get("clusters_tree_U");
    }
    if (!clusters_tree) {
        clusters_tree = (TTree*)f->Get("clusters_tree_V");
    }
    if (!clusters_tree) {
        clusters_tree = (TTree*)f->Get("clusters_tree");
    }
    if (!clusters_tree) {
        LogError << "No clusters tree found in file: " << root_filename << std::endl;
        f->Close();
        return clusters;
    }
    
    // std::vector<TriggerPrimitive> matrix;
    // std::vector<TriggerPrimitive>* matrix_ptr = &matrix;

    int nrows;
    UInt_t event;
    float true_dir_x;
    float true_dir_y;
    float true_dir_z;
    float true_mom_x;
    float true_mom_y;
    float true_mom_z;
    float true_energy;
    std::string true_label;
    std::string true_interaction;
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
    
    // Use fallback for branches that might not exist in older files
    SetBranchWithFallback(clusters_tree, {"true_mom_x"}, &true_mom_x, "true_mom_x");
    SetBranchWithFallback(clusters_tree, {"true_mom_y"}, &true_mom_y, "true_mom_y");
    SetBranchWithFallback(clusters_tree, {"true_mom_z"}, &true_mom_z, "true_mom_z");
    
    clusters_tree->SetBranchAddress("true_energy", &true_energy);
    clusters_tree->SetBranchAddress("true_label", &true_label);
    clusters_tree->SetBranchAddress("min_distance_from_true_pos", &min_distance_from_true_pos);
    clusters_tree->SetBranchAddress("supernova_tp_fraction", &supernova_tp_fraction);
    clusters_tree->SetBranchAddress("generator_tp_fraction", &generator_tp_fraction);
    clusters_tree->SetBranchAddress("true_interaction", &true_interaction);
    for (int i = 0; i < clusters_tree->GetEntries(); i++) {
        clusters_tree->GetEntry(i);
        // Cluster g(matrix);
        // Cluster.set_true_dir({true_dir_x, true_dir_y, true_dir_z});
        // Cluster.set_true_energy(true_energy);
        // Cluster.set_true_label(true_label);
        // Cluster.set_reco_pos({reco_pos_x, reco_pos_y, reco_pos_z});
        // Cluster.set_min_distance_from_true_pos(min_distance_from_true_pos);
        // Cluster.set_supernova_tp_fraction(supernova_tp_fraction);       
        // Cluster.set_true_interaction(true_interaction);
        // clusters.push_back(g);
    }
    f->Close();
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

