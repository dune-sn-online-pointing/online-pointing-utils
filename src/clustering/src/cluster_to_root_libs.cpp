#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <climits>
#include <unordered_map>
#include <set>
#include <initializer_list>
#include <array>
#include <TDirectory.h>
#include <TKey.h>
#include <TBranch.h>
#include <TLeaf.h>
#include <filesystem>

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

namespace {

template <typename T>
bool SetBranchWithFallback(TTree* tree,
                           std::initializer_list<const char*> candidateNames,
                           T* address,
                           const std::string& context) {
    for (const auto* name : candidateNames) {
        if (tree->GetBranch(name) != nullptr) {
            tree->SetBranchAddress(name, address);
            return true;
        }
    }

    std::ostringstream oss;
    oss << "Branches [";
    bool first = true;
    for (const auto* name : candidateNames) {
        if (!first) {
            oss << ", ";
        }
        oss << name;
        first = false;
    }
    oss << "] not found";

    LogError << oss.str() << " in " << context << std::endl;
    return false;
}

} // namespace
// Global map to store time offset corrections per event (TPC ticks)
static std::map<int, double> g_event_time_offsets;

void write_tps_to_root(
    const std::string& out_filename,
    const std::vector<std::vector<TriggerPrimitive>>& tps_by_event,
    const std::vector<std::vector<TrueParticle>>& true_particles_by_event,
    const std::vector<std::vector<Neutrino>>& neutrinos_by_event)
{
    // Ensure output directory exists
    std::string folder = out_filename.substr(0, out_filename.find_last_of("/"));
    if (!folder.empty()) {
        // Use std::filesystem to create directories
        try {
            std::filesystem::create_directories(folder);
        } catch (...) {
            // Fallback to system call if filesystem fails
            std::string command = std::string("mkdir -p ") + folder;
            system(command.c_str());
        }
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
    LogInfo << "Wrote TPs file: " << (_ec_abs ? out_filename : abs_p.string()) << std::endl;
}

void read_tps_from_root(
    const std::string& in_filename,
    std::map<int, std::vector<TriggerPrimitive>>& tps_by_event,
    std::map<int, std::vector<TrueParticle>>& true_particles_by_event,
    std::map<int, std::vector<Neutrino>>& neutrinos_by_event)
{
    LogInfo << "Reading TPs from: " << in_filename << std::endl;
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
void file_reader(std::string filename,
                 std::vector<TriggerPrimitive>& tps,
                 std::vector<TrueParticle>& true_particles,
                 std::vector<Neutrino>& neutrinos,
                 int supernova_option,
                 int event_number,
                 double time_tolerance_ticks,
                 int channel_tolerance) {
    
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
    if (TPtree->SetBranchAddress("Event", &this_event_number) < 0) {
        LogWarning << "Failed to bind branch 'Event'" << std::endl;
    }


    get_first_and_last_event(TPtree, (int*)&this_event_number, event_number, first_tp_entry_in_event, last_tp_entry_in_event);

    // LogInfo << "First entry having this event number: " << first_tp_entry_in_event << std::endl;
    // LogInfo << "Last entry having this event number: " << last_tp_entry_in_event << std::endl;
    LogInfo << "Number of TPs in event " << event_number << ": " << last_tp_entry_in_event - first_tp_entry_in_event + 1 << std::endl;

    tps.reserve(last_tp_entry_in_event - first_tp_entry_in_event + 1);

    UShort_t this_version = 0;
    uint64_t this_time_start = 0;
    UInt_t this_channel = 0;          // Raw channel read from file (may be detector-local)
    UInt_t this_adc_integral = 0;
    UShort_t this_adc_peak = 0;
    UShort_t this_detid = 0;          // Detector (APA) id if present
    // int this_event_number = 0;
    auto bindBranch = [&](const char* name, void* address) {
        if (TPtree->SetBranchAddress(name, address) < 0) {
            LogWarning << "Failed to bind branch '" << name << "'" << std::endl;
        }
    };

    bindBranch("version", &this_version);
    bindBranch("time_start", &this_time_start);
    bindBranch("channel", &this_channel);
    bindBranch("adc_integral", &this_adc_integral);
    bindBranch("adc_peak", &this_adc_peak);
    bindBranch("detid", &this_detid);
    bindBranch("Event", &this_event_number);
    
    // For version 1
    uint64_t this_time_over_threshold = 0;
    uint64_t this_time_peak = 0;
    // Some files use a misspelled v1 name: "time_over_threhsold". Prefer the correct one, fallback to the misspelled.
    {
        const char* totBranchName = nullptr;
        if (TPtree->GetBranch("time_over_threshold") != nullptr) {
            totBranchName = "time_over_threshold";
        } else if (TPtree->GetBranch("time_over_threhsold") != nullptr) {
            totBranchName = "time_over_threhsold";
        }
        if (totBranchName != nullptr) {
            TPtree->SetBranchAddress(totBranchName, &this_time_over_threshold);
        } else {
            LogWarning << "Failed to bind v1 ToT (time_over_threshold/time_over_threhsold)" << std::endl;
        }
    }
    // Some rare files might misspell time_peak as time_peek; try a fallback as well.
    {
        const char* tpeakBranchName = nullptr;
        if (TPtree->GetBranch("time_peak") != nullptr) {
            tpeakBranchName = "time_peak";
        } else if (TPtree->GetBranch("time_peek") != nullptr) {
            tpeakBranchName = "time_peek";
        }
        if (tpeakBranchName != nullptr) {
            TPtree->SetBranchAddress(tpeakBranchName, &this_time_peak);
        } else {
            LogWarning << "Failed to bind v1 time_peak (time_peak/time_peek)" << std::endl;
        }
    }

    // For version 2 (branch types may vary across files; commonly UShort_t or ULong_t/ULong64_t)
    // Bind both correct and commonly misspelled branch names when present, then choose per-entry the non-zero one.
    UShort_t this_samples_over_threshold_correct = 0;
    UShort_t this_samples_over_threshold_typo   = 0;
    UShort_t this_samples_to_peak_correct       = 0;
    UShort_t this_samples_to_peak_typo          = 0;
    bool bound_sover_correct = false, bound_sover_typo = false;
    bool bound_stopeak_correct = false, bound_stopeak_typo = false;
    // Keep TLeaf pointers for type/diagnostic inspection and fallback reads
    TLeaf* leaf_sover_correct = nullptr;
    TLeaf* leaf_sover_typo = nullptr;
    TLeaf* leaf_stopeak_correct = nullptr;
    TLeaf* leaf_stopeak_typo = nullptr;
    if (TPtree->GetBranch("samples_over_threshold") != nullptr) {
        TPtree->SetBranchAddress("samples_over_threshold", &this_samples_over_threshold_correct);
        bound_sover_correct = true;
        leaf_sover_correct = TPtree->GetLeaf("samples_over_threshold");
    }
    if (TPtree->GetBranch("samples_over_thershold") != nullptr) {
        TPtree->SetBranchAddress("samples_over_thershold", &this_samples_over_threshold_typo);
        bound_sover_typo = true;
        leaf_sover_typo = TPtree->GetLeaf("samples_over_thershold");
    }
    if (!bound_sover_correct && !bound_sover_typo) {
        LogWarning << "Failed to bind v2 ToT (samples_over_threshold/samples_over_thershold)" << std::endl;
    }
    if (TPtree->GetBranch("samples_to_peak") != nullptr) {
        TPtree->SetBranchAddress("samples_to_peak", &this_samples_to_peak_correct);
        bound_stopeak_correct = true;
        leaf_stopeak_correct = TPtree->GetLeaf("samples_to_peak");
    }
    if (TPtree->GetBranch("samples_to_peek") != nullptr) {
        TPtree->SetBranchAddress("samples_to_peek", &this_samples_to_peak_typo);
        bound_stopeak_typo = true;
        leaf_stopeak_typo = TPtree->GetLeaf("samples_to_peek");
    }
    if (!bound_stopeak_correct && !bound_stopeak_typo) {
        LogWarning << "Failed to bind v2 samples_to_peak (samples_to_peak/samples_to_peek)" << std::endl;
    }
    // Debug: Log detected leaf types for diagnostics (commented out to reduce verbosity)
    // if (bound_sover_correct && leaf_sover_correct) {
    //     LogInfo << "[TP-READ-DIAG] Leaf type samples_over_threshold: " << leaf_sover_correct->GetTypeName() << std::endl;
    // }
    // if (bound_sover_typo && leaf_sover_typo) {
    //     LogInfo << "[TP-READ-DIAG] Leaf type samples_over_thershold: " << leaf_sover_typo->GetTypeName() << std::endl;
    // }
    // if (bound_stopeak_correct && leaf_stopeak_correct) {
    //     LogInfo << "[TP-READ-DIAG] Leaf type samples_to_peak: " << leaf_stopeak_correct->GetTypeName() << std::endl;
    // }
    // if (bound_stopeak_typo && leaf_stopeak_typo) {
    //     LogInfo << "[TP-READ-DIAG] Leaf type samples_to_peek: " << leaf_stopeak_typo->GetTypeName() << std::endl;
    // }
    
    LogInfo << "  If the TOT or peak branches are not found, it is because the TPs are not that version" << std::endl;

    LogInfo << " Reading tree of TriggerPrimitives" << std::endl;

    // Counters and guards for ToT-based filtering
    size_t tot_filtered_count = 0;
    size_t tot_nonzero_seen = 0; // if no TP has ToT>0, we skip applying the ToT<2 filter

    // Loop over the entries in the tree
    for (Long64_t iTP = first_tp_entry_in_event; iTP <= last_tp_entry_in_event; ++iTP) {
        TPtree->GetEntry(iTP);

    // Build effective v2-like values for ToT and time-to-peak
        UShort_t eff_samples_over_threshold = 0;
        UShort_t eff_samples_to_peak = 0;
        if (this_version == 1){
            // v1 -> v2 conversion
            int clock_to_TPC_ticks = 32; // TODO move to namespace
            eff_samples_over_threshold = static_cast<UShort_t>(this_time_over_threshold / clock_to_TPC_ticks);
            eff_samples_to_peak       = static_cast<UShort_t>((this_time_peak > this_time_start)
                                        ? ((this_time_peak - this_time_start) / clock_to_TPC_ticks)
                                        : 0);
        } else {
            // v2: pick the non-zero branch among correct and typo when both are present
            UShort_t sover_cand1 = bound_sover_correct ? this_samples_over_threshold_correct : 0;
            UShort_t sover_cand2 = bound_sover_typo    ? this_samples_over_threshold_typo   : 0;
            eff_samples_over_threshold = (sover_cand1 > 0 ? sover_cand1 : sover_cand2);

            UShort_t stopeak_cand1 = bound_stopeak_correct ? this_samples_to_peak_correct : 0;
            UShort_t stopeak_cand2 = bound_stopeak_typo    ? this_samples_to_peak_typo    : 0;
            eff_samples_to_peak = (stopeak_cand1 > 0 ? stopeak_cand1 : stopeak_cand2);
        }

        // Common fallback: if effective values remain zero and v2 leaves exist, try reading via TLeaf
        // This also covers files reporting version==1 but actually storing v2-style fields.
        if (eff_samples_over_threshold == 0) {
            double v_sover_c = (leaf_sover_correct ? leaf_sover_correct->GetValue() : 0.0);
            double v_sover_t = (leaf_sover_typo ? leaf_sover_typo->GetValue() : 0.0);
            unsigned long long vi = (unsigned long long)((v_sover_c > 0.0) ? v_sover_c : v_sover_t);
            if (vi > 0ULL) {
                eff_samples_over_threshold = static_cast<UShort_t>(std::min<unsigned long long>(vi, 0xFFFFULL));
            }
        }
        if (eff_samples_to_peak == 0) {
            double v_stopeak_c = (leaf_stopeak_correct ? leaf_stopeak_correct->GetValue() : 0.0);
            double v_stopeak_t = (leaf_stopeak_typo ? leaf_stopeak_typo->GetValue() : 0.0);
            unsigned long long vi = (unsigned long long)((v_stopeak_c > 0.0) ? v_stopeak_c : v_stopeak_t);
            if (vi > 0ULL) {
                eff_samples_to_peak = static_cast<UShort_t>(std::min<unsigned long long>(vi, 0xFFFFULL));
            }
        }

        // skipping flag (useless), type, and algorithm (dropped from TP v2)
        
    // Temporary diagnostic to inspect raw values for the first few TPs in this event
    if (iTP - first_tp_entry_in_event < 5) {
        auto* chLeaf = TPtree->GetLeaf("channel");
        std::array<unsigned char, sizeof(UInt_t)> raw_bytes{};
        if (chLeaf != nullptr) {
            auto* raw_ptr = reinterpret_cast<const unsigned char*>(chLeaf->GetValuePointer());
            if (raw_ptr != nullptr) {
                std::copy(raw_ptr, raw_ptr + raw_bytes.size(), raw_bytes.begin());
            }
        }
        // Debug TP details (commented out to reduce verbosity)
        // LogInfo << "[TP-READ-DIAG] version=" << this_version
        //     << " raw channel=" << this_channel
        //     << " detid=" << this_detid
        //     << " time_start=" << this_time_start
        //     << " samples_over_threshold(eff)=" << eff_samples_over_threshold
        //     << " samples_to_peak(eff)=" << eff_samples_to_peak
        //     << " adc_integral=" << this_adc_integral
        //     << " adc_peak=" << this_adc_peak
        //     << " raw_bytes=["
        //     << static_cast<int>(raw_bytes[0]) << ","
        //     << static_cast<int>(raw_bytes[1]) << ","
        //     << static_cast<int>(raw_bytes[2]) << ","
        //     << static_cast<int>(raw_bytes[3])
        //     << "]"
        //     << std::endl;
    }

    // Determine if the channel appears detector-local; if so promote to global numbering
        // Global scheme is: global = detid * APA::total_channels + local
        uint64_t effective_channel = this_channel;
        if (this_detid > 0 && this_channel >= 0 && this_channel < APA::total_channels) {
            effective_channel = (uint64_t)this_detid * APA::total_channels + (uint64_t)this_channel;
        }
        // Heuristic safeguard: if channel already large (>= total_channels) but detid==0 we assume it's already global and leave unchanged

        // Track whether any non-zero ToT is observed; we'll apply the ToT<2 filter only if ToT info is present and non-trivial
        if (eff_samples_over_threshold > 0) {
            ++tot_nonzero_seen;
        }

        TriggerPrimitive this_tp = TriggerPrimitive(
            this_version,
            0, // flag
            this_detid,
            effective_channel,
            eff_samples_over_threshold,
            this_time_start,
            eff_samples_to_peak,
            this_adc_integral,
            this_adc_peak
        );
        // Debug channel promotion (commented out to reduce verbosity)
        // if (effective_channel != (uint64_t)this_channel) {
        //     LogInfo << "Promoted detector-local channel " << this_channel << " with detid " << this_detid
        //             << " to global channel " << effective_channel << std::endl;
        // }
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

            LogInfo << " Found " << tps.size() << " TPs in file " << filename << " after ToT>=2 filter"
                    << " (filtered " << tot_filtered_count << ")" << std::endl;
        } else {
            // No meaningful ToT data; skip the filter to avoid dropping everything
            LogWarning << " ToT field absent or all zeros; skipping ToT<2 filter. Kept "
                       << tps.size() << " TPs from file " << filename << std::endl;
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
    // MCparticlestree->SetBranchAddress("block_id", &block_id);
    if (!SetBranchWithFallback(MCparticlestree, {"track_id", "g4_track_id"}, &track_id, "MC particles track id")) {
        return;
    }
    if (!SetBranchWithFallback(MCparticlestree, {"truth_id", "truth_track_id", "truth_block_id"}, &truth_id, "MC particles truth id")) {
        return;
    }
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
            LogWarning << "TrackID " << trackID << " not found in MC particles." << std::endl;
        }
    } // end of simides, not used anywhere anymore

    // TEMPORARY DIAGNOSTIC (events 4,6,8,10): For each TP print channel, time, min dt to any SimIDE on same channel, membership.
    if (event_number == 4 || event_number == 6 || event_number == 8 || event_number == 10) {
        // Build map: channel -> vector of simide timestamps (TPC ticks)
        std::unordered_map<int, std::vector<double>> simideTimeByChannel;
        int nSimIDEsDiag = 0;
        double simideMinTime = std::numeric_limits<double>::max();
        double simideMaxTime = -1.0;
        std::vector<std::pair<int,double>> firstSimIdeSamples; firstSimIdeSamples.reserve(20);
        // ADDED DIAGNOSTIC: Check raw timestamps and channel IDs before conversion
        std::vector<UShort_t> rawTimestampSamples; rawTimestampSamples.reserve(15);
        std::vector<UShort_t> rawChannelSamples; rawChannelSamples.reserve(15);
        for (Long64_t iSimIde = first_simide_entry_in_event; iSimIde <= last_simide_entry_in_event; ++iSimIde) {
            simidestree->GetEntry(iSimIde);
            if ((int)rawTimestampSamples.size() < 15) {
                rawTimestampSamples.push_back(Timestamp);
                rawChannelSamples.push_back(ChannelID);
            }
            // OVERFLOW FIX: Cast to larger type before multiplication to prevent 16-bit overflow
            double tConv = (double)((uint64_t)Timestamp * conversion_tdc_to_tpc); // scaled units
            simideTimeByChannel[(int)ChannelID].push_back(tConv);
            nSimIDEsDiag++;
            if ((int)firstSimIdeSamples.size() < 15) firstSimIdeSamples.emplace_back((int)ChannelID, tConv);
            simideMinTime = std::min(simideMinTime, tConv);
            simideMaxTime = std::max(simideMaxTime, tConv);
        }
        for (auto &kv : simideTimeByChannel) {
            auto &vec = kv.second; std::sort(vec.begin(), vec.end());
        }
        // Attempt automatic time-offset detection: compare earliest SimIDE time on overlapping channels with TP times.
        std::vector<double> diffs;
        diffs.reserve(32);
        for (const auto &kv : simideTimeByChannel) {
            int ch = kv.first;
            double earliestSim = kv.second.front();
            for (const auto &tp : tps) {
                if (tp.GetEvent() != (int)event_number) continue;
                if (tp.GetChannel() == ch) {
                    double diff = earliestSim - tp.GetTimeStart();
                    if (diff > 0) diffs.push_back(diff);
                    break; // only one TP needed per channel
                }
            }
        }
        double appliedOffset = 0.0;
        if (diffs.size() >= 3) {
            std::sort(diffs.begin(), diffs.end());
            double median = diffs[diffs.size()/2];
            // Heuristic bounds: expect large systematic offset in range [1000, 200000] ticks
            if (median > 1000 && median < 200000) {
                appliedOffset = median;
                // Shift true particle windows back by this offset
                for (auto &p : true_particles) {
                    double ns = p.GetTimeStart() - appliedOffset; if (ns < 0) ns = 0; p.SetTimeStart(ns);
                    double ne = p.GetTimeEnd() - appliedOffset; if (ne < ns) ne = ns; p.SetTimeEnd(ne);
                }
            }
        }
        // Store the offset in global map for use in write_tps_to_root
        g_event_time_offsets[event_number] = appliedOffset;
        
        // Debug output for time offset correction (commented out)
        // if (appliedOffset > 0) {
        //     LogInfo << "[DIAG] Event " << event_number << " Applied time offset correction ticks=" << appliedOffset << " (median diff)." << std::endl;
        // } else {
        //     LogInfo << "[DIAG] Event " << event_number << " No time offset correction applied (diff samples=" << diffs.size() << ")." << std::endl;
        // }
        
        // TP time span for this event
        double tpMinTime = std::numeric_limits<double>::max();
        double tpMaxTime = -1.0;
        int tpCountInEvent = 0;
        for (size_t i = 0; i < tps.size(); ++i) {
            if (tps[i].GetEvent() != (int)event_number) continue;
            tpCountInEvent++;
            tpMinTime = std::min(tpMinTime, tps[i].GetTimeStart());
            tpMaxTime = std::max(tpMaxTime, tps[i].GetTimeStart());
        }
        
        // Debug diagnostics (commented out to reduce output)
        // LogInfo << "[DIAG] Event " << event_number << " SimIDEs summary: nSimIDEs=" << nSimIDEsDiag
        //         << " uniqueChannels=" << simideTimeByChannel.size()
        //         << " timeRangeTicks=[" << simideMinTime << "," << simideMaxTime << "]" << std::endl;
        // LogInfo << "[DIAG] Event " << event_number << " RAW timestamps (before ×32): ";
        // for (size_t i = 0; i < rawTimestampSamples.size(); ++i) {
        //     if (i > 0) LogInfo << ",";
        //     LogInfo << rawTimestampSamples[i];
        // }
        // LogInfo << std::endl;
        // LogInfo << "[DIAG] Event " << event_number << " RAW SimIDE channels (no promotion): ";
        // for (size_t i = 0; i < rawChannelSamples.size(); ++i) {
        //     if (i > 0) LogInfo << ",";
        //     int ch = rawChannelSamples[i];
        //     int detid = ch / 2560;
        //     int local_ch = ch % 2560;
        //     char plane = 'U';
        //     if (local_ch >= 800 && local_ch < 1600) plane = 'V';
        //     else if (local_ch >= 1600) plane = 'X';
        //     LogInfo << ch << "(" << plane << ")";
        // }
        // LogInfo << std::endl;
        
        // Debug TP diagnostics (commented out to reduce output)
        // LogInfo << "[DIAG] Event " << event_number << " TPs summary: nTPs=" << tpCountInEvent
        //         << " timeRangeTicks=[" << tpMinTime << "," << tpMaxTime << "]" << std::endl;
        // if (!firstSimIdeSamples.empty()) {
        //     std::ostringstream oss; oss << "[DIAG] Event " << event_number << " FirstSimIDEs(ch:time)=";
        //     for (size_t i=0;i<firstSimIdeSamples.size();++i){ oss << firstSimIdeSamples[i].first << ":" << firstSimIdeSamples[i].second; if (i+1<firstSimIdeSamples.size()) oss << ","; }
        //     LogInfo << oss.str() << std::endl;
        // }
        
        // TP channel plane analysis (commented out debug)
        // int tpCountU=0, tpCountV=0, tpCountX=0;
        // int tpMinU=999999, tpMaxU=-1, tpMinV=999999, tpMaxV=-1, tpMinX=999999, tpMaxX=-1;
        // for (size_t i = 0; i < tps.size(); ++i) {
        //     if (tps[i].GetEvent() != (int)event_number) continue;
        //     int ch = tps[i].GetChannel();
        //     int detid = ch / 2560;
        //     int local_ch = ch % 2560;
        //     if (local_ch < 800) { tpCountU++; tpMinU=std::min(tpMinU,ch); tpMaxU=std::max(tpMaxU,ch); }
        //     else if (local_ch < 1600) { tpCountV++; tpMinV=std::min(tpMinV,ch); tpMaxV=std::max(tpMaxV,ch); }
        //     else { tpCountX++; tpMinX=std::min(tpMinX,ch); tpMaxX=std::max(tpMaxX,ch); }
        // }
        // LogInfo << "[DIAG] Event " << event_number << " TP planes: U=" << tpCountU;
        // if(tpCountU>0) LogInfo << "[" << tpMinU << "-" << tpMaxU << "]";
        // LogInfo << " V=" << tpCountV;
        // if(tpCountV>0) LogInfo << "[" << tpMinV << "-" << tpMaxV << "]";
        // LogInfo << " X=" << tpCountX;
        // if(tpCountX>0) LogInfo << "[" << tpMinX << "-" << tpMaxX << "]";
        // LogInfo << std::endl;
        
        // Track MARLEY associations by plane (commented out debug)
        // int marleyCountU=0, marleyCountV=0, marleyCountX=0;
        // LogInfo << "[DIAG] Event " << event_number << " TP-to-SimIDE channel/time proximity:" << std::endl;
        // LogInfo << "[DIAG] Format: TPIndex channel time_start ticks | inSimIDEset | minDeltaTicks" << std::endl;
        // for (size_t i = 0; i < tps.size(); ++i) {
        //     if (tps[i].GetEvent() != (int)event_number) continue; // only current event's TPs
        //     int globalCh = tps[i].GetChannel();
        //     int localCh = tps[i].GetDetectorChannel();
        //     double t = tps[i].GetTimeStart();
        //     bool inSet = simideTimeByChannel.find(globalCh) != simideTimeByChannel.end();
        //     double minDt = -1.0;
        //     if (inSet) {
        //         const auto &vt = simideTimeByChannel[globalCh];
        //         // binary search for closest
        //         auto it = std::lower_bound(vt.begin(), vt.end(), t);
        //         if (it != vt.end()) minDt = std::abs(*it - t);
        //         if (it != vt.begin()) minDt = (minDt < 0) ? std::abs(*(it-1)-t) : std::min(minDt, std::abs(*(it-1)-t));
        //     }
        //     LogInfo << "[DIAG] TP " << i << " gch=" << globalCh << " (local=" << localCh << ") t=" << t
        //             << " | inSet=" << (inSet?"Y":"N")
        //             << " | minDt=" << minDt << std::endl;
        //     // Count MARLEY associations by plane
        //     if (inSet) {
        //         int detid = globalCh / 2560;
        //         int local_channel = globalCh % 2560;
        //         if (local_channel < 800) marleyCountU++;
        //         else if (local_channel < 1600) marleyCountV++;
        //         else marleyCountX++;
        //     }
        // }
        // // MARLEY association summary by plane
        // LogInfo << "[DIAG] Event " << event_number << " MARLEY associations by plane: U=" << marleyCountU 
        //         << " V=" << marleyCountV << " X=" << marleyCountX 
        //         << " (total=" << (marleyCountU + marleyCountV + marleyCountX) << ")" << std::endl;
        // // True particle spatial info (first one if exists)
        // if (!true_particles.empty()) {
        //     const auto &p = true_particles.front();
        //     LogInfo << "[DIAG] Event " << event_number << " TrueParticle pos (x,y,z)=(" << p.GetX() << "," << p.GetY() << "," << p.GetZ() << ")"
        //             << " timeWindowTicks=[" << p.GetTimeStart() << "," << p.GetTimeEnd() << "]"
        //             << " nChannels=" << p.GetChannels().size() << std::endl;
        // }
    }

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

    // NEW: Apply direct TP-SimIDE matching to replace/supplement trueParticle-based matching
    LogInfo << " Applying direct TP-SimIDE matching for event " << event_number << std::endl;
    const double effective_time_tolerance = (time_tolerance_ticks >= 0.0) ? time_tolerance_ticks : 5000.0;
    const int effective_channel_tolerance = (channel_tolerance >= 0) ? channel_tolerance : 50;
    match_tps_to_simides_direct(tps, true_particles, file, event_number, effective_time_tolerance, effective_channel_tolerance);

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
    // Ensure a fixed TDirectory inside the ROOT file for cluster trees
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
        // Compute fraction of TPs in this cluster with a non-UNKNOWN generator
        {
            int cluster_truth_count = 0;
            const auto& cl_tps = cluster.get_tps();
            for (auto* tp : cl_tps) {
                auto* tpTruth = tp->GetTrueParticle();
                if (tpTruth != nullptr && tpTruth->GetGeneratorName() != "UNKNOWN") {
                    cluster_truth_count++;
                }
            }
            generator_tp_fraction = cl_tps.empty() ? 0.f : static_cast<float>(cluster_truth_count) / static_cast<float>(cl_tps.size());
            // std::cout << "Fraction of TPs with non-null generator: " << generator_tp_fraction << std::endl;
        }
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
    // Write inside 'clusters' directory
    clusters_dir->cd();
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

// Direct TP-SimIDE matching function based on time and channel proximity
void match_tps_to_simides_direct(
    std::vector<TriggerPrimitive>& tps,
    std::vector<TrueParticle>& true_particles,
    TFile* file,
    int event_number,
    double time_tolerance_ticks,
    int channel_tolerance)
{
    LogInfo << "Starting direct TP-SimIDE matching for event " << event_number << std::endl;
    
    // Fetch any previously estimated time-offset correction for this event
    double event_time_offset = 0.0;
    {
        auto it = g_event_time_offsets.find(event_number);
        if (it != g_event_time_offsets.end()) event_time_offset = it->second;
    }
    if (event_time_offset != 0.0) {
        LogInfo << "[DIRECT] Using event time-offset correction (ticks) = " << event_time_offset << std::endl;
    }

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
    get_first_and_last_event(simidestree, (int*)&event_number_simides, event_number, first_simide_entry, last_simide_entry);
    
    if (first_simide_entry == -1) {
        LogWarning << "No SimIDEs found for event " << event_number << std::endl;
        return;
    }
    
    // Set up SimIDE branch addresses
    UInt_t simide_channel;
    UShort_t simide_timestamp;
    Int_t simide_track_id;

    if (!SetBranchWithFallback(simidestree, {"ChannelID", "channel"}, &simide_channel, "SimIDEs channel")) {
        return;
    }
    if (!SetBranchWithFallback(simidestree, {"Timestamp", "timestamp"}, &simide_timestamp, "SimIDEs timestamp")) {
        return;
    }
    if (!SetBranchWithFallback(simidestree, {"trackID", "origTrackID"}, &simide_track_id, "SimIDEs track ID")) {
        return;
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
    };
    std::vector<SimIDEInfo> simides_in_event;
    
    // Read all SimIDEs for this event
    for (Long64_t iSimIde = first_simide_entry; iSimIde <= last_simide_entry; ++iSimIde) {
        simidestree->GetEntry(iSimIde);
        
        // Convert SimIDE timestamp to TPC ticks (with overflow protection)
        uint64_t timestampU64 = (uint64_t)simide_timestamp;
        double time_tpc = (double)(timestampU64 * conversion_tdc_to_tpc);
        // Apply event time-offset so SimIDE times are comparable to TP times
        double time_tpc_aligned = time_tpc - event_time_offset;
        
        // Find associated particle
        auto particle_it = track_to_particle.find(std::abs(simide_track_id));
        if (particle_it != track_to_particle.end()) {
            simides_in_event.push_back({
                (int)simide_channel,
                time_tpc_aligned,
                simide_track_id,
                particle_it->second
            });
        }
    }
    
    LogInfo << "Found " << simides_in_event.size() << " SimIDEs linked to particles in event " << event_number << std::endl;
    
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
    LogInfo << "Direct TP-SimIDE matching results for event " << event_number << ": "
            << matched_tp_count << "/" << total_tp_count << " TPs matched ("
            << std::fixed << std::setprecision(1) << match_efficiency << "%)" << std::endl;
    if (skipped_tp_outside_windows > 0) {
        LogInfo << "[DIRECT] Skipped " << skipped_tp_outside_windows << " TPs outside APA/time windows." << std::endl;
    }
    if (matched_tp_count > 0) {
        LogInfo << "[DIRECT] Plane-consistent matches: " << matched_same_plane << "/" << matched_tp_count << std::endl;
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

