#include "cluster_to_root_libs.h"
#include "Cluster.h"
#include "TriggerPrimitive.hpp"
#include "global.h"

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});


bool ensureDirectoryExists(const std::string& folder) {
    if (folder.empty()) {
        return true;
    }

    std::error_code ec;
    std::filesystem::create_directories(folder, ec);
    if (ec) {
        LogError << "Failed to ensure directory '" << folder << "' exists: " << ec.message() << std::endl;
        return false;
    }
    return true;
}

// template <typename T>
// bool SetBranchWithFallback(TTree* tree,
//                            std::initializer_list<const char*> candidateNames,
//                            T* address,
//                            const std::string& context) {
//     for (const auto* name : candidateNames) {
//         if (tree->GetBranch(name) != nullptr) {
//             tree->SetBranchAddress(name, address);
//             return true;
//         }
//     }

//     std::ostringstream oss;
//     oss << "Branches [";
//     bool first = true;
//     for (const auto* name : candidateNames) {
//         if (!first) {
//             oss << ", ";
//         }
//         oss << name;
//         first = false;
//     }
//     oss << "] not found";

//     LogError << oss.str() << " in " << context << std::endl;
//     return false;
// }

// } // namespace
// Global map to store time offset corrections per event (TPC ticks)
static std::map<int, double> g_event_time_offsets;

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

    // TPs tree
    TTree tpsTree("tps", "Trigger Primitives with truth links");
    int evt = 0; UShort_t version=0; UInt_t detid=0; UInt_t channel=0; UInt_t adc_integral=0; UShort_t adc_peak=0; UShort_t det=0; Int_t det_channel=0; ULong64_t tstart=0; ULong64_t s_over=0; ULong64_t s_to_peak=0;
    std::string view; std::string gen_name; std::string process;
    Int_t tp_truth_id=-1; Int_t tp_track_id=-1; Int_t tp_pdg=0; Int_t nu_truth_id=-1;
    Float_t nu_energy = -1.0f;
    Float_t time_offset_correction = 0.0f; // TPC ticks correction applied to truth time windows
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
    tpsTree.Branch("truth_id", &tp_truth_id, "truth_id/I");
    tpsTree.Branch("track_id", &tp_track_id, "track_id/I");
    tpsTree.Branch("pdg", &tp_pdg, "pdg/I");
    tpsTree.Branch("generator_name", &gen_name);
    tpsTree.Branch("process", &process);
    tpsTree.Branch("neutrino_truth_id", &nu_truth_id, "neutrino_truth_id/I");
    tpsTree.Branch("neutrino_energy", &nu_energy, "neutrino_energy/F");
    tpsTree.Branch("time_offset_correction", &time_offset_correction, "time_offset_correction/F");

    // True particles tree
    TTree trueTree("true_particles", "True particles per event");
    int tevt=0; float x=0,y=0,z=0,Px=0,Py=0,Pz=0, en=0; std::string tgen; int pdg=0; std::string tproc; int track_id=0; int truth_id=0; double tstart_d=0, tend_d=0; std::vector<int>* channels = nullptr; int nu_link_truth_id=-1;
    trueTree.Branch("event", &tevt, "event/I");
    trueTree.Branch("x", &x, "x/F"); trueTree.Branch("y", &y, "y/F"); trueTree.Branch("z", &z, "z/F");
    trueTree.Branch("Px", &Px, "Px/F"); trueTree.Branch("Py", &Py, "Py/F"); trueTree.Branch("Pz", &Pz, "Pz/F");
    trueTree.Branch("en", &en, "en/F");
    trueTree.Branch("generator_name", &tgen);
    trueTree.Branch("pdg", &pdg, "pdg/I");
    trueTree.Branch("process", &tproc);
    trueTree.Branch("track_id", &track_id, "track_id/I");
    trueTree.Branch("truth_id", &truth_id, "truth_id/I");
    trueTree.Branch("time_start", &tstart_d, "time_start/D");
    trueTree.Branch("time_end", &tend_d, "time_end/D");
    trueTree.Branch("channels", &channels);
    trueTree.Branch("neutrino_truth_id", &nu_link_truth_id, "neutrino_truth_id/I");

    // Neutrinos tree
    TTree nuTree("neutrinos", "Neutrinos per event");
    int nevt=0; std::string interaction; float nx=0,ny=0,nz=0,nPx=0,nPy=0,nPz=0; int nen=0; int ntruth_id=0;
    nuTree.Branch("event", &nevt, "event/I");
    nuTree.Branch("interaction", &interaction);
    nuTree.Branch("x", &nx, "x/F"); nuTree.Branch("y", &ny, "y/F"); nuTree.Branch("z", &nz, "z/F");
    nuTree.Branch("Px", &nPx, "Px/F"); nuTree.Branch("Py", &nPy, "Py/F"); nuTree.Branch("Pz", &nPz, "Pz/F");
    nuTree.Branch("en", &nen, "en/I");
    nuTree.Branch("truth_id", &ntruth_id, "truth_id/I");

    // Fill truth first for easy linking
    for (size_t ev = 0; ev < true_particles_by_event.size(); ++ev) {
        const auto& v = true_particles_by_event[ev];
        for (const auto& p : v) {
            tevt = p.GetEvent(); x=p.GetX(); y=p.GetY(); z=p.GetZ(); Px=p.GetPx(); Py=p.GetPy(); Pz=p.GetPz(); en=p.GetEnergy(); tgen=p.GetGeneratorName(); pdg=p.GetPdg(); tproc=p.GetProcess(); track_id=p.GetTrackId(); truth_id=p.GetTruthId(); tstart_d=p.GetTimeStart(); tend_d=p.GetTimeEnd();
            std::vector<int> chs(p.GetChannels().begin(), p.GetChannels().end()); channels = &chs; // local; will be copied by ROOT
            nu_link_truth_id = (p.GetNeutrino() ? p.GetNeutrino()->GetTruthId() : -1);
            trueTree.Fill();
        }
    }

    for (size_t ev = 0; ev < neutrinos_by_event.size(); ++ev) {
        const auto& v = neutrinos_by_event[ev];
        for (const auto& n : v) {
            nevt = n.GetEvent(); interaction = n.GetInteraction(); nx=n.GetX(); ny=n.GetY(); nz=n.GetZ(); nPx=n.GetPx(); nPy=n.GetPy(); nPz=n.GetPz(); nen=n.GetEnergy(); ntruth_id=n.GetTruthId();
            nuTree.Fill();
        }
    }

    // Now TPs with truth links
    for (size_t ev = 0; ev < tps_by_event.size(); ++ev) {
        const auto& v = tps_by_event[ev];
        for (const auto& tp : v) {
            evt = tp.GetEvent(); version = TriggerPrimitive::s_trigger_primitive_version; detid = 0; channel = tp.GetChannel(); s_over = tp.GetSamplesOverThreshold(); tstart = tp.GetTimeStart(); s_to_peak = tp.GetSamplesToPeak(); adc_integral = tp.GetAdcIntegral(); adc_peak = tp.GetAdcPeak(); det = tp.GetDetector(); det_channel = tp.GetDetectorChannel(); view = tp.GetView();
            
            // Set time offset correction for this event
            auto offset_it = g_event_time_offsets.find(evt);
            time_offset_correction = (offset_it != g_event_time_offsets.end()) ? (Float_t)offset_it->second : 0.0f;
            
            const auto* tpp = tp.GetTrueParticle();
            if (tpp != nullptr) {
                tp_truth_id = tpp->GetTruthId(); tp_track_id = tpp->GetTrackId(); tp_pdg = tpp->GetPdg(); gen_name = tpp->GetGeneratorName(); process = tpp->GetProcess();
                nu_truth_id = (tpp->GetNeutrino() ? tpp->GetNeutrino()->GetTruthId() : -1);
                nu_energy = (tpp->GetNeutrino() ? (Float_t)tpp->GetNeutrino()->GetEnergy() : -1.0f);
            }
            else { tp_truth_id = -1; tp_track_id = -1; tp_pdg = 0; gen_name = "UNKNOWN"; process = ""; nu_truth_id = -1; nu_energy = -1.0f; }
            tpsTree.Fill();
        }
    }

    tpsDir->cd();
    tpsTree.Write(); trueTree.Write(); nuTree.Write();
    outFile.Close();
    
    // Clear the global offset map after writing
    g_event_time_offsets.clear();
    
    // Report absolute output path for consistency
    std::error_code _ec_abs;
    auto abs_p = std::filesystem::absolute(std::filesystem::path(out_filename), _ec_abs);
    if (verboseMode) LogInfo << "Wrote TPs file: " << (_ec_abs ? out_filename : abs_p.string()) << std::endl;
}

void read_tps(
    const std::string& in_filename,
    std::map<int, std::vector<TriggerPrimitive>>& tps_by_event,
    std::map<int, std::vector<TrueParticle>>& true_particles_by_event,
    std::map<int, std::vector<Neutrino>>& neutrinos_by_event)
{
    if (verboseMode) LogInfo << "Reading TPs from: " << in_filename << std::endl;
    TFile inFile(in_filename.c_str(), "READ"); if (inFile.IsZombie()) { LogError << "Cannot open: " << in_filename << std::endl; return; }
    TDirectory* tpsDir = inFile.GetDirectory("tps"); if (!tpsDir) { LogError << "Directory 'tps' not found in: " << in_filename << std::endl; return; }

    // Read neutrinos
    if (auto* nuTree = dynamic_cast<TTree*>(tpsDir->Get("neutrinos"))) {
        int event=0; std::string* interaction=nullptr; float x=0,y=0,z=0,Px=0,Py=0,Pz=0; int en=0; int truth_id=0; nuTree->SetBranchAddress("event", &event); nuTree->SetBranchAddress("interaction", &interaction); nuTree->SetBranchAddress("x", &x); nuTree->SetBranchAddress("y", &y); nuTree->SetBranchAddress("z", &z); nuTree->SetBranchAddress("Px", &Px); nuTree->SetBranchAddress("Py", &Py); nuTree->SetBranchAddress("Pz", &Pz); nuTree->SetBranchAddress("en", &en); nuTree->SetBranchAddress("truth_id", &truth_id);
        for (Long64_t i=0;i<nuTree->GetEntries();++i){ nuTree->GetEntry(i); Neutrino n(event, *interaction, x,y,z,Px,Py,Pz,en,truth_id); neutrinos_by_event[event].push_back(n);}    }

    // Read true particles
    std::map<std::pair<int,int>, Neutrino*> nuLookup; // (event, truth_id) -> neutrino ptr
    for (auto& kv : neutrinos_by_event) { for (auto& n : kv.second) { nuLookup[{kv.first, n.GetTruthId()}] = &n; } }

    if (auto* tTree = dynamic_cast<TTree*>(tpsDir->Get("true_particles"))) {
        int event=0; float x=0,y=0,z=0,Px=0,Py=0,Pz=0,en=0; std::string* gen=nullptr; int pdg=0; std::string* proc=nullptr; int track_id=0; int truth_id=0; double tstart=0,tend=0; std::vector<int>* channels=nullptr; int nu_truth_id=-1;
        tTree->SetBranchAddress("event", &event); tTree->SetBranchAddress("x", &x); tTree->SetBranchAddress("y", &y); tTree->SetBranchAddress("z", &z); tTree->SetBranchAddress("Px", &Px); tTree->SetBranchAddress("Py", &Py); tTree->SetBranchAddress("Pz", &Pz); tTree->SetBranchAddress("en", &en); tTree->SetBranchAddress("generator_name", &gen); tTree->SetBranchAddress("pdg", &pdg); tTree->SetBranchAddress("process", &proc); tTree->SetBranchAddress("track_id", &track_id); tTree->SetBranchAddress("truth_id", &truth_id); tTree->SetBranchAddress("time_start", &tstart); tTree->SetBranchAddress("time_end", &tend); tTree->SetBranchAddress("channels", &channels); tTree->SetBranchAddress("neutrino_truth_id", &nu_truth_id);
        for (Long64_t i=0;i<tTree->GetEntries();++i){ tTree->GetEntry(i); TrueParticle tp(event,x,y,z,Px,Py,Pz,en,*gen,pdg,*proc,track_id,truth_id); tp.SetTimeStart(tstart); tp.SetTimeEnd(tend); if (channels){ for (int ch : *channels) tp.AddChannel(ch); } auto it = nuLookup.find({event, nu_truth_id}); if (it != nuLookup.end()) tp.SetNeutrino(it->second); true_particles_by_event[event].push_back(tp);}    }

    // Build lookup for true particles
    std::map<std::pair<int,int>, TrueParticle*> truthLookup; // (event, truth_id)
    for (auto& kv : true_particles_by_event) { for (auto& tp : kv.second) { truthLookup[{kv.first, tp.GetTruthId()}] = &tp; } }

    // Read TPs
    if (auto* tpTree = dynamic_cast<TTree*>(tpsDir->Get("tps"))) {
        int event=0; UShort_t version=0; UInt_t detid=0; UInt_t channel=0; ULong64_t s_over=0; ULong64_t tstart=0; ULong64_t s_to_peak=0; UInt_t adc_integral=0; UShort_t adc_peak=0; UShort_t det=0; Int_t det_channel=0; std::string* view=nullptr; Int_t truth_id=-1; Int_t track_id=-1; Int_t pdg=0; std::string* gen_name=nullptr; std::string* process=nullptr; Int_t nu_truth_id=-1; Float_t nu_energy=-1.0f;
        tpTree->SetBranchAddress("event", &event); tpTree->SetBranchAddress("version", &version); tpTree->SetBranchAddress("detid", &detid); tpTree->SetBranchAddress("channel", &channel); tpTree->SetBranchAddress("samples_over_threshold", &s_over); tpTree->SetBranchAddress("time_start", &tstart); tpTree->SetBranchAddress("samples_to_peak", &s_to_peak); tpTree->SetBranchAddress("adc_integral", &adc_integral); tpTree->SetBranchAddress("adc_peak", &adc_peak); tpTree->SetBranchAddress("detector", &det); tpTree->SetBranchAddress("detector_channel", &det_channel); tpTree->SetBranchAddress("view", &view); tpTree->SetBranchAddress("truth_id", &truth_id); tpTree->SetBranchAddress("track_id", &track_id); tpTree->SetBranchAddress("pdg", &pdg); tpTree->SetBranchAddress("generator_name", &gen_name); tpTree->SetBranchAddress("process", &process); tpTree->SetBranchAddress("neutrino_truth_id", &nu_truth_id);
        // neutrino_energy is optional for backward compatibility
        if (tpTree->GetBranch("neutrino_energy")) {
            tpTree->SetBranchAddress("neutrino_energy", &nu_energy);
        }
        for (Long64_t i=0;i<tpTree->GetEntries();++i){ tpTree->GetEntry(i); TriggerPrimitive prim(version, 0, detid, channel, s_over, tstart, s_to_peak, adc_integral, adc_peak); prim.SetEvent(event); prim.SetDetector(det); prim.SetDetectorChannel(det_channel); // SetView already in constructor
            auto it = truthLookup.find({event, truth_id}); if (it != truthLookup.end()) prim.SetTrueParticle(it->second); tps_by_event[event].push_back(prim);}    }

    inFile.Close();
}


// TODO change this, one argument should be nentries_event
void get_first_and_last_event(TTree* tree, int* branch_value, int which_event, int& first_entry, int& last_entry) {
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
                        // LogInfo << "Appended TP to candidate Cluster" << std::endl;
                        break;
                    }
                }
                if (appended) break;
                // LogInfo << "Not appended to candidate Cluster" << std::endl;
            }
        }

        // If not appended to any candidate, create a new Cluster in the buffer
        if (!appended) {
            // LogInfo << "Creating new candidate Cluster" << std::endl;
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
                // LogInfo << "Candidate Cluster has " << candidate.size() << " TPs" << std::endl;
                clusters.emplace_back(Cluster(std::move(candidate)));
                // LogInfo << "Cluster created with " << clusters.back().get_tps().size() << " TPs" << std::endl;
            }
            else {
            }
        }
    }

    LogInfo << "Finished clustering. Number of clusters: " << clusters.size() << std::endl;

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
    
    LogInfo << "Number of blips: " << blips.size() << std::endl;
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
    TTree *clusters_tree =  (TTree*)clusters_dir->Get(Form("clusters_tree_%s", view.c_str()));

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
        if (verboseMode) LogInfo << "Tree not found, creating it" << std::endl;
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
    for (auto& Cluster : clusters) {
        event = Cluster.get_event();
        n_tps = Cluster.get_size();
        true_dir_x = Cluster.get_true_dir()[0];
        true_dir_y = Cluster.get_true_dir()[1];
        true_dir_z = Cluster.get_true_dir()[2];
        true_neutrino_energy = Cluster.get_true_neutrino_energy();
        true_particle_energy = Cluster.get_true_particle_energy();
        true_label = Cluster.get_true_label();
        true_label_point = &true_label;
        reco_pos_x = Cluster.get_reco_pos()[0];
        reco_pos_y = Cluster.get_reco_pos()[1];
        reco_pos_z = Cluster.get_reco_pos()[2];
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

        tp_detector_channel->clear();
        tp_detector->clear();
        tp_samples_over_threshold->clear();
        tp_time_start->clear();
        tp_samples_to_peak->clear();
        tp_adc_peak->clear();
        tp_samples_to_peak->clear();
        tp_adc_integral->clear();
        for (auto& tp : Cluster.get_tps()) {
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
    // Write inside 'clusters' directory
    clusters_dir->cd();
    clusters_tree->Write("", TObject::kOverwrite);
    clusters_file->Close();

    return;   
}

std::vector<Cluster> read_clusters_from_root(std::string root_filename){
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

// Direct TP-SimIDE matching function based on time and channel proximity
