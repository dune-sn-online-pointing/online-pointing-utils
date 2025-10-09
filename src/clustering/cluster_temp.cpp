#include "Clustering.h"
#include "Cluster.h"
#include "TriggerPrimitive.hpp"
#include "global.h"

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

namespace {

// bool ensureDirectoryExists(const std::string& folder) {
//     if (folder.empty()) {
//         return true;
//     }

//     std::error_code ec;
//     std::filesystem::create_directories(folder, ec);
//     if (ec) {
//         LogError << "Failed to ensure directory '" << folder << "' exists: " << ec.message() << std::endl;
//         return false;
//     }
//     return true;
// }


} // namespace
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

