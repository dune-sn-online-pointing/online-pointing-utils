#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <nlohmann/json.hpp>

#include "CmdLineParser.h"
#include "Logger.h"
#include "GenericToolbox.Utils.h"

// ROOT includes
#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TF1.h>
#include <TStyle.h>
#include <TROOT.h>
#include <TText.h>
#include <TDatime.h>
#include <TDirectory.h>
#include <chrono>
#include <filesystem>
#include <TLatex.h>

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

int main(int argc, char* argv[]) {  
    CmdLineParser clp;

    clp.getDescription() << "> analyze_tps app - Generate plots from ROOT files with trigger primitive data."<< std::endl;

    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");
    clp.addOption("outFolder", {"--output-folder"}, "Output folder path (optional, defaults to input file folder)");

    clp.addDummyOption("Triggers");
    clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");

    clp.addDummyOption();
    // usage always displayed
    LogInfo << clp.getDescription().str() << std::endl;

    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;

    clp.parseCmdLine(argc, argv);

    LogThrowIf( clp.isNoOptionTriggered(), "No option was provided." );

    LogInfo << "Provided arguments: " << std::endl;
    LogInfo << clp.getValueSummary() << std::endl << std::endl;

    std::string json = clp.getOptionVal<std::string>("json");
    std::filesystem::path json_path(json);
    std::filesystem::path json_dir = json_path.has_parent_path() ? json_path.parent_path() : std::filesystem::current_path();
    std::error_code _ec_abs;
    if (!json_dir.is_absolute()) json_dir = std::filesystem::absolute(json_dir, _ec_abs);
    
    // Read the configuration file
    std::ifstream i(json);
    LogThrowIf(not i.is_open(), "Could not open JSON file: " << json);
    
    nlohmann::json j;
    i >> j;
    
    // Determine input sources: either a single ROOT filename or a text file containing a list of ROOT files
    std::vector<std::string> inputFiles;
    std::string filename;
    bool has_filelist = false;
    auto resolvePath = [&](const std::string& p) -> std::string {
        if (p.empty()) return p;
        std::filesystem::path rel(p);
        if (rel.is_absolute()) return rel.string();
        std::vector<std::filesystem::path> candidates = {
            rel,
            std::filesystem::current_path() / rel,
            std::filesystem::current_path().parent_path() / rel,
            std::filesystem::current_path().parent_path().parent_path() / rel,
            json_dir / rel,
            json_dir.parent_path() / rel,
            json_dir.parent_path().parent_path() / rel
        };
        for (const auto& c : candidates) {
            std::error_code ec;
            if (std::filesystem::exists(c, ec)) {
                std::filesystem::path abs = std::filesystem::absolute(c, ec);
                return ec ? c.string() : abs.string();
            }
        }
        return rel.string();
    };

    if (j.contains("filename") && !j["filename"].is_null() && !j["filename"].get<std::string>().empty()) {
        LogInfo << "Using single input file: " << j["filename"].get<std::string>() << std::endl;
        filename = resolvePath(j["filename"]);
        inputFiles.push_back(filename);
    }
    else{
        if (j.contains("filelist") && !j["filelist"].is_null() && !j["filelist"].get<std::string>().empty()) {
            std::string listPath = resolvePath(j["filelist"]);
            LogInfo << "Using file list: " << j["filelist"].get<std::string>() << " (resolved: " << listPath << ")" << std::endl;
            has_filelist = true;
            std::ifstream listFile(listPath);
            LogThrowIf(!listFile.is_open(), "Could not open file list: " << listPath);
            std::filesystem::path list_dir = std::filesystem::path(listPath).parent_path();
            std::error_code _ec_list;
            if (!list_dir.is_absolute()) list_dir = std::filesystem::absolute(list_dir, _ec_list);
        auto resolvePathWithBase = [&](const std::string& p, const std::filesystem::path& base) -> std::string {
                if (p.empty()) return p;
                std::filesystem::path rel(p);
                if (rel.is_absolute()) return rel.string();
                std::vector<std::filesystem::path> candidates = {
                    rel,
                    std::filesystem::current_path() / rel,
                    base / rel,
                    base.parent_path() / rel,
                    json_dir / rel,
            json_dir.parent_path() / rel,
            std::filesystem::current_path().parent_path() / rel,
            std::filesystem::current_path().parent_path().parent_path() / rel,
            json_dir.parent_path().parent_path() / rel
                };
                for (const auto& c : candidates) {
                    std::error_code ec;
                    if (std::filesystem::exists(c, ec)) {
                        std::filesystem::path abs = std::filesystem::absolute(c, ec);
                        return ec ? c.string() : abs.string();
                    }
                }
                return rel.string();
            };
            auto trim = [](std::string& s){
                const char* ws = " \t\r\n";
                size_t start = s.find_first_not_of(ws);
                size_t end = s.find_last_not_of(ws);
                if (start == std::string::npos) { s.clear(); return; }
                s = s.substr(start, end - start + 1);
            };
            std::string line;
            while (std::getline(listFile, line)) {
                if (line.empty()) continue;
                // skip comments and whitespace
                trim(line);
                if (line.empty() || line[0] == '#') continue;
                std::string resolved = resolvePathWithBase(line, list_dir);
                // Make absolute to avoid ROOT opening relative to the current build dir
                std::error_code ec_abs;
                std::filesystem::path abs_path = std::filesystem::absolute(std::filesystem::path(resolved), ec_abs);
                if (!ec_abs) resolved = abs_path.string();
                LogDebug << "Queued input file: '" << line << "' -> '" << resolved << "'" << std::endl;
                inputFiles.push_back(resolved);
            }
            listFile.close();
        }
    }
    LogThrowIf(inputFiles.empty(), "No input ROOT files provided. Set 'filename' or 'filelist' in JSON.");

    LogInfo << "Number of input files: " << inputFiles.size() << (has_filelist ? " (from list)" : "") << std::endl;

    // Determine output folder: command line option takes precedence, otherwise use JSON or derived from each input file when needed
    std::string fallbackOutfolder;
    if (clp.isOptionTriggered("outFolder")) {
        fallbackOutfolder = clp.getOptionVal<std::string>("outFolder");
        LogInfo << "Output folder (from command line): " << fallbackOutfolder << std::endl;
    } else if (j.contains("output_folder") && !j["output_folder"].get<std::string>().empty()) {
        fallbackOutfolder = j["output_folder"];
        LogInfo << "Output folder (from JSON): " << fallbackOutfolder << std::endl;
    }

    // Read optional parameters from JSON with defaults
    int tot_cut = j.value("tot_cut", 0);
    int tot_hist_max = j.value("tot_hist_max", 200); // max ToT to display in histograms (samples)
    int adc_integral_hist_max = j.value("adc_integral_hist_max", 300000); // upper range for ADC integral histograms
    LogInfo << "ToT cut: " << tot_cut << std::endl;

    // Track outputs to summarize at end
    std::vector<std::string> producedFiles;

    // Process each input file independently
    for (const auto& inFile : inputFiles) {
        // Resolve to an absolute path if possible
    std::string inFileResolved = inFile;
        {
            std::error_code ec_abs;
            std::filesystem::path cand = std::filesystem::path(inFile);
            if (!cand.is_absolute()) {
                // try current dir, JSON dir, and repo root relative candidates
        std::filesystem::path repo_root = json_dir.parent_path().parent_path();
                std::vector<std::filesystem::path> candidates = {
                    std::filesystem::current_path() / cand,
                    json_dir / cand,
                    json_dir.parent_path() / cand,
            json_dir.parent_path().parent_path() / cand,
            repo_root / cand
                };
                for (const auto& c : candidates) {
                    if (std::filesystem::exists(c)) { inFileResolved = std::filesystem::absolute(c).string(); break; }
                }
            } else {
                if (std::filesystem::exists(cand)) inFileResolved = cand.string();
            }
        }
    bool resolvedExists = std::filesystem::exists(std::filesystem::path(inFileResolved));
    LogInfo << "Input ROOT file: " << inFile << " | resolved: " << inFileResolved << " | exists: " << (resolvedExists?"yes":"no") << std::endl;
        // Determine output folder for this file
        std::string outfolder;
        if (!fallbackOutfolder.empty()) outfolder = fallbackOutfolder;
        else if (j.contains("output_folder") && !j["output_folder"].get<std::string>().empty()) outfolder = j["output_folder"];
        else {
            outfolder = inFile.substr(0, inFile.find_last_of("/\\"));
            if (outfolder.empty()) outfolder = ".";
        }

        // Open the ROOT file
        TFile *file = TFile::Open(inFileResolved.c_str());
        if (!file || file->IsZombie()) {
            LogError << "Error opening file: " << inFileResolved << std::endl;
            if (file) { file->Close(); delete file; }
            continue; // process next file
        }

    // Open the TPs tree inside the "tps" directory
    TDirectory* tpsDir = file->GetDirectory("tps");
    if (tpsDir == nullptr) {
        LogError << "Directory 'tps' not found in file: " << inFile << std::endl;
        file->Close();
        delete file;
        continue;
    }
    TTree *tpTree = dynamic_cast<TTree*>(tpsDir->Get("tps"));
    if (!tpTree) {
        LogError << "Tree 'tps' not found in directory 'tps' for file: " << inFile << std::endl;
        file->Close();
        delete file;
        continue;
    }

    // Optional diagnostics: read true_particles to detect MARLEY truth per event and build channel/time maps
    std::set<int> events_with_marley_truth;
    std::map<int, double> event_nu_energy; // per-event neutrino energy [MeV]
    // Extended neutrino kinematics (optional branches): position (x,y,z) and time t
    struct NuKinematics { double en{-1}; double x{0}; double y{0}; double z{0}; double t{0}; bool hasXYZ{false}; bool hasT{false}; };
    std::map<int, NuKinematics> event_nu_info; // event -> kinematics
    // For backtracking checks:
    std::unordered_map<int, std::unordered_set<int>> event_union_channels; // event -> set of channels from truth
    auto makeTruthKey = [](int evt, int tid) -> long long { return ( (static_cast<long long>(evt) << 32) | (static_cast<unsigned int>(tid)) ); };
    std::unordered_map<long long, std::pair<double,double>> truth_time_window; // (evt,truth_id) -> [tmin,tmax]
    if (auto* tTreeTruth = dynamic_cast<TTree*>(tpsDir->Get("true_particles"))) {
        int tevt = 0; std::string* tgen = nullptr; double tstart=0.0, tend=0.0; std::vector<int>* channels=nullptr; int ttruth=0;
        tTreeTruth->SetBranchAddress("event", &tevt);
        tTreeTruth->SetBranchAddress("generator_name", &tgen);
        if (tTreeTruth->GetBranch("time_start")) tTreeTruth->SetBranchAddress("time_start", &tstart);
        if (tTreeTruth->GetBranch("time_end"))   tTreeTruth->SetBranchAddress("time_end", &tend);
        if (tTreeTruth->GetBranch("channels"))   tTreeTruth->SetBranchAddress("channels", &channels);
        if (tTreeTruth->GetBranch("truth_id"))   tTreeTruth->SetBranchAddress("truth_id", &ttruth);
        Long64_t ntruth = tTreeTruth->GetEntries();
        for (Long64_t i=0;i<ntruth;++i){
            tTreeTruth->GetEntry(i);
            if (tgen){ std::string low=*tgen; std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){return (char)std::tolower(c);}); if (low.find("marley")!=std::string::npos) events_with_marley_truth.insert(tevt); }
            // union channels per event for fast membership checks
            if (channels){ auto &uset = event_union_channels[tevt]; for (int ch : *channels) uset.insert(ch); }
            // time window per (event, truth_id)
            truth_time_window[ makeTruthKey(tevt, ttruth) ] = { tstart, tend };
        }
    }
    // Read neutrino energies per event if the 'neutrinos' tree exists
    if (auto* nuTree = dynamic_cast<TTree*>(tpsDir->Get("neutrinos"))) {
        int nevt = 0; int nen = 0; float nux=0.f, nuy=0.f, nuz=0.f, nut=0.f; // use float to match typical branch types
        nuTree->SetBranchAddress("event", &nevt);
        if (nuTree->GetBranch("en")) nuTree->SetBranchAddress("en", &nen);
        // Try multiple possible branch name variants for position/time
        auto bindFirstAvailable = [&](const std::vector<std::string>& names, float& var, bool& flag){
            for (const auto& n : names) {
                if (nuTree->GetBranch(n.c_str())) { nuTree->SetBranchAddress(n.c_str(), &var); flag = true; return; }
            }
        };
        bool hasX=false, hasY=false, hasZ=false, hasT=false;
        bindFirstAvailable({"x","nu_x","pos_x","vx"}, nux, hasX);
        bindFirstAvailable({"y","nu_y","pos_y","vy"}, nuy, hasY);
        bindFirstAvailable({"z","nu_z","pos_z","vz"}, nuz, hasZ);
        bindFirstAvailable({"t","time","t0","nu_t"}, nut, hasT);
        Long64_t nnu = nuTree->GetEntries();
        for (Long64_t i=0;i<nnu;++i){
            nuTree->GetEntry(i);
            event_nu_energy[nevt] = static_cast<double>(nen);
            NuKinematics k; k.en = (double)nen; k.x=(double)nux; k.y=(double)nuy; k.z=(double)nuz; k.t=(double)nut; k.hasXYZ = (hasX||hasY||hasZ); k.hasT = hasT; event_nu_info[nevt] = k;
        }
        LogInfo << "Neutrino kinematics: entries=" << nnu << ", with position=" << ( (hasX||hasY||hasZ)?"yes":"no" ) << ", time=" << (hasT?"yes":"no") << std::endl;
    }

    // Branches we need from the TPs tree
    int tp_event = 0;
    ULong64_t samples_over_threshold = 0;
    UShort_t adc_peak = 0;
    UInt_t adc_integral = 0;
    UInt_t ch_offline = 0; // TP channel (offline)
    Int_t tp_truth_id = -1; // linked truth id if any
    Float_t time_offset_correction = 0.0f; // time offset correction (TPC ticks)
    std::string* view = nullptr;
    std::string* generator_name = nullptr;
    tpTree->SetBranchAddress("event", &tp_event);
    tpTree->SetBranchAddress("samples_over_threshold", &samples_over_threshold);
    tpTree->SetBranchAddress("adc_peak", &adc_peak);
    if (tpTree->GetBranch("adc_integral")) tpTree->SetBranchAddress("adc_integral", &adc_integral);
    tpTree->SetBranchAddress("view", &view);
    tpTree->SetBranchAddress("generator_name", &generator_name);
    if (tpTree->GetBranch("channel")) tpTree->SetBranchAddress("channel", &ch_offline);
    if (tpTree->GetBranch("truth_id")) tpTree->SetBranchAddress("truth_id", &tp_truth_id);
    if (tpTree->GetBranch("time_offset_correction")) tpTree->SetBranchAddress("time_offset_correction", &time_offset_correction);

    // We'll compute per-plane entry counts while looping
    int nentries_X = 0;
    int nentries_U = 0;
    int nentries_V = 0;

    // Avoid ROOT directory ownership to prevent name collisions across files
    TH1::AddDirectory(kFALSE);
    // Create integer-aligned histograms (1 bin per ADC code); coarse views via Rebin(2)
    const double adc_lo = 54.5;
    const double adc_hi = 800.5;
    const int bins_adc_int = static_cast<int>(adc_hi - adc_lo); // e.g. 800.5-54.5 = 746 bins
    TH1F *h_peak_all_fine = new TH1F("h_peak_all_fine", "ADC Peak (All Planes)", bins_adc_int, adc_lo, adc_hi);
    TH1F *h_peak_X_fine   = new TH1F("h_peak_X_fine",   "ADC Peak (Plane X)",  bins_adc_int, adc_lo, adc_hi);
    TH1F *h_peak_U_fine   = new TH1F("h_peak_U_fine",   "ADC Peak (Plane U)",  bins_adc_int, adc_lo, adc_hi);
    TH1F *h_peak_V_fine   = new TH1F("h_peak_V_fine",   "ADC Peak (Plane V)",  bins_adc_int, adc_lo, adc_hi);
    // MARLEY-only ADC peak histograms
    TH1F *h_peak_all_marley_fine = new TH1F("h_peak_all_marley_fine", "ADC Peak (All Planes, MARLEY)", bins_adc_int, adc_lo, adc_hi);
    TH1F *h_peak_X_marley_fine   = new TH1F("h_peak_X_marley_fine",   "ADC Peak (Plane X, MARLEY)",  bins_adc_int, adc_lo, adc_hi);
    TH1F *h_peak_U_marley_fine   = new TH1F("h_peak_U_marley_fine",   "ADC Peak (Plane U, MARLEY)",  bins_adc_int, adc_lo, adc_hi);
    TH1F *h_peak_V_marley_fine   = new TH1F("h_peak_V_marley_fine",   "ADC Peak (Plane V, MARLEY)",  bins_adc_int, adc_lo, adc_hi);


    // Reset histogram statistics (avoid global ROOT resets that can invalidate trees/branches)
    h_peak_all_fine->Reset();
    h_peak_X_fine->Reset();
    h_peak_U_fine->Reset();
    h_peak_V_fine->Reset();
    h_peak_all_marley_fine->Reset();
    h_peak_X_marley_fine->Reset();
    h_peak_U_marley_fine->Reset();
    h_peak_V_marley_fine->Reset();

    // ToT histograms (after ToT cut): integer-aligned bins from -0.5..tot_hist_max+0.5
    int tot_bins = std::max(1, tot_hist_max + 1);
    double tot_lo = -0.5;
    double tot_hi = tot_hist_max + 0.5;
    TH1F *h_tot_all = new TH1F("h_tot_all", "ToT (All Planes)", tot_bins, tot_lo, tot_hi);
    TH1F *h_tot_X   = new TH1F("h_tot_X",   "ToT (Plane X)",    tot_bins, tot_lo, tot_hi);
    TH1F *h_tot_U   = new TH1F("h_tot_U",   "ToT (Plane U)",    tot_bins, tot_lo, tot_hi);
    TH1F *h_tot_V   = new TH1F("h_tot_V",   "ToT (Plane V)",    tot_bins, tot_lo, tot_hi);
    h_tot_all->Reset(); h_tot_X->Reset(); h_tot_U->Reset(); h_tot_V->Reset();

    // ADC vs ToT (after ToT cut)
    TH2F *h_adc_vs_tot_all = new TH2F("h_adc_vs_tot_all", "ADC Peak vs ToT (All Planes);ToT [samples];ADC peak", tot_bins, tot_lo, tot_hi, bins_adc_int/2, adc_lo, adc_hi);
    TH2F *h_adc_vs_tot_X   = new TH2F("h_adc_vs_tot_X",   "ADC Peak vs ToT (X);ToT [samples];ADC peak",        tot_bins, tot_lo, tot_hi, bins_adc_int/2, adc_lo, adc_hi);
    TH2F *h_adc_vs_tot_U   = new TH2F("h_adc_vs_tot_U",   "ADC Peak vs ToT (U);ToT [samples];ADC peak",        tot_bins, tot_lo, tot_hi, bins_adc_int/2, adc_lo, adc_hi);
    TH2F *h_adc_vs_tot_V   = new TH2F("h_adc_vs_tot_V",   "ADC Peak vs ToT (V);ToT [samples];ADC peak",        tot_bins, tot_lo, tot_hi, bins_adc_int/2, adc_lo, adc_hi);

    // MARLEY-only ToT histograms (after ToT cut)
    TH1F *h_tot_all_marley = new TH1F("h_tot_all_marley", "ToT (MARLEY, All Planes)", tot_bins, tot_lo, tot_hi);
    TH1F *h_tot_X_marley   = new TH1F("h_tot_X_marley",   "ToT (MARLEY, Plane X)",    tot_bins, tot_lo, tot_hi);
    TH1F *h_tot_U_marley   = new TH1F("h_tot_U_marley",   "ToT (MARLEY, Plane U)",    tot_bins, tot_lo, tot_hi);
    TH1F *h_tot_V_marley   = new TH1F("h_tot_V_marley",   "ToT (MARLEY, Plane V)",    tot_bins, tot_lo, tot_hi);
    h_tot_all_marley->Reset(); h_tot_X_marley->Reset(); h_tot_U_marley->Reset(); h_tot_V_marley->Reset();

    // ADC integral histograms (after ToT cut)
    const int bins_adc_intgl = 200; // coarse, wide-range
    double int_lo = 0.0;
    double int_hi = 30000; //std::max(1000, adc_integral_hist_max);
    TH1F *h_int_all = new TH1F("h_int_all", "ADC Integral (All Planes);ADC integral;Entries", bins_adc_intgl, int_lo, int_hi);
    TH1F *h_int_X   = new TH1F("h_int_X",   "ADC Integral (Plane X);ADC integral;Entries",     bins_adc_intgl, int_lo, int_hi);
    TH1F *h_int_U   = new TH1F("h_int_U",   "ADC Integral (Plane U);ADC integral;Entries",     bins_adc_intgl, int_lo, int_hi);
    TH1F *h_int_V   = new TH1F("h_int_V",   "ADC Integral (Plane V);ADC integral;Entries",     bins_adc_intgl, int_lo, int_hi);

    // Prepare label counting across all planes, per plane, and per event
    std::map<std::string, long long> label_tp_counts; // all planes
    std::map<std::string, long long> label_tp_counts_X_plane;
    std::map<std::string, long long> label_tp_counts_U_plane;
    std::map<std::string, long long> label_tp_counts_V_plane;
    std::map<int, std::map<std::string, long long>> event_label_counts; // combined across planes per event
    // Track MARLEY presence per event and per plane for diagnostics
    std::map<int, bool> event_has_marley_X;
    std::map<int, bool> event_has_marley_U;
    std::map<int, bool> event_has_marley_V;
    // Track time offset corrections per event
    std::map<int, double> event_time_offsets; // event -> offset in TPC ticks

    // Single pass over the TPs to fill histograms and label counts
    LogInfo << "Processing TPs..." << std::endl;
    {
        bool warned_null_once = false;
        Long64_t nentries = tpTree->GetEntries();
        // Counters to diagnose UNKNOWN causes
        long long unknown_total = 0, unknown_in_union = 0, unknown_not_in_union = 0;
        // Keep track for tail composition; we'll tally after thresholds are computed (from histograms)
        struct TailCounters { long long marley=0, known_nonmarley=0, unk_in_union=0, unk_not_in_union=0; } adc99, tot99, int99, adc95, tot95, int95;
        for (Long64_t i = 0; i < nentries; ++i) {
            Long64_t nb = tpTree->GetEntry(i);
            if (nb <= 0 || view == nullptr || generator_name == nullptr) {
                if (!warned_null_once) {
                    LogWarning << "Null branch data or no bytes read at entry " << i << "; skipping (will suppress further warnings)." << std::endl;
                    warned_null_once = true;
                }
                continue;
            }
            if (static_cast<long long>(samples_over_threshold) > tot_cut) {
                const std::string &v = *view;
                const std::string &lab = *generator_name;
                bool is_marley = false; {
                    std::string low = lab; std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return (char)std::tolower(c); });
                    if (low.find("marley") != std::string::npos) is_marley = true;
                }
                bool is_unknown = false; {
                    std::string low = *generator_name; std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return (char)std::tolower(c); });
                    is_unknown = (low == "unknown" || tp_truth_id < 0);
                }
                if (v == "X") { h_peak_X_fine->Fill(adc_peak); nentries_X++; label_tp_counts_X_plane[lab]++; }
                else if (v == "U") { h_peak_U_fine->Fill(adc_peak); nentries_U++; label_tp_counts_U_plane[lab]++; }
                else if (v == "V") { h_peak_V_fine->Fill(adc_peak); nentries_V++; label_tp_counts_V_plane[lab]++; }
                // all planes
                h_peak_all_fine->Fill(adc_peak);
                // ToT hists and ADC vs ToT
                double tot = static_cast<double>(samples_over_threshold);
                if (v == "X") { h_tot_X->Fill(tot); h_adc_vs_tot_X->Fill(tot, adc_peak); }
                else if (v == "U") { h_tot_U->Fill(tot); h_adc_vs_tot_U->Fill(tot, adc_peak); }
                else if (v == "V") { h_tot_V->Fill(tot); h_adc_vs_tot_V->Fill(tot, adc_peak); }
                h_tot_all->Fill(tot);
                h_adc_vs_tot_all->Fill(tot, adc_peak);
                // ADC integral histograms (if branch available)
                if (tpTree->GetBranch("adc_integral")){
                    double ain = static_cast<double>(adc_integral);
                    if (v == "X") h_int_X->Fill(ain);
                    else if (v == "U") h_int_U->Fill(ain);
                    else if (v == "V") h_int_V->Fill(ain);
                    h_int_all->Fill(ain);
                }
                // MARLEY-only peaks and per-plane presence flags
                if (is_marley) {
                    // per-plane presence per event
                    if (v == "X") { event_has_marley_X[tp_event] = true; }
                    else if (v == "U") { event_has_marley_U[tp_event] = true; }
                    else if (v == "V") { event_has_marley_V[tp_event] = true; }
                    // ToT (MARLEY)
                    if (v == "X") { h_tot_X_marley->Fill(tot); }
                    else if (v == "U") { h_tot_U_marley->Fill(tot); }
                    else if (v == "V") { h_tot_V_marley->Fill(tot); }
                    h_tot_all_marley->Fill(tot);
                    if (v == "X") { h_peak_X_marley_fine->Fill(adc_peak); }
                    else if (v == "U") { h_peak_U_marley_fine->Fill(adc_peak); }
                    else if (v == "V") { h_peak_V_marley_fine->Fill(adc_peak); }
                    h_peak_all_marley_fine->Fill(adc_peak);
                }
                // Track UNKNOWN membership wrt union of truth channels (per event)
                if (is_unknown) {
                    unknown_total++;
                    auto it = event_union_channels.find(tp_event);
                    bool in_union = (it != event_union_channels.end() && it->second.find(static_cast<int>(ch_offline)) != it->second.end());
                    if (in_union) unknown_in_union++; else unknown_not_in_union++;
                }
                // Store time offset correction per event (only once per event)
                if (event_time_offsets.find(tp_event) == event_time_offsets.end()) {
                    event_time_offsets[tp_event] = static_cast<double>(time_offset_correction);
                }
                
                label_tp_counts[lab]++;
                event_label_counts[tp_event][lab]++;
            }
        }
        LogInfo << "UNKNOWN diagnostic: total=" << unknown_total << ", in-union(ch)=" << unknown_in_union << ", not-in-union(ch)=" << unknown_not_in_union << std::endl;
    }

    LogInfo << "Number of TPs after ToT cut - X: " << nentries_X << ", U: " << nentries_U << ", V: " << nentries_V << std::endl;

    // Set ROOT style
    gStyle->SetOptStat(111111);

    // Create PDF report with multiple pages
    LogInfo << "Creating PDF report..." << std::endl;
    // Extract base filename and create PDF output path
    std::string base_filename = inFile.substr(inFile.find_last_of("/\\") + 1);
    size_t dot_pos = base_filename.find_last_of(".");
    if (dot_pos != std::string::npos) {
        base_filename = base_filename.substr(0, dot_pos);
    }
    // If input is *_tps_bktr<N>.root, propagate _bktr<N> into the PDF name
    std::string bktr_suffix;
    {
        // find pattern "_tps_bktr" and copy remainder until extension
        size_t pos = base_filename.find("_tps_bktr");
        if (pos != std::string::npos) {
            bktr_suffix = base_filename.substr(pos + std::string("_tps").size()); // yields _bktr<N>
        }
    }
    std::string pdf_output = outfolder + "/" + base_filename + "_tot" + std::to_string(tot_cut) + ".pdf";
    // Absolute path for reporting
    std::string pdf_output_abs = pdf_output;
    {
        std::error_code _ec_pdf_abs;
        auto abs_p = std::filesystem::absolute(std::filesystem::path(pdf_output), _ec_pdf_abs);
        if (!_ec_pdf_abs) pdf_output_abs = abs_p.string();
    }
    
    // Create title page
    TCanvas *c_title = new TCanvas("c_title", "Title Page", 800, 600);
    c_title->SetFillColor(kWhite);
    c_title->cd();
    
    // Add title text
    TText *title = new TText(0.5, 0.8, "DUNE Online Pointing Analysis Report");
    title->SetTextAlign(22);
    title->SetTextSize(0.06);
    title->SetTextFont(62);
    title->SetNDC();
    title->Draw();
    
    TText *subtitle = new TText(0.5, 0.7, "ADC Peak Analysis and Energy-Charge Correlations");
    subtitle->SetTextAlign(22);
    subtitle->SetTextSize(0.04);
    subtitle->SetTextFont(42);
    subtitle->SetNDC();
    subtitle->Draw();
    
    // Add file info
    TText *file_info = new TText(0.5, 0.5, Form("Input file: %s", inFile.c_str()));
    file_info->SetTextAlign(22);
    file_info->SetTextSize(0.025);
    file_info->SetTextFont(42);
    file_info->SetNDC();
    file_info->Draw();
    
    TText *stats_info = new TText(0.5, 0.45, Form("Number of TPs (after ToT cut) - X: %d, U: %d, V: %d", nentries_X, nentries_U, nentries_V));
    stats_info->SetTextAlign(22);
    stats_info->SetTextSize(0.025);
    stats_info->SetTextFont(42);
    stats_info->SetNDC();
    stats_info->Draw();
    
    TText *tot_info = new TText(0.5, 0.4, Form("ToT cut: %d", tot_cut));
    tot_info->SetTextAlign(22);
    tot_info->SetTextSize(0.025);
    tot_info->SetTextFont(42);
    tot_info->SetNDC();
    tot_info->Draw();
    
    // Get current date/time
    TDatime now;
    TText *date_info = new TText(0.5, 0.2, Form("Generated on: %s", now.AsString()));
    date_info->SetTextAlign(22);
    date_info->SetTextSize(0.02);
    date_info->SetTextFont(42);
    date_info->SetNDC();
    date_info->Draw();
    
    // Save title page as first page of PDF
    std::string pdf_first_page = pdf_output + "(";
    c_title->SaveAs(pdf_first_page.c_str());
    LogInfo << "Title page saved to PDF" << std::endl;

    // Prepare coarse clones for display on non-zoomed page
    TH1F *h_peak_all_coarse = (TH1F*) h_peak_all_fine->Clone("h_peak_all_coarse");
    TH1F *h_peak_X_coarse   = (TH1F*) h_peak_X_fine->Clone("h_peak_X_coarse");
    TH1F *h_peak_U_coarse   = (TH1F*) h_peak_U_fine->Clone("h_peak_U_coarse");
    TH1F *h_peak_V_coarse   = (TH1F*) h_peak_V_fine->Clone("h_peak_V_coarse");
    // Rebin to a coarser view; choose a factor that exactly divides bins_adc_int to avoid ROOT warnings
    h_peak_all_coarse->Rebin(2);
    h_peak_X_coarse->Rebin(2);
    h_peak_U_coarse->Rebin(2);
    h_peak_V_coarse->Rebin(2);

    // Configure styles for non-zoomed (coarse) display
    auto style_hist = [](TH1F* h, Color_t line, Color_t fill){
        h->SetOption("HIST");
        h->SetXTitle("ADC Peak");
        h->SetYTitle("Entries");
        h->SetTitle("ADC Peak Histogram");
        h->SetLineColor(line);
        h->SetFillColorAlpha(fill, 0.20);
    };
    style_hist(h_peak_all_coarse, kBlack, kBlack);
    style_hist(h_peak_X_coarse,   kBlue,  kBlue);
    style_hist(h_peak_U_coarse,   kRed,   kRed);
    style_hist(h_peak_V_coarse,   kGreen+2, kGreen+2);

    // Configure styles for zoomed (fine) display
    auto style_hist_fine = [](TH1F* h, const char* title, Color_t line, Color_t fill){
        h->SetOption("HIST");
        h->SetXTitle("ADC Peak");
        h->SetYTitle("Entries");
        h->SetTitle(title);
        h->SetLineColor(line);
        h->SetFillColorAlpha(fill, 0.20);
    };
    style_hist_fine(h_peak_all_fine, "ADC Peak Histogram (Fine)", kBlack, kBlack);
    style_hist_fine(h_peak_X_fine,   "ADC Peak Histogram (Fine)", kBlue,  kBlue);
    style_hist_fine(h_peak_U_fine,   "ADC Peak Histogram (Fine)", kRed,   kRed);
    style_hist_fine(h_peak_V_fine,   "ADC Peak Histogram (Fine)", kGreen+2, kGreen+2);
    style_hist_fine(h_peak_all_marley_fine, "ADC Peak Histogram (MARLEY, Fine)", kMagenta+2, kMagenta+2);
    style_hist_fine(h_peak_X_marley_fine,   "ADC Peak Histogram (MARLEY, Fine)", kMagenta+2, kMagenta+2);
    style_hist_fine(h_peak_U_marley_fine,   "ADC Peak Histogram (MARLEY, Fine)", kMagenta+2, kMagenta+2);
    style_hist_fine(h_peak_V_marley_fine,   "ADC Peak Histogram (MARLEY, Fine)", kMagenta+2, kMagenta+2);
    // Make MARLEY histograms line-only for overlay clarity
    auto style_peak_m_overlay = [](TH1F* h){ if(!h) return; h->SetFillStyle(0); h->SetLineColor(kMagenta+2); h->SetLineWidth(2); };
    style_peak_m_overlay(h_peak_all_marley_fine);
    style_peak_m_overlay(h_peak_X_marley_fine);
    style_peak_m_overlay(h_peak_U_marley_fine);
    style_peak_m_overlay(h_peak_V_marley_fine);

    // Create canvas for ADC peak histograms (Page 2)
    LogInfo << "Creating ADC peak plots..." << std::endl;
    TCanvas *c1 = new TCanvas("c1", "ADC Peak by Plane", 1000, 800);
    c1->Divide(2,2);

    c1->cd(1);
    gPad->SetLogy();
    h_peak_all_coarse->Draw("HIST");
    if (h_peak_all_marley_fine && h_peak_all_marley_fine->GetEntries()>0) h_peak_all_marley_fine->Draw("HIST SAME");
    TLegend *leg_all = new TLegend(0.50, 0.78, 0.88, 0.92);
    leg_all->SetBorderSize(0);
    leg_all->AddEntry(h_peak_all_coarse, "All Planes (All)", "f");
    if (h_peak_all_marley_fine && h_peak_all_marley_fine->GetEntries()>0) leg_all->AddEntry(h_peak_all_marley_fine, "All Planes (MARLEY)", "l");
    leg_all->Draw();

    c1->cd(2);
    gPad->SetLogy();
    h_peak_X_coarse->Draw("HIST");
    if (h_peak_X_marley_fine && h_peak_X_marley_fine->GetEntries()>0) h_peak_X_marley_fine->Draw("HIST SAME");
    TLegend *leg_X = new TLegend(0.50, 0.78, 0.88, 0.92);
    leg_X->SetBorderSize(0);
    leg_X->AddEntry(h_peak_X_coarse, "Plane X (All)", "f");
    if (h_peak_X_marley_fine && h_peak_X_marley_fine->GetEntries()>0) leg_X->AddEntry(h_peak_X_marley_fine, "Plane X (MARLEY)", "l");
    leg_X->Draw();

    c1->cd(3);
    gPad->SetLogy();
    h_peak_U_coarse->Draw("HIST");
    if (h_peak_U_marley_fine && h_peak_U_marley_fine->GetEntries()>0) h_peak_U_marley_fine->Draw("HIST SAME");
    TLegend *leg_U = new TLegend(0.50, 0.78, 0.88, 0.92);
    leg_U->SetBorderSize(0);
    leg_U->AddEntry(h_peak_U_coarse, "Plane U (All)", "f");
    if (h_peak_U_marley_fine && h_peak_U_marley_fine->GetEntries()>0) leg_U->AddEntry(h_peak_U_marley_fine, "Plane U (MARLEY)", "l");
    leg_U->Draw();

    c1->cd(4);
    gPad->SetLogy();
    h_peak_V_coarse->Draw("HIST");
    if (h_peak_V_marley_fine && h_peak_V_marley_fine->GetEntries()>0) h_peak_V_marley_fine->Draw("HIST SAME");
    TLegend *leg_V = new TLegend(0.50, 0.78, 0.88, 0.92);
    leg_V->SetBorderSize(0);
    leg_V->AddEntry(h_peak_V_coarse, "Plane V (All)", "f");
    if (h_peak_V_marley_fine && h_peak_V_marley_fine->GetEntries()>0) leg_V->AddEntry(h_peak_V_marley_fine, "Plane V (MARLEY)", "l");
    leg_V->Draw();

    // Save second page of PDF
    c1->SaveAs(pdf_output.c_str());
    LogInfo << "ADC peak plots saved to PDF (page 2)" << std::endl;

    // Create canvas for ADC peak histograms zoomed to 0-250 (Page 3)
    LogInfo << "Creating ADC peak plots (zoomed 0-250)..." << std::endl;
    TCanvas *c1_zoom = new TCanvas("c1_zoom", "ADC Peak by Plane (Zoomed 0-250)", 1000, 800);
    c1_zoom->Divide(2,2);

    c1_zoom->cd(1);
    gPad->SetLogy();
    h_peak_all_fine->GetXaxis()->SetRangeUser(0, 250);
    h_peak_all_fine->Draw("HIST");
    if (h_peak_all_marley_fine && h_peak_all_marley_fine->GetEntries()>0) { h_peak_all_marley_fine->GetXaxis()->SetRangeUser(0, 250); h_peak_all_marley_fine->Draw("HIST SAME"); }
    TLegend *leg_all_zoom = new TLegend(0.50, 0.78, 0.88, 0.92);
    leg_all_zoom->SetBorderSize(0);
    leg_all_zoom->AddEntry(h_peak_all_fine, "All Planes (All)", "f");
    if (h_peak_all_marley_fine && h_peak_all_marley_fine->GetEntries()>0) leg_all_zoom->AddEntry(h_peak_all_marley_fine, "All Planes (MARLEY)", "l");
    leg_all_zoom->Draw();

    c1_zoom->cd(2);
    gPad->SetLogy();
    h_peak_X_fine->GetXaxis()->SetRangeUser(0, 250);
    h_peak_X_fine->SetTitle("ADC Peak Histogram (Zoomed 0-250)");
    h_peak_X_fine->Draw("HIST");
    // Fit exponential in [60,70] for X
    {
        auto hasContent = [](TH1* h, double xmin, double xmax){
            if (!h) return false;
            int b1 = h->GetXaxis()->FindFixBin(xmin + 1e-6);
            int b2 = h->GetXaxis()->FindFixBin(xmax - 1e-6);
            if (b2 < b1) std::swap(b1, b2);
            return h->Integral(b1, b2) > 0; };
        int color = h_peak_X_fine->GetLineColor();
        if (hasContent(h_peak_X_fine, 60., 70.)) {
            h_peak_X_fine->Fit("expo", "RQ", "", 60., 70.);
            if (auto f = h_peak_X_fine->GetFunction("expo")) { f->SetLineColor(color); f->SetLineWidth(2); }
        }
    }
    if (h_peak_X_marley_fine && h_peak_X_marley_fine->GetEntries()>0) { h_peak_X_marley_fine->GetXaxis()->SetRangeUser(0, 250); h_peak_X_marley_fine->Draw("HIST SAME"); }
    TLegend *leg_X_zoom = new TLegend(0.50, 0.78, 0.88, 0.92);
    leg_X_zoom->SetBorderSize(0);
    leg_X_zoom->AddEntry(h_peak_X_fine, "Plane X (All)", "f");
    if (h_peak_X_marley_fine && h_peak_X_marley_fine->GetEntries()>0) leg_X_zoom->AddEntry(h_peak_X_marley_fine, "Plane X (MARLEY)", "l");
    leg_X_zoom->Draw();

    c1_zoom->cd(3);
    gPad->SetLogy();
    h_peak_U_fine->GetXaxis()->SetRangeUser(0, 250);
    h_peak_U_fine->SetTitle("ADC Peak Histogram (Zoomed 0-250)");
    h_peak_U_fine->Draw("HIST");
    // Fit exponential in [60,80] for U
    {
        auto hasContent = [](TH1* h, double xmin, double xmax){
            if (!h) return false;
            int b1 = h->GetXaxis()->FindFixBin(xmin + 1e-6);
            int b2 = h->GetXaxis()->FindFixBin(xmax - 1e-6);
            if (b2 < b1) std::swap(b1, b2);
            return h->Integral(b1, b2) > 0; };
        int color = h_peak_U_fine->GetLineColor();
        if (hasContent(h_peak_U_fine, 60., 80.)) {
            h_peak_U_fine->Fit("expo", "RQ", "", 60., 80.);
            if (auto f = h_peak_U_fine->GetFunction("expo")) { f->SetLineColor(color); f->SetLineWidth(2); }
        }
    }
    if (h_peak_U_marley_fine && h_peak_U_marley_fine->GetEntries()>0) { h_peak_U_marley_fine->GetXaxis()->SetRangeUser(0, 250); h_peak_U_marley_fine->Draw("HIST SAME"); }
    TLegend *leg_U_zoom = new TLegend(0.50, 0.78, 0.88, 0.92);
    leg_U_zoom->SetBorderSize(0);
    leg_U_zoom->AddEntry(h_peak_U_fine, "Plane U (All)", "f");
    if (h_peak_U_marley_fine && h_peak_U_marley_fine->GetEntries()>0) leg_U_zoom->AddEntry(h_peak_U_marley_fine, "Plane U (MARLEY)", "l");
    leg_U_zoom->Draw();

    c1_zoom->cd(4);
    gPad->SetLogy();
    h_peak_V_fine->GetXaxis()->SetRangeUser(0, 250);
    h_peak_V_fine->SetTitle("ADC Peak Histogram (Zoomed 0-250)");
    h_peak_V_fine->Draw("HIST");
    // Fit exponential in [60,80] for V
    {
        auto hasContent = [](TH1* h, double xmin, double xmax){
            if (!h) return false;
            int b1 = h->GetXaxis()->FindFixBin(xmin + 1e-6);
            int b2 = h->GetXaxis()->FindFixBin(xmax - 1e-6);
            if (b2 < b1) std::swap(b1, b2);
            return h->Integral(b1, b2) > 0; };
        int color = h_peak_V_fine->GetLineColor();
        if (hasContent(h_peak_V_fine, 60., 80.)) {
            h_peak_V_fine->Fit("expo", "RQ", "", 60., 80.);
            if (auto f = h_peak_V_fine->GetFunction("expo")) { f->SetLineColor(color); f->SetLineWidth(2); }
        }
    }
    if (h_peak_V_marley_fine && h_peak_V_marley_fine->GetEntries()>0) { h_peak_V_marley_fine->GetXaxis()->SetRangeUser(0, 250); h_peak_V_marley_fine->Draw("HIST SAME"); }
    TLegend *leg_V_zoom = new TLegend(0.50, 0.78, 0.88, 0.92);
    leg_V_zoom->SetBorderSize(0);
    leg_V_zoom->AddEntry(h_peak_V_fine, "Plane V (All)", "f");
    if (h_peak_V_marley_fine && h_peak_V_marley_fine->GetEntries()>0) leg_V_zoom->AddEntry(h_peak_V_marley_fine, "Plane V (MARLEY)", "l");
    leg_V_zoom->Draw();

    // Save third page of PDF
    c1_zoom->SaveAs(pdf_output.c_str());

    // Cleanup coarse clones used for non-zoomed page
    delete h_peak_all_coarse; h_peak_all_coarse = nullptr;
    delete h_peak_X_coarse;   h_peak_X_coarse = nullptr;
    delete h_peak_U_coarse;   h_peak_U_coarse = nullptr;
    delete h_peak_V_coarse;   h_peak_V_coarse = nullptr;
    LogInfo << "ADC peak plots (zoomed 0-250) saved to PDF (page 3)" << std::endl;

    // ToT distributions (Page 4): All, X, U, V with MARLEY overlays
    LogInfo << "Creating ToT distribution plots..." << std::endl;
    auto style_tot_hist = [](TH1F* h, const char* title, Color_t line, Color_t fill){
        h->SetOption("HIST");
        h->SetXTitle("ToT [samples over threshold]");
        h->SetYTitle("Entries");
        h->SetTitle(title);
        h->SetLineColor(line);
        h->SetFillColorAlpha(fill, 0.20);
    };
    style_tot_hist(h_tot_all, "ToT (All Planes)", kBlack, kBlack);
    style_tot_hist(h_tot_X,   "ToT (Plane X)",    kBlue,  kBlue);
    style_tot_hist(h_tot_U,   "ToT (Plane U)",    kRed,   kRed);
    style_tot_hist(h_tot_V,   "ToT (Plane V)",    kGreen+2, kGreen+2);

    // Style MARLEY overlays as line-only for clarity on top of filled histograms
    auto style_tot_m_overlay = [](TH1F* h){
        if (!h) return; h->SetLineColor(kMagenta+2); h->SetLineWidth(2); h->SetFillStyle(0); h->SetTitle((std::string(h->GetTitle()) + " (overlay)").c_str()); };
    style_tot_m_overlay(h_tot_all_marley);
    style_tot_m_overlay(h_tot_X_marley);
    style_tot_m_overlay(h_tot_U_marley);
    style_tot_m_overlay(h_tot_V_marley);

    // Helper to zoom X axis consistently
    auto zoomX = [](TH1* h, double xmin, double xmax){ if (h && h->GetXaxis()) h->GetXaxis()->SetRangeUser(xmin, xmax); };

    // Limit X axis to [0,40] for better visualization
    const double tot_xmin = 0.0, tot_xmax = 40.0;
    zoomX(h_tot_all, tot_xmin, tot_xmax);      zoomX(h_tot_all_marley, tot_xmin, tot_xmax);
    zoomX(h_tot_X,   tot_xmin, tot_xmax);      zoomX(h_tot_X_marley,   tot_xmin, tot_xmax);
    zoomX(h_tot_U,   tot_xmin, tot_xmax);      zoomX(h_tot_U_marley,   tot_xmin, tot_xmax);
    zoomX(h_tot_V,   tot_xmin, tot_xmax);      zoomX(h_tot_V_marley,   tot_xmin, tot_xmax);

    TCanvas *c_tot = new TCanvas("c_tot", "ToT distributions (with MARLEY overlays)", 1000, 800);
    c_tot->Divide(2,2);
    // Pad 1: All planes
    c_tot->cd(1); gPad->SetLogy();
    h_tot_all->Draw("HIST");
    if (h_tot_all_marley && h_tot_all_marley->GetEntries() > 0) h_tot_all_marley->Draw("HIST SAME");
    { auto *leg = new TLegend(0.55, 0.78, 0.88, 0.92); leg->SetBorderSize(0); leg->AddEntry(h_tot_all, "All Planes (All)", "f"); if (h_tot_all_marley && h_tot_all_marley->GetEntries()>0) leg->AddEntry(h_tot_all_marley, "All Planes (MARLEY)", "l"); leg->Draw(); }
    // Pad 2: X plane
    c_tot->cd(2); gPad->SetLogy();
    h_tot_X->Draw("HIST");
    if (h_tot_X_marley && h_tot_X_marley->GetEntries() > 0) h_tot_X_marley->Draw("HIST SAME");
    { auto *leg = new TLegend(0.55, 0.78, 0.88, 0.92); leg->SetBorderSize(0); leg->AddEntry(h_tot_X, "Plane X (All)", "f"); if (h_tot_X_marley && h_tot_X_marley->GetEntries()>0) leg->AddEntry(h_tot_X_marley, "Plane X (MARLEY)", "l"); leg->Draw(); }
    // Pad 3: U plane
    c_tot->cd(3); gPad->SetLogy();
    h_tot_U->Draw("HIST");
    if (h_tot_U_marley && h_tot_U_marley->GetEntries() > 0) h_tot_U_marley->Draw("HIST SAME");
    { auto *leg = new TLegend(0.55, 0.78, 0.88, 0.92); leg->SetBorderSize(0); leg->AddEntry(h_tot_U, "Plane U (All)", "f"); if (h_tot_U_marley && h_tot_U_marley->GetEntries()>0) leg->AddEntry(h_tot_U_marley, "Plane U (MARLEY)", "l"); leg->Draw(); }
    // Pad 4: V plane
    c_tot->cd(4); gPad->SetLogy();
    h_tot_V->Draw("HIST");
    if (h_tot_V_marley && h_tot_V_marley->GetEntries() > 0) h_tot_V_marley->Draw("HIST SAME");
    { auto *leg = new TLegend(0.55, 0.78, 0.88, 0.92); leg->SetBorderSize(0); leg->AddEntry(h_tot_V, "Plane V (All)", "f"); if (h_tot_V_marley && h_tot_V_marley->GetEntries()>0) leg->AddEntry(h_tot_V_marley, "Plane V (MARLEY)", "l"); leg->Draw(); }
    c_tot->SaveAs(pdf_output.c_str());

    // ADC vs ToT (Page 5): All, X, U, V
    LogInfo << "Creating ADC vs ToT plots..." << std::endl;
    TCanvas *c_adc_tot = new TCanvas("c_adc_tot", "ADC vs ToT", 1000, 800);
    c_adc_tot->Divide(2,2);
    auto draw_colz = [](){ gPad->SetRightMargin(0.13); gPad->SetGridx(); gPad->SetGridy(); gPad->SetLogz(); };
    c_adc_tot->cd(1); draw_colz(); h_adc_vs_tot_all->Draw("COLZ");
    c_adc_tot->cd(2); draw_colz(); h_adc_vs_tot_X->Draw("COLZ");
    c_adc_tot->cd(3); draw_colz(); h_adc_vs_tot_U->Draw("COLZ");
    c_adc_tot->cd(4); draw_colz(); h_adc_vs_tot_V->Draw("COLZ");
    c_adc_tot->SaveAs(pdf_output.c_str());

    // MARLEY-only ToT distributions are now overlaid on the ToT page above.

    // ADC integral distributions (Page 6): All, X, U, V
    LogInfo << "Creating ADC integral distribution plots..." << std::endl;
    auto style_int_hist = [](TH1F* h, Color_t line, Color_t fill){
        h->SetOption("HIST");
        h->SetLineColor(line);
        h->SetFillColorAlpha(fill, 0.20);
    };
    style_int_hist(h_int_all, kBlack, kBlack);
    style_int_hist(h_int_X,   kBlue,  kBlue);
    style_int_hist(h_int_U,   kRed,   kRed);
    style_int_hist(h_int_V,   kGreen+2, kGreen+2);
    TCanvas *c_int = new TCanvas("c_int", "ADC integral distributions", 1000, 800);
    c_int->Divide(2,2);
    c_int->cd(1); gPad->SetLogy(); h_int_all->Draw("HIST"); { TLegend *leg=new TLegend(0.6,0.8,0.88,0.92); leg->AddEntry(h_int_all, "All Planes", "f"); leg->Draw(); }
    c_int->cd(2); gPad->SetLogy(); h_int_X->Draw("HIST");   { TLegend *leg=new TLegend(0.6,0.8,0.88,0.92); leg->AddEntry(h_int_X,   "Plane X",   "f"); leg->Draw(); }
    c_int->cd(3); gPad->SetLogy(); h_int_U->Draw("HIST");   { TLegend *leg=new TLegend(0.6,0.8,0.88,0.92); leg->AddEntry(h_int_U,   "Plane U",   "f"); leg->Draw(); }
    c_int->cd(4); gPad->SetLogy(); h_int_V->Draw("HIST");   { TLegend *leg=new TLegend(0.6,0.8,0.88,0.92); leg->AddEntry(h_int_V,   "Plane V",   "f"); leg->Draw(); }
    c_int->SaveAs(pdf_output.c_str());

    // Backtracking sanity page: tail composition at 95% and 99%
    LogInfo << "Creating backtracking diagnostics page (tails composition)..." << std::endl;
    auto percentile_from_hist = [](TH1* h, double p) -> double {
        if (!h) return 0.0; double target = p * h->GetEntries(); double cum = 0.0; int nb = h->GetNbinsX();
        for (int b=1; b<=nb; ++b){ cum += h->GetBinContent(b); if (cum >= target){ return h->GetXaxis()->GetBinUpEdge(b); } }
        return h->GetXaxis()->GetXmax(); };
    double adc95_th = percentile_from_hist(h_peak_all_fine, 0.95);
    double adc99_th = percentile_from_hist(h_peak_all_fine, 0.99);
    double tot95_th = percentile_from_hist(h_tot_all, 0.95);
    double tot99_th = percentile_from_hist(h_tot_all, 0.99);
    double int95_th = percentile_from_hist(h_int_all, 0.95);
    double int99_th = percentile_from_hist(h_int_all, 0.99);

    // Re-scan TP tree quickly to tally tails by category
    {
        Long64_t nentries = tpTree->GetEntries();
        // ensure branches are still set (they are)
        long long adc95_m=0, adc95_kn=0, adc95_ui=0, adc95_un=0;
        long long adc99_m=0, adc99_kn=0, adc99_ui=0, adc99_un=0;
        long long tot95_m=0, tot95_kn=0, tot95_ui=0, tot95_un=0;
        long long tot99_m=0, tot99_kn=0, tot99_ui=0, tot99_un=0;
        long long int95_m=0, int95_kn=0, int95_ui=0, int95_un=0;
        long long int99_m=0, int99_kn=0, int99_ui=0, int99_un=0;
        for (Long64_t i=0;i<nentries;++i){ tpTree->GetEntry(i);
            if (static_cast<long long>(samples_over_threshold) <= tot_cut) continue; // same selection
            std::string low = *generator_name; std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return (char)std::tolower(c); });
            bool is_m = (low.find("marley") != std::string::npos);
            bool is_unk = (low == "unknown" || tp_truth_id < 0);
            bool is_kn = (!is_m && !is_unk);
            bool in_union = false; { auto it = event_union_channels.find(tp_event); in_union = (it != event_union_channels.end() && it->second.find(static_cast<int>(ch_offline)) != it->second.end()); }
            auto bump = [&](bool cond, long long &m, long long &kn, long long &ui, long long &un){ if (!cond) return; if (is_m) ++m; else if (is_kn) ++kn; else if (is_unk && in_union) ++ui; else ++un; };
            bump(adc_peak >= adc95_th, adc95_m, adc95_kn, adc95_ui, adc95_un);
            bump(adc_peak >= adc99_th, adc99_m, adc99_kn, adc99_ui, adc99_un);
            double tot = static_cast<double>(samples_over_threshold);
            bump(tot >= tot95_th,  tot95_m, tot95_kn, tot95_ui, tot95_un);
            bump(tot >= tot99_th,  tot99_m, tot99_kn, tot99_ui, tot99_un);
            double integ = static_cast<double>(adc_integral);
            bump(integ >= int95_th, int95_m, int95_kn, int95_ui, int95_un);
            bump(integ >= int99_th, int99_m, int99_kn, int99_ui, int99_un);
        }
        TCanvas *c_bktr = new TCanvas("c_bktr", "Backtracking diagnostics: tail composition", 1100, 700);
        c_bktr->Divide(2,1);
        c_bktr->cd(1); {
            gPad->SetMargin(0.12,0.06,0.12,0.06);
            TText t; t.SetTextFont(42); t.SetTextSize(0.032); double y=0.94, dy=0.055;
            auto line=[&](const std::string& s){ t.DrawTextNDC(0.12, y, s.c_str()); y-=dy; };
            char b[256]; snprintf(b,sizeof(b),"ADCpeak tails: th95=%.1f, th99=%.1f", adc95_th, adc99_th); line(b);
            snprintf(b,sizeof(b),"  95%%: MARLEY=%lld, KNOWN!=MARLEY=%lld, UNKNOWN(ch-in-union)=%lld, UNKNOWN(ch-out)=%lld", adc95_m, adc95_kn, adc95_ui, adc95_un); line(b);
            snprintf(b,sizeof(b),"  99%%: MARLEY=%lld, KNOWN!=MARLEY=%lld, UNKNOWN(ch-in-union)=%lld, UNKNOWN(ch-out)=%lld", adc99_m, adc99_kn, adc99_ui, adc99_un); line(b);
            y-=0.02; snprintf(b,sizeof(b),"ToT tails: th95=%.0f, th99=%.0f", tot95_th, tot99_th); line(b);
            snprintf(b,sizeof(b),"  95%%: MARLEY=%lld, KNOWN!=MARLEY=%lld, UNKNOWN(ch-in-union)=%lld, UNKNOWN(ch-out)=%lld", tot95_m, tot95_kn, tot95_ui, tot95_un); line(b);
            snprintf(b,sizeof(b),"  99%%: MARLEY=%lld, KNOWN!=MARLEY=%lld, UNKNOWN(ch-in-union)=%lld, UNKNOWN(ch-out)=%lld", tot99_m, tot99_kn, tot99_ui, tot99_un); line(b);
            y-=0.02; snprintf(b,sizeof(b),"Integral tails: th95=%.0f, th99=%.0f", int95_th, int99_th); line(b);
            snprintf(b,sizeof(b),"  95%%: MARLEY=%lld, KNOWN!=MARLEY=%lld, UNKNOWN(ch-in-union)=%lld, UNKNOWN(ch-out)=%lld", int95_m, int95_kn, int95_ui, int95_un); line(b);
            snprintf(b,sizeof(b),"  99%%: MARLEY=%lld, KNOWN!=MARLEY=%lld, UNKNOWN(ch-in-union)=%lld, UNKNOWN(ch-out)=%lld", int99_m, int99_kn, int99_ui, int99_un); line(b);
        }
        c_bktr->cd(2); {
            // Simple bar chart for ADC 99%% composition
            TH1F *hbar = new TH1F("h_adc99_comp","ADCpeak > 99th composition;Category;Count",4,0,4);
            hbar->SetStats(0);
            hbar->GetXaxis()->SetBinLabel(1,"MARLEY"); hbar->GetXaxis()->SetBinLabel(2,"KNOWN!=MARLEY"); hbar->GetXaxis()->SetBinLabel(3,"UNK ch-in-union"); hbar->GetXaxis()->SetBinLabel(4,"UNK ch-out");
            hbar->SetBinContent(1, adc99_m); hbar->SetBinContent(2, adc99_kn); hbar->SetBinContent(3, adc99_ui); hbar->SetBinContent(4, adc99_un);
            hbar->SetFillColorAlpha(kMagenta+2,0.35); hbar->SetLineColor(kMagenta+2);
            gPad->SetGridx(); gPad->SetGridy(); gPad->SetLogy();
            hbar->Draw("HIST");
        }
        c_bktr->SaveAs(pdf_output.c_str());
        delete c_bktr;
    }

    // New page: MARLEY presence per plane (event-level)
    LogInfo << "Creating MARLEY per-plane diagnostic page..." << std::endl;
    {
        // Gather event IDs we considered
        std::vector<int> event_ids; event_ids.reserve(event_label_counts.size());
        for (const auto &kv : event_label_counts) event_ids.push_back(kv.first);
        std::sort(event_ids.begin(), event_ids.end());
        int total_events = static_cast<int>(event_ids.size());
    int evX = 0, evU = 0, evV = 0;
    int evAllThree = 0, evIndAny = 0, evIndBoth = 0, evXOnly = 0, evIndOnly = 0, evNone = 0, evXnotInd = 0, evIndNotX = 0;
        for (int evt : event_ids) {
            bool hx = (event_has_marley_X.find(evt) != event_has_marley_X.end());
            bool hu = (event_has_marley_U.find(evt) != event_has_marley_U.end());
            bool hv = (event_has_marley_V.find(evt) != event_has_marley_V.end());
            bool hind = (hu || hv);
            if (hx) evX++; if (hu) evU++; if (hv) evV++;
            if (hx && hu && hv) evAllThree++;
            if (hind) evIndAny++;
            if (hu && hv) evIndBoth++;
            if (hx && !hind) evXOnly++;
            if (hind && !hx) evIndOnly++;
            if (!hx && !hu && !hv) evNone++;
            if (hx && !hind) evXnotInd++;
            if (hind && !hx) evIndNotX++;
        }
    // Compute percentages
    auto pct = [&](int v) -> double { return (total_events > 0) ? (100.0 * static_cast<double>(v) / static_cast<double>(total_events)) : 0.0; };
    double pX = pct(evX), pU = pct(evU), pV = pct(evV);
    double pAllThree = pct(evAllThree), pIndAny = pct(evIndAny), pIndBoth = pct(evIndBoth);
    double pXOnly = pct(evXOnly), pIndOnly = pct(evIndOnly), pNone = pct(evNone);
    double pXnotInd = pct(evXnotInd), pIndNotX = pct(evIndNotX);
        TCanvas *c_mplane = new TCanvas("c_marley_plane", "MARLEY presence per plane", 1100, 700);
        c_mplane->Divide(2,1);
        // Left: bar chart of events with MARLEY per plane
        c_mplane->cd(1);
        gPad->SetGridx(); gPad->SetGridy();
    TH1F *h_m_ev_plane = new TH1F("h_m_ev_plane", "Events with MARLEY per plane;Plane;Events [%]", 3, 0, 3);
        h_m_ev_plane->SetStats(0);
        h_m_ev_plane->GetXaxis()->SetBinLabel(1, "X (collection)");
        h_m_ev_plane->GetXaxis()->SetBinLabel(2, "U (induction)");
        h_m_ev_plane->GetXaxis()->SetBinLabel(3, "V (induction)");
    h_m_ev_plane->SetBinContent(1, pX);
    h_m_ev_plane->SetBinContent(2, pU);
    h_m_ev_plane->SetBinContent(3, pV);
    h_m_ev_plane->SetMinimum(0.0);
    h_m_ev_plane->SetMaximum(100.0);
        h_m_ev_plane->SetLineColor(kAzure+2);
        h_m_ev_plane->SetFillColorAlpha(kAzure+2, 0.25);
        h_m_ev_plane->Draw("HIST");
        // Right: text summary with combinations
        c_mplane->cd(2);
        gPad->SetLeftMargin(0.12); gPad->SetRightMargin(0.12);
        TText t; t.SetTextFont(42); t.SetTextSize(0.035); t.SetTextAlign(13);
        double y = 0.90, dy = 0.06; t.DrawTextNDC(0.12, y, "MARLEY per-plane summary"); y -= dy;
        char buf[256];
    snprintf(buf, sizeof(buf), "Events (total): %d", total_events); t.DrawTextNDC(0.12, y, buf); y -= dy;
    snprintf(buf, sizeof(buf), "X plane: %.1f%%", pX); t.DrawTextNDC(0.12, y, buf); y -= dy;
    snprintf(buf, sizeof(buf), "U plane: %.1f%%", pU); t.DrawTextNDC(0.12, y, buf); y -= dy;
    snprintf(buf, sizeof(buf), "V plane: %.1f%%", pV); t.DrawTextNDC(0.12, y, buf); y -= dy;
        y -= 0.02;
    snprintf(buf, sizeof(buf), "Induction (U or V): %.1f%%", pIndAny); t.DrawTextNDC(0.12, y, buf); y -= dy;
    snprintf(buf, sizeof(buf), "Induction both (U and V): %.1f%%", pIndBoth); t.DrawTextNDC(0.12, y, buf); y -= dy;
    snprintf(buf, sizeof(buf), "X only: %.1f%%", pXOnly); t.DrawTextNDC(0.12, y, buf); y -= dy;
    snprintf(buf, sizeof(buf), "Induction only (no X): %.1f%%", pIndOnly); t.DrawTextNDC(0.12, y, buf); y -= dy;
    snprintf(buf, sizeof(buf), "X and not induction: %.1f%%", pXnotInd); t.DrawTextNDC(0.12, y, buf); y -= dy;
    snprintf(buf, sizeof(buf), "Induction and not X: %.1f%%", pIndNotX); t.DrawTextNDC(0.12, y, buf); y -= dy;
    snprintf(buf, sizeof(buf), "All three planes: %.1f%%", pAllThree); t.DrawTextNDC(0.12, y, buf); y -= dy;
    snprintf(buf, sizeof(buf), "None: %.1f%%", pNone); t.DrawTextNDC(0.12, y, buf);
        c_mplane->SaveAs(pdf_output.c_str());
        delete h_m_ev_plane; delete c_mplane;
        // Console summary for quick inspection
    LogInfo << "MARLEY per-plane diagnostic (events %): X=" << pX << "%, U=" << pU << "%, V=" << pV
        << "; Induction(any)=" << pIndAny << "%, Induction(both)=" << pIndBoth
        << "; Xonly=" << pXOnly << "%, Indonly=" << pIndOnly
        << "; All3=" << pAllThree << "%, None=" << pNone << std::endl;
    }

    // MARLEY-only ADC peak page (zoomed only)
    LogInfo << "Creating MARLEY-only ADC peak plots (zoomed 0-250)..." << std::endl;
    TCanvas *c1_m_zoom = new TCanvas("c1_m_zoom", "ADC Peak by Plane (MARLEY, Zoomed 0-250)", 1000, 800);
    c1_m_zoom->Divide(2,2);
    c1_m_zoom->cd(1); gPad->SetLogy(); h_peak_all_marley_fine->GetXaxis()->SetRangeUser(0, 250); h_peak_all_marley_fine->Draw("HIST");
    { TLegend *leg = new TLegend(0.5, 0.8, 0.7, 0.9); leg->AddEntry(h_peak_all_marley_fine, "All Planes (MARLEY)", "f"); leg->Draw(); }
    c1_m_zoom->cd(2); gPad->SetLogy(); h_peak_X_marley_fine->GetXaxis()->SetRangeUser(0, 250); h_peak_X_marley_fine->SetTitle("ADC Peak Histogram (MARLEY, Zoomed 0-250)"); h_peak_X_marley_fine->Draw("HIST");
    { TLegend *leg = new TLegend(0.5, 0.8, 0.7, 0.9); leg->AddEntry(h_peak_X_marley_fine, "Plane X (MARLEY)", "f"); leg->Draw(); }
    c1_m_zoom->cd(3); gPad->SetLogy(); h_peak_U_marley_fine->GetXaxis()->SetRangeUser(0, 250); h_peak_U_marley_fine->SetTitle("ADC Peak Histogram (MARLEY, Zoomed 0-250)"); h_peak_U_marley_fine->Draw("HIST");
    { TLegend *leg = new TLegend(0.5, 0.8, 0.7, 0.9); leg->AddEntry(h_peak_U_marley_fine, "Plane U (MARLEY)", "f"); leg->Draw(); }
    c1_m_zoom->cd(4); gPad->SetLogy(); h_peak_V_marley_fine->GetXaxis()->SetRangeUser(0, 250); h_peak_V_marley_fine->SetTitle("ADC Peak Histogram (MARLEY, Zoomed 0-250)"); h_peak_V_marley_fine->Draw("HIST");
    { TLegend *leg = new TLegend(0.5, 0.8, 0.7, 0.9); leg->AddEntry(h_peak_V_marley_fine, "Plane V (MARLEY)", "f"); leg->Draw(); }
    c1_m_zoom->SaveAs(pdf_output.c_str());

    // Label counts already built during the TP processing loop above
    LogInfo << "Counting TP per generator label... done." << std::endl;

    // Add per-plane label histograms page
    LogInfo << "Creating per-plane generator label plots..." << std::endl;
    {
        TCanvas *c_plane_labels = new TCanvas("c_plane_labels", "Generator labels per plane", 1200, 900);
        c_plane_labels->Divide(1,3);

        auto make_plane_hist = [&](const char* name, const char* title, const std::map<std::string,long long>& counts){
            size_t nlabels = counts.size();
            TH1F *h = new TH1F(name, title, std::max<size_t>(1, nlabels), 0, std::max<size_t>(1, nlabels));
            // Horizontal bar chart: labels become Y-axis; values along X
            h->SetOption("HBAR");
            // h->SetXTitle("TPs (after ToT cut)");
            // Remove Y-axis title entirely on label plots
            h->SetYTitle("");
            h->SetStats(0);
            int bin = 1;
            for (const auto &kv : counts) { h->SetBinContent(bin, kv.second); h->GetXaxis()->SetBinLabel(bin, kv.first.c_str()); bin++; }
            // styling
            double leftMargin = 0.24; double yLabelSize = 0.070; size_t m = counts.size();
            if (m > 6 && m <= 12) { leftMargin = 0.30; yLabelSize = 0.052; }
            else if (m > 12 && m <= 20) { leftMargin = 0.36; yLabelSize = 0.042; }
            else if (m > 20) { leftMargin = 0.42; yLabelSize = 0.036; }
            gPad->SetLeftMargin(leftMargin); gPad->SetBottomMargin(0.12); gPad->SetRightMargin(0.06);
            // log scale on X for counts axis
            gPad->SetLogx();
            // add grid lines
            gPad->SetGridx();
            gPad->SetGridy();
            // With HBAR, X bin labels are drawn on Y axis
            h->GetYaxis()->SetLabelSize(yLabelSize);
            h->GetYaxis()->SetTitle("");
            h->GetYaxis()->SetTitleSize(0);
            h->Draw("HBAR");
            return h;
        };

        c_plane_labels->cd(1); TH1F* hU = make_plane_hist("h_labels_U", "U plane", label_tp_counts_U_plane);
        c_plane_labels->cd(2); TH1F* hV = make_plane_hist("h_labels_V", "V plane", label_tp_counts_V_plane);
        c_plane_labels->cd(3); TH1F* hX = make_plane_hist("h_labels_X", "X plane", label_tp_counts_X_plane);
        c_plane_labels->SaveAs(pdf_output.c_str());
        // cleanup plane histos and canvas
    delete hX; delete hU; delete hV; delete c_plane_labels;
    }

    // Add per-event label histograms (paginated, 3x3 grid)
    LogInfo << "Creating per-event generator label plots..." << std::endl;
    {
        // gather and sort event IDs
        std::vector<int> event_ids; event_ids.reserve(event_label_counts.size());
        for (auto &kv : event_label_counts) event_ids.push_back(kv.first);
        std::sort(event_ids.begin(), event_ids.end());
        const int per_page = 9; int total = static_cast<int>(event_ids.size());
        for (int start = 0; start < total; start += per_page) {
            int end = std::min(start + per_page, total);
            TCanvas *c_evt = new TCanvas((std::string("c_evt_") + std::to_string(start/per_page)).c_str(), "Generator labels per event", 1500, 1000);
            c_evt->Divide(3,3);
            std::vector<TH1F*> to_delete;
            for (int idx = start; idx < end; ++idx) {
                int pad = (idx - start) + 1;
                c_evt->cd(pad);
                int evt = event_ids[idx];
                const auto &counts = event_label_counts[evt];
                size_t nlabels = counts.size();
                // Title includes neutrino energy if available
                std::string htitle = std::string("Event ") + std::to_string(evt);
                auto itE = event_nu_energy.find(evt);
                if (itE != event_nu_energy.end()) {
                    // ASCII dash and proper TLatex: E_{#nu}
                    char buf[128]; snprintf(buf, sizeof(buf), " - E_{#nu}=%.1f MeV", itE->second);
                    htitle += buf;
                }
                TH1F *h = new TH1F((std::string("h_labels_evt_") + std::to_string(evt)).c_str(), htitle.c_str(), std::max<size_t>(1, nlabels), 0, std::max<size_t>(1, nlabels));
                // Horizontal bars for per-event counts
                h->SetOption("HBAR"); 
                // h->SetXTitle("TPs (after ToT cut)"); 
                h->SetYTitle(""); h->SetStats(0);
                int bin = 1; for (const auto &p : counts) { h->SetBinContent(bin, p.second); h->GetXaxis()->SetBinLabel(bin, p.first.c_str()); bin++; }
                // Remove Y axis (labels/ticks) to declutter per-event grid
                gPad->SetLeftMargin(0.12); gPad->SetBottomMargin(0.12); gPad->SetRightMargin(0.06);
                h->GetYaxis()->SetLabelSize(0);
                h->GetYaxis()->SetTitleSize(0);
                h->GetYaxis()->SetTickLength(0);
                h->GetYaxis()->SetAxisColor(0);
                // log scale on X for counts axis
                gPad->SetLogx();
                // add grid lines
                gPad->SetGridx();
                gPad->SetGridy();
                h->Draw("HBAR");
                to_delete.push_back(h);
            }
            c_evt->SaveAs(pdf_output.c_str());
            for (auto* h : to_delete) delete h; delete c_evt;
        }
    }

    // Scatter plot: MARLEY TPs per event vs neutrino energy
    {
        std::vector<double> xs; xs.reserve(event_label_counts.size());
        std::vector<double> ys; ys.reserve(event_label_counts.size());
        auto toLower = [](std::string s){ std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); }); return s; };
        for (const auto& kv : event_label_counts) {
            int evt = kv.first; const auto& counts = kv.second;
            long long marleyCount = 0;
            for (const auto& p : counts) { if (toLower(p.first).find("marley") != std::string::npos) marleyCount += p.second; }
            auto it = event_nu_energy.find(evt);
            if (it != event_nu_energy.end()) { xs.push_back(it->second); ys.push_back((double)marleyCount); }
        }
        if (!xs.empty()) {
            TCanvas *c_scatter = new TCanvas("c_marley_vs_enu", "MARLEY TPs vs neutrino energy", 1000, 700);
            TGraph *gr = new TGraph((int)xs.size(), xs.data(), ys.data());
            gr->SetTitle("MARLEY TPs per event vs neutrino energy;E_{#nu} [MeV];MARLEY TPs");
            gr->SetMarkerStyle(20); gr->SetMarkerColor(kRed+1); gr->SetLineColor(kRed+1);
            gr->Draw("AP");
            gPad->SetGridx(); gPad->SetGridy();
            c_scatter->SaveAs(pdf_output.c_str());
            delete gr; delete c_scatter;
        }
    }

    // New page(s): Neutrino kinematics table (per event)
    if (!event_nu_info.empty()) {
        std::vector<int> nu_events; nu_events.reserve(event_nu_info.size());
        for (const auto &kv : event_nu_info) nu_events.push_back(kv.first);
        std::sort(nu_events.begin(), nu_events.end());
        const int rows_per_page = 28; // number of rows (events) per page
        int page_index = 0;
        for (size_t start = 0; start < nu_events.size(); start += rows_per_page) {
            size_t end = std::min(start + (size_t)rows_per_page, nu_events.size());
            std::string cname = Form("c_nu_kin_%d", page_index);
            TCanvas *c_nu = new TCanvas(cname.c_str(), "Neutrino kinematics", 1100, 1600);
            c_nu->cd();
            TLatex latex; latex.SetNDC(); latex.SetTextFont(42);
            latex.SetTextSize(0.025);
            latex.DrawLatex(0.05, 0.97, "Neutrino Kinematics per Event");
            latex.SetTextSize(0.018);
            latex.DrawLatex(0.05, 0.94, "Columns: Event | E_{#nu} [MeV] | (x,y,z) | t (if available)");
            double y = 0.90;
            double dy = 0.028;
            for (size_t i = start; i < end; ++i) {
                int evt = nu_events[i];
                const auto &info = event_nu_info[evt];
                char buf[512];
                std::string posStr = info.hasXYZ ? Form("(%.1f, %.1f, %.1f)", info.x, info.y, info.z) : std::string("(n/a)");
                std::string tStr   = info.hasT ? Form(" t=%.1f", info.t) : std::string("");
                snprintf(buf, sizeof(buf), "Evt %6d : E=%.1f MeV  pos=%s%s", evt, info.en, posStr.c_str(), tStr.c_str());
                latex.DrawLatex(0.05, y, buf);
                y -= dy; if (y < 0.06) break; // safety
            }
            if (start == 0) {
                latex.SetTextSize(0.016);
                latex.DrawLatex(0.05, 0.03, "Note: Position/time shown only if corresponding branches existed in 'neutrinos' tree.");
            }
            c_nu->SaveAs(pdf_output.c_str());
            delete c_nu; page_index++;
        }
    }

    // Time offset corrections histogram
    if (!event_time_offsets.empty()) {
        // Collect all unique offset values
        std::set<double> unique_offsets;
        for (const auto &kv : event_time_offsets) {
            unique_offsets.insert(kv.second);
        }
        LogInfo << "Time offset analysis: " << unique_offsets.size() << " unique offset values across " << event_time_offsets.size() << " events" << std::endl;
        
        // Create histogram for time offset distribution
        double min_offset = *unique_offsets.begin();
        double max_offset = *unique_offsets.rbegin();
        
        // Determine binning
        int nbins = 50; // default
        if (unique_offsets.size() <= 10) nbins = std::max(10, (int)unique_offsets.size() + 5);
        else if (unique_offsets.size() <= 50) nbins = (int)unique_offsets.size() + 10;
        
        // Extend range slightly for better display
        double range = std::max(1.0, max_offset - min_offset);
        double bin_lo = min_offset - 0.05 * range;
        double bin_hi = max_offset + 0.05 * range;
        
        TCanvas *c_offset = new TCanvas("c_time_offset", "Time Offset Corrections", 1000, 700);
        TH1F *h_offset = new TH1F("h_time_offset", "Time Offset Corrections per Event;Time Offset [TPC ticks];Events", nbins, bin_lo, bin_hi);
        h_offset->SetLineColor(kBlue+1);
        h_offset->SetFillColorAlpha(kBlue+1, 0.3);
        h_offset->SetStats(1);
        
        for (const auto &kv : event_time_offsets) {
            h_offset->Fill(kv.second);
        }
        
        c_offset->cd();
        h_offset->Draw("HIST");
        // Remove grid as requested
        
        // Add text summary
        TLatex latex; latex.SetNDC(); latex.SetTextFont(42); latex.SetTextSize(0.03);
        latex.DrawLatex(0.15, 0.82, Form("Total events: %d", (int)event_time_offsets.size()));
        latex.DrawLatex(0.15, 0.78, Form("Events with offset > 0: %d", (int)std::count_if(event_time_offsets.begin(), event_time_offsets.end(), [](const auto& p){return p.second > 0;})));
        latex.DrawLatex(0.15, 0.74, Form("Range: [%.1f, %.1f] TPC ticks", min_offset, max_offset));
        if (unique_offsets.size() <= 8) {  // Increased limit to show more values
            latex.DrawLatex(0.15, 0.70, "Unique values (all events):");
            int line = 0;
            for (double val : unique_offsets) {
                if (line < 6) { // increased display limit
                    int count = (int)std::count_if(event_time_offsets.begin(), event_time_offsets.end(), [val](const auto& p){return std::abs(p.second - val) < 0.1;});
                    latex.DrawLatex(0.17, 0.66 - line*0.035, Form("%.0f ticks (%d events)", val, count)); // Use %.0f for cleaner display
                    line++;
                } else break;
            }
        }
        
        c_offset->SaveAs(pdf_output.c_str());
        delete h_offset; delete c_offset;
    }

    // Diagnostics: MARLEY presence per event (case-insensitive), and UNKNOWN counts
    {
        auto toLower = [](std::string s){ std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); }); return s; };
        int total_events = static_cast<int>(event_label_counts.size());
        int events_with_marley = 0;
        int events_without_marley = 0;
        int sample_listed = 0;
        std::vector<int> sample_missing_ids;
        long long unknown_total_in_missing = 0;
        int events_with_truth_marley_but_no_tp = 0;
        for (const auto& kv : event_label_counts) {
            int evt = kv.first; const auto& counts = kv.second;
            bool has_marley = false;
            long long unknown_in_evt = 0;
            for (const auto& p : counts) {
                std::string lab = toLower(p.first);
                if (lab.find("marley") != std::string::npos) has_marley = true;
                if (lab == "unknown") unknown_in_evt += p.second;
            }
            if (has_marley) { events_with_marley++; }
            else {
                events_without_marley++;
                unknown_total_in_missing += unknown_in_evt;
                if (sample_listed < 10) { sample_missing_ids.push_back(evt); sample_listed++; }
                if (!events_with_marley_truth.empty() && events_with_marley_truth.count(evt)) {
                    events_with_truth_marley_but_no_tp++;
                }
            }
        }
        LogInfo << "MARLEY diagnostic: " << events_with_marley << "/" << total_events << " events contain MARLEY TPs after ToT cut." << std::endl;
        if (events_without_marley > 0) {
            LogInfo << "Events without MARLEY TPs: " << events_without_marley << " (showing up to 10):" << std::endl;
            std::ostringstream oss; for (size_t i=0;i<sample_missing_ids.size();++i){ if(i) oss << ", "; oss << sample_missing_ids[i]; }
            LogInfo << "  IDs: [" << oss.str() << "]" << std::endl;
            LogInfo << "  Total UNKNOWN TPs across missing events: " << unknown_total_in_missing << std::endl;
            if (!events_with_marley_truth.empty()) {
                LogInfo << "  Of these, events with MARLEY truth but no MARLEY-labeled TPs: " << events_with_truth_marley_but_no_tp << std::endl;
            }
            LogInfo << "  Note: missing MARLEY can result from ToT cuts or TP↔truth association in backtracking; 'UNKNOWN' suggests unlinked TPs." << std::endl;
        }
    }

    // Create canvas for generator label histogram (final page)
    LogInfo << "Creating generator label plot..." << std::endl;
    TCanvas *c_labels = new TCanvas("c_labels", "TP count by generator label", 1200, 700);
    size_t nlabels = label_tp_counts.size();
    if (nlabels == 0) {
        LogWarning << "No labels found to plot." << std::endl;
    }
    TH1F *h_labels = new TH1F("h_labels", "TP count by generator label", std::max<size_t>(1, nlabels), 0, std::max<size_t>(1, nlabels));
    // Use horizontal bar chart for overall counts
    h_labels->SetOption("HBAR");
    h_labels->SetXTitle("Number of TPs (after ToT cut)");
    h_labels->SetYTitle("");
    h_labels->SetStats(0);
    h_labels->SetLineColor(kMagenta+2);
    h_labels->SetFillColorAlpha(kMagenta+2, 0.25);
    
    // Styling: ensure long labels fit on Y axis
    double leftMargin = 0.28;
    double yLabelSize = 0.065;
    if (nlabels > 6 && nlabels <= 12) { leftMargin = 0.34; yLabelSize = 0.052; }
    else if (nlabels > 12 && nlabels <= 20) { leftMargin = 0.40; yLabelSize = 0.042; }
    else if (nlabels > 20) { leftMargin = 0.46; yLabelSize = 0.036; }
    c_labels->SetLeftMargin(leftMargin);
    c_labels->SetBottomMargin(0.12);
    c_labels->SetRightMargin(0.06);
    // log scale on X for counts axis
    c_labels->SetLogx();
    // add grid lines
    c_labels->SetGridx();
    c_labels->SetGridy();
    h_labels->GetYaxis()->SetLabelSize(yLabelSize);
    
    int bin = 1;
    for (const auto &kv : label_tp_counts) {
        h_labels->SetBinContent(bin, kv.second);
        h_labels->GetXaxis()->SetBinLabel(bin, kv.first.c_str());
        bin++;
    }
    h_labels->Draw("HBAR");

    // Save final page
    std::string pdf_last_page = pdf_output + ")";
    c_labels->SaveAs(pdf_last_page.c_str());
    LogInfo << "Generator label plot saved to PDF (final page)" << std::endl;
    LogInfo << "Complete PDF report saved as: " << pdf_output_abs << std::endl;
    producedFiles.push_back(pdf_output_abs);


    // Comprehensive cleanup
    if (h_peak_all_fine) { delete h_peak_all_fine; h_peak_all_fine = nullptr; }
    if (h_peak_X_fine) { delete h_peak_X_fine; h_peak_X_fine = nullptr; }
    if (h_peak_U_fine) { delete h_peak_U_fine; h_peak_U_fine = nullptr; }
    if (h_peak_V_fine) { delete h_peak_V_fine; h_peak_V_fine = nullptr; }
    if (h_peak_all_marley_fine) { delete h_peak_all_marley_fine; h_peak_all_marley_fine = nullptr; }
    if (h_peak_X_marley_fine) { delete h_peak_X_marley_fine; h_peak_X_marley_fine = nullptr; }
    if (h_peak_U_marley_fine) { delete h_peak_U_marley_fine; h_peak_U_marley_fine = nullptr; }
    if (h_peak_V_marley_fine) { delete h_peak_V_marley_fine; h_peak_V_marley_fine = nullptr; }
    // delete MARLEY canvases
    if (c1_m_zoom) { delete c1_m_zoom; c1_m_zoom = nullptr; }
    // delete ToT/ADCvsToT histograms and canvases
    if (h_tot_all) { delete h_tot_all; h_tot_all = nullptr; }
    if (h_tot_X) { delete h_tot_X; h_tot_X = nullptr; }
    if (h_tot_U) { delete h_tot_U; h_tot_U = nullptr; }
    if (h_tot_V) { delete h_tot_V; h_tot_V = nullptr; }
    if (h_adc_vs_tot_all) { delete h_adc_vs_tot_all; h_adc_vs_tot_all = nullptr; }
    if (h_adc_vs_tot_X) { delete h_adc_vs_tot_X; h_adc_vs_tot_X = nullptr; }
    if (h_adc_vs_tot_U) { delete h_adc_vs_tot_U; h_adc_vs_tot_U = nullptr; }
    if (h_adc_vs_tot_V) { delete h_adc_vs_tot_V; h_adc_vs_tot_V = nullptr; }
    if (h_tot_all_marley) { delete h_tot_all_marley; h_tot_all_marley = nullptr; }
    if (h_tot_X_marley) { delete h_tot_X_marley; h_tot_X_marley = nullptr; }
    if (h_tot_U_marley) { delete h_tot_U_marley; h_tot_U_marley = nullptr; }
    if (h_tot_V_marley) { delete h_tot_V_marley; h_tot_V_marley = nullptr; }
    if (c_tot) { delete c_tot; c_tot = nullptr; }
    if (c_int) { delete c_int; c_int = nullptr; }
    if (c_adc_tot) { delete c_adc_tot; c_adc_tot = nullptr; }
    // delete integral histos
    if (h_int_all) { delete h_int_all; h_int_all = nullptr; }
    if (h_int_X) { delete h_int_X; h_int_X = nullptr; }
    if (h_int_U) { delete h_int_U; h_int_U = nullptr; }
    if (h_int_V) { delete h_int_V; h_int_V = nullptr; }
        // delete label canvas and histogram
        if (h_labels) { delete h_labels; h_labels = nullptr; }
        if (c_title) { delete c_title; c_title = nullptr; }
        if (c1) { delete c1; c1 = nullptr; }
        if (c1_zoom) { delete c1_zoom; c1_zoom = nullptr; }
        if (c_labels) { delete c_labels; c_labels = nullptr; }

        // Close file properly
        if (file) {
            file->Close();
            delete file;
            file = nullptr;
        }

        // Final ROOT cleanup per file
        gROOT->GetListOfCanvases()->Clear();
        gROOT->GetListOfFunctions()->Clear();
    } // end for each input file
    
    // Final ROOT cleanup
    gROOT->GetListOfCanvases()->Clear();
    gROOT->GetListOfFunctions()->Clear();

    // Print final summary of produced files and locations
    if (!producedFiles.empty()) {
        LogInfo << "\nSummary of produced files (" << producedFiles.size() << "):" << std::endl;
        for (const auto& p : producedFiles) {
            LogInfo << " - " << p << std::endl;
        }
        std::set<std::string> outDirs;
        for (const auto& p : producedFiles) {
            auto pos = p.find_last_of("/\\");
            outDirs.insert(pos == std::string::npos ? std::string(".") : p.substr(0, pos));
        }
        LogInfo << "Output location(s):" << std::endl;
        for (const auto& d : outDirs) {
            LogInfo << " - " << d << std::endl;
        }
    } else {
        LogWarning << "No output files were produced." << std::endl;
    }


    LogInfo << "analyze_tps completed successfully!" << std::endl;
    return 0;
}
