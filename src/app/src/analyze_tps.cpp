#include <TROOT.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TText.h>
#include <TGraph.h>
#include <TDatime.h>
#include <TFile.h>
#include <TTree.h>
#include <TPad.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TF1.h>
// Includes
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <nlohmann/json.hpp>

// Project includes
#include "CmdLineParser.h"
#include "Logger.h"

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

int main(int argc, char* argv[]) {
    CmdLineParser clp;
    clp.getDescription() << "> analyze_tps app - Generate trigger primitive analysis plots from *_tps_bktr<N>.root files." << std::endl;
    clp.addDummyOption("Main options");
    clp.addOption("json", {"-j", "--json"}, "JSON file containing the configuration");
    clp.addOption("outFolder", {"--output-folder"}, "Output folder path (optional)");
    clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");
    clp.addDummyOption();
    LogInfo << clp.getDescription().str() << std::endl;
    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;
    clp.parseCmdLine(argc, argv);
    LogThrowIf(clp.isNoOptionTriggered(), "No option was provided.");

    // Parse JSON configuration
    std::string json = clp.getOptionVal<std::string>("json");
    std::ifstream jf(json);
    LogThrowIf(!jf.is_open(), "Could not open JSON: " << json);
    nlohmann::json j;
    jf >> j;

    // Determine output folder
    std::string outFolder;
    if (clp.isOptionTriggered("outFolder")) {
        outFolder = clp.getOptionVal<std::string>("outFolder");
    } else if (j.contains("outputFolder")) {
        outFolder = j.value("outputFolder", std::string(""));
    } else if (j.contains("output_folder")) {
        outFolder = j.value("output_folder", std::string(""));
    }

    // Get input files from various JSON fields
    std::vector<std::string> inputs;
    
    // First try "filename" field (single file)
    if (j.contains("filename") && !j["filename"].get<std::string>().empty()) {
        std::string filepath = j["filename"].get<std::string>();
        std::error_code ec;
        auto abs = std::filesystem::absolute(filepath, ec);
        inputs.push_back(ec ? filepath : abs.string());
    }
    
    // Then try "filelist" field (list file)
    if (j.contains("filelist") && !j["filelist"].get<std::string>().empty()) {
        std::ifstream fl(j["filelist"].get<std::string>());
        LogThrowIf(!fl.is_open(), "Could not open file list: " << j["filelist"].get<std::string>());
        std::string line;
        while (std::getline(fl, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::error_code ec;
            auto abs = std::filesystem::absolute(line, ec);
            inputs.push_back(ec ? line : abs.string());
        }
    }
    
    // Try "inputFile" field (single file)
    if (inputs.empty() && j.contains("inputFile") && !j["inputFile"].get<std::string>().empty()) {
        std::string filepath = j["inputFile"].get<std::string>();
        std::error_code ec;
        auto abs = std::filesystem::absolute(filepath, ec);
        inputs.push_back(ec ? filepath : abs.string());
    }
    
    // Try "inputListFile" field (list file)
    if (inputs.empty() && j.contains("inputListFile") && !j["inputListFile"].get<std::string>().empty()) {
        std::ifstream fl(j["inputListFile"].get<std::string>());
        LogThrowIf(!fl.is_open(), "Could not open input list file: " << j["inputListFile"].get<std::string>());
        std::string line;
        while (std::getline(fl, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::error_code ec;
            auto abs = std::filesystem::absolute(line, ec);
            inputs.push_back(ec ? line : abs.string());
        }
    }
    
    // Try "inputFolder" + "inputList" combination
    if (inputs.empty() && j.contains("inputFolder") && !j["inputFolder"].get<std::string>().empty()) {
        std::string folder = j["inputFolder"].get<std::string>();
        
        // If inputList is provided, use those specific files
        if (j.contains("inputList") && j["inputList"].is_array() && !j["inputList"].empty()) {
            for (const auto& item : j["inputList"]) {
                std::string filename = item.get<std::string>();
                std::string filepath = folder + "/" + filename;
                std::error_code ec;
                auto abs = std::filesystem::absolute(filepath, ec);
                inputs.push_back(ec ? filepath : abs.string());
            }
        } else {
            // No specific list provided, scan the folder for .root files
            try {
                for (const auto& entry : std::filesystem::directory_iterator(folder)) {
                    if (entry.is_regular_file()) {
                        std::string filepath = entry.path().string();
                        // Look for files that match the pattern *_tps_bktr*.root or *_tps.root
                        if (filepath.size() >= 5 && filepath.substr(filepath.size()-5) == ".root") {
                            std::string basename = entry.path().filename().string();
                            if (basename.find("_tps_bktr") != std::string::npos || 
                                basename.find("_tps.root") != std::string::npos) {
                                inputs.push_back(filepath);
                            }
                        }
                    }
                }
                // Sort the found files for consistent processing order
                std::sort(inputs.begin(), inputs.end());
            } catch (const std::filesystem::filesystem_error& e) {
                LogWarning << "Error scanning directory " << folder << ": " << e.what() << std::endl;
            }
        }
    }
    
    LogThrowIf(inputs.empty(), "No input files found. Check JSON configuration for inputFile, inputFolder, filelist, or inputListFile fields.");

    // Get ToT cut from configuration
    int tot_cut = j.value("tot_cut", 1); // default to 5
    bool verboseMode = clp.isOptionTriggered("verboseMode");
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

    // Add missing variable declarations
    int tot_hist_max = 100; // adjust as needed

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
    double int_hi = 10000.0; // adjust as needed
    
    TH1F *h_int_all = new TH1F("h_int_all", "ADC Integral (All Planes)", bins_adc_intgl, int_lo, int_hi);
    TH1F *h_int_X   = new TH1F("h_int_X",   "ADC Integral (Plane X)",    bins_adc_intgl, int_lo, int_hi);
    TH1F *h_int_U   = new TH1F("h_int_U",   "ADC Integral (Plane U)",    bins_adc_intgl, int_lo, int_hi);
    TH1F *h_int_V   = new TH1F("h_int_V",   "ADC Integral (Plane V)",    bins_adc_intgl, int_lo, int_hi);
    h_int_all->Reset(); h_int_X->Reset(); h_int_U->Reset(); h_int_V->Reset();

    // ADC integral histograms (MARLEY only, after ToT cut)
    TH1F *h_int_all_marley = new TH1F("h_int_all_marley", "ADC Integral (MARLEY, All Planes)", bins_adc_intgl, int_lo, int_hi);
    TH1F *h_int_X_marley   = new TH1F("h_int_X_marley",   "ADC Integral (MARLEY, Plane X)",    bins_adc_intgl, int_lo, int_hi);
    TH1F *h_int_U_marley   = new TH1F("h_int_U_marley",   "ADC Integral (MARLEY, Plane U)",    bins_adc_intgl, int_lo, int_hi);
    TH1F *h_int_V_marley   = new TH1F("h_int_V_marley",   "ADC Integral (MARLEY, Plane V)",    bins_adc_intgl, int_lo, int_hi);
    h_int_all_marley->Reset(); h_int_X_marley->Reset(); h_int_U_marley->Reset(); h_int_V_marley->Reset();

    // Generate output filename based on first input file
    std::string pdf_output = "tp_analysis_report.pdf";
    if (!inputs.empty() && !outFolder.empty()) {
        // Extract base name from first input file
        std::string first_input = inputs[0];
        auto pos = first_input.find_last_of("/\\");
        std::string basename = (pos == std::string::npos) ? first_input : first_input.substr(pos + 1);
        // Remove file extension and _tps_bktr suffix
        if (basename.size() > 5 && basename.substr(basename.size()-5) == ".root") {
            basename = basename.substr(0, basename.size()-5);
        }
        auto bktr_pos = basename.find("_tps_bktr");
        if (bktr_pos != std::string::npos) {
            basename = basename.substr(0, bktr_pos);
        }
        
        pdf_output = outFolder + "/" + basename + "_tp_analysis_report.pdf";
    } else if (!outFolder.empty()) {
        pdf_output = outFolder + "/tp_analysis_report.pdf";
    }
    
    std::vector<std::string> producedFiles;
    
    // Add data structures for analysis
    std::map<std::string, long long> label_tp_counts;
    std::map<std::string, long long> label_tp_counts_X_plane;
    std::map<std::string, long long> label_tp_counts_U_plane;
    std::map<std::string, long long> label_tp_counts_V_plane;
    std::map<int, std::map<std::string, long long>> event_label_counts;
    std::map<int, std::set<int>> event_union_channels;
    std::set<int> event_has_marley_X;
    std::set<int> event_has_marley_U;
    std::set<int> event_has_marley_V;
    std::set<int> events_with_marley_truth;
    std::map<int, double> event_nu_energy;
    std::map<int, double> event_time_offsets;
    
    struct NeutrinoInfo {
        double en = 0.0;
        double x = 0.0, y = 0.0, z = 0.0;
        double t = 0.0;
        bool hasXYZ = false;
        bool hasT = false;
    };
    std::map<int, NeutrinoInfo> event_nu_info;

    // Process all input files
    LogInfo << "Processing " << inputs.size() << " input file(s)..." << std::endl;
    for (const auto& input_file : inputs) {
        LogInfo << "Opening file: " << input_file << std::endl;
        
        TFile* file = TFile::Open(input_file.c_str());
        if (!file || file->IsZombie()) {
            LogWarning << "Cannot open file: " << input_file << std::endl;
            if (file) { file->Close(); delete file; }
            continue;
        }

        // Find the trigger primitives tree
        TTree* tpTree = nullptr;
        if (auto* dir = file->GetDirectory("tps")) {
            tpTree = dynamic_cast<TTree*>(dir->Get("tps"));
        }
        if (!tpTree) {
            tpTree = dynamic_cast<TTree*>(file->Get("tps_tree"));
        }
        if (!tpTree) {
            tpTree = dynamic_cast<TTree*>(file->Get("tps"));
        }
        
        if (!tpTree) {
            LogWarning << "No trigger primitives tree found in file: " << input_file << std::endl;
            file->Close();
            delete file;
            continue;
        }

        LogInfo << "Found TP tree with " << tpTree->GetEntries() << " entries" << std::endl;

        // Set up branch addresses for reading TPs
        Int_t tp_event = 0;
        UInt_t ch_offline = 0;
        UShort_t adc_peak = 0;
        ULong64_t samples_over_threshold = 0;
        UInt_t adc_integral = 0;
        std::string* generator_name = new std::string();
        Int_t tp_truth_id = 0;
        UShort_t detector = 0;
        std::string* view = new std::string();
        
        tpTree->SetBranchAddress("event", &tp_event);
        tpTree->SetBranchAddress("channel", &ch_offline);
        tpTree->SetBranchAddress("adc_peak", &adc_peak);
        tpTree->SetBranchAddress("samples_over_threshold", &samples_over_threshold);
        tpTree->SetBranchAddress("adc_integral", &adc_integral);
        tpTree->SetBranchAddress("generator_name", &generator_name);
        tpTree->SetBranchAddress("truth_id", &tp_truth_id);
        if (tpTree->GetBranch("detector")) {
            tpTree->SetBranchAddress("detector", &detector);
        }
        if (tpTree->GetBranch("view")) {
            tpTree->SetBranchAddress("view", &view);
        }

        // First pass: read all entries and fill histograms
        Long64_t nentries = tpTree->GetEntries();
        for (Long64_t i = 0; i < nentries; ++i) {
            tpTree->GetEntry(i);
            
            // Apply ToT cut
            if (static_cast<int>(samples_over_threshold) <= tot_cut) continue;
            
            // Determine plane based on view field
            std::string plane = *view; // Use view directly since it contains U, V, X
            
            // Count entries per plane
            if (plane == "X") nentries_X++;
            else if (plane == "U") nentries_U++;
            else if (plane == "V") nentries_V++;
            
            // Fill ADC peak histograms
            h_peak_all_fine->Fill(adc_peak);
            if (plane == "X") h_peak_X_fine->Fill(adc_peak);
            else if (plane == "U") h_peak_U_fine->Fill(adc_peak);
            else if (plane == "V") h_peak_V_fine->Fill(adc_peak);
            
            // Check if this is MARLEY
            std::string gen_lower = *generator_name;
            std::transform(gen_lower.begin(), gen_lower.end(), gen_lower.begin(), 
                          [](unsigned char c){ return std::tolower(c); });
            bool is_marley = (gen_lower.find("marley") != std::string::npos);
            
            if (is_marley) {
                h_peak_all_marley_fine->Fill(adc_peak);
                if (plane == "X") h_peak_X_marley_fine->Fill(adc_peak);
                else if (plane == "U") h_peak_U_marley_fine->Fill(adc_peak);
                else if (plane == "V") h_peak_V_marley_fine->Fill(adc_peak);
                
                events_with_marley_truth.insert(tp_event);
                if (plane == "X") event_has_marley_X.insert(tp_event);
                else if (plane == "U") event_has_marley_U.insert(tp_event);
                else if (plane == "V") event_has_marley_V.insert(tp_event);
            }
            
            // Fill ToT histograms
            h_tot_all->Fill(samples_over_threshold);
            if (plane == "X") h_tot_X->Fill(samples_over_threshold);
            else if (plane == "U") h_tot_U->Fill(samples_over_threshold);
            else if (plane == "V") h_tot_V->Fill(samples_over_threshold);
            
            if (is_marley) {
                h_tot_all_marley->Fill(samples_over_threshold);
                if (plane == "X") h_tot_X_marley->Fill(samples_over_threshold);
                else if (plane == "U") h_tot_U_marley->Fill(samples_over_threshold);
                else if (plane == "V") h_tot_V_marley->Fill(samples_over_threshold);
            }
            
            // Fill ADC vs ToT histograms
            h_adc_vs_tot_all->Fill(samples_over_threshold, adc_peak);
            if (plane == "X") h_adc_vs_tot_X->Fill(samples_over_threshold, adc_peak);
            else if (plane == "U") h_adc_vs_tot_U->Fill(samples_over_threshold, adc_peak);
            else if (plane == "V") h_adc_vs_tot_V->Fill(samples_over_threshold, adc_peak);
            
            // Fill ADC integral histograms
            h_int_all->Fill(adc_integral);
            if (plane == "X") h_int_X->Fill(adc_integral);
            else if (plane == "U") h_int_U->Fill(adc_integral);
            else if (plane == "V") h_int_V->Fill(adc_integral);
            
            if (is_marley) {
                h_int_all_marley->Fill(adc_integral);
                if (plane == "X") h_int_X_marley->Fill(adc_integral);
                else if (plane == "U") h_int_U_marley->Fill(adc_integral);
                else if (plane == "V") h_int_V_marley->Fill(adc_integral);
            }
            
            // Update label counts for analysis
            label_tp_counts[*generator_name]++;
            if (plane == "X") label_tp_counts_X_plane[*generator_name]++;
            else if (plane == "U") label_tp_counts_U_plane[*generator_name]++;
            else if (plane == "V") label_tp_counts_V_plane[*generator_name]++;
            
            // Track events and channels for union computation
            event_label_counts[tp_event][*generator_name]++;
            if (is_marley) {
                event_union_channels[tp_event].insert(ch_offline);
            }
        }
        
        // Clean up string pointers and file
        delete generator_name;
        delete view;
        file->Close();
        delete file;
    }

    LogInfo << "Finished processing all input files." << std::endl;
    LogInfo << "TP counts after ToT cut - X: " << nentries_X << ", U: " << nentries_U << ", V: " << nentries_V << std::endl;
    
    // Title page setup
    TCanvas *c_title = new TCanvas("c_title", "TP Analysis Report", 800, 600);
    c_title->cd();
    
    // Title text
    TText *title_text = new TText(0.5, 0.7, "Trigger Primitive Analysis Report");
    title_text->SetTextAlign(22);
    title_text->SetTextSize(0.05);
    title_text->SetTextFont(62);
    title_text->SetNDC();
    title_text->Draw();
    
    // Summary info text
    TText *tot_info = new TText(0.5, 0.5, "Summary of TP analysis results");
    
    // ... (continue moving all code into main) ...

    // (The rest of the code from the file should be inside this main function)

    // ... (move all code that was after this closing brace into main) ...

    // (Paste all code from after the original main's closing brace here)
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
    if (verboseMode) LogInfo << "Title page saved to PDF" << std::endl;

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
    if (verboseMode) LogInfo << "Creating ADC peak plots..." << std::endl;
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
    if (verboseMode) LogInfo << "ADC peak plots saved to PDF (page 2)" << std::endl;

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

    // ADC integral distributions (Page 6): All, X, U, V with MARLEY overlays
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

    // Style MARLEY overlays as line-only for clarity on top of filled histograms
    auto style_int_m_overlay = [](TH1F* h){
        if (!h) return; h->SetLineColor(kMagenta+2); h->SetLineWidth(2); h->SetFillStyle(0); h->SetTitle((std::string(h->GetTitle()) + " (overlay)").c_str()); };
    style_int_m_overlay(h_int_all_marley);
    style_int_m_overlay(h_int_X_marley);
    style_int_m_overlay(h_int_U_marley);
    style_int_m_overlay(h_int_V_marley);

    TCanvas *c_int = new TCanvas("c_int", "ADC integral distributions (with MARLEY overlays)", 1000, 800);
    c_int->Divide(2,2);
    c_int->cd(1); gPad->SetLogy(); h_int_all->Draw("HIST"); 
    if (h_int_all_marley && h_int_all_marley->GetEntries() > 0) h_int_all_marley->Draw("HIST SAME");
    { TLegend *leg=new TLegend(0.6,0.78,0.88,0.92); leg->SetBorderSize(0); leg->AddEntry(h_int_all, "All Planes (All)", "f"); if (h_int_all_marley && h_int_all_marley->GetEntries()>0) leg->AddEntry(h_int_all_marley, "All Planes (MARLEY)", "l"); leg->Draw(); }
    c_int->cd(2); gPad->SetLogy(); h_int_X->Draw("HIST");   
    if (h_int_X_marley && h_int_X_marley->GetEntries() > 0) h_int_X_marley->Draw("HIST SAME");
    { TLegend *leg=new TLegend(0.6,0.78,0.88,0.92); leg->SetBorderSize(0); leg->AddEntry(h_int_X, "Plane X (All)", "f"); if (h_int_X_marley && h_int_X_marley->GetEntries()>0) leg->AddEntry(h_int_X_marley, "Plane X (MARLEY)", "l"); leg->Draw(); }
    c_int->cd(3); gPad->SetLogy(); h_int_U->Draw("HIST");   
    if (h_int_U_marley && h_int_U_marley->GetEntries() > 0) h_int_U_marley->Draw("HIST SAME");
    { TLegend *leg=new TLegend(0.6,0.78,0.88,0.92); leg->SetBorderSize(0); leg->AddEntry(h_int_U, "Plane U (All)", "f"); if (h_int_U_marley && h_int_U_marley->GetEntries()>0) leg->AddEntry(h_int_U_marley, "Plane U (MARLEY)", "l"); leg->Draw(); }
    c_int->cd(4); gPad->SetLogy(); h_int_V->Draw("HIST");   
    if (h_int_V_marley && h_int_V_marley->GetEntries() > 0) h_int_V_marley->Draw("HIST SAME");
    { TLegend *leg=new TLegend(0.6,0.78,0.88,0.92); leg->SetBorderSize(0); leg->AddEntry(h_int_V, "Plane V (All)", "f"); if (h_int_V_marley && h_int_V_marley->GetEntries()>0) leg->AddEntry(h_int_V_marley, "Plane V (MARLEY)", "l"); leg->Draw(); }
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

    // Backtracking diagnostics: For now, just show the thresholds
    // TODO: Implement tail composition analysis if needed
    TCanvas *c_bktr = new TCanvas("c_bktr", "Backtracking diagnostics: tail composition", 1100, 700);
    c_bktr->Divide(2,1);
    c_bktr->cd(1);
    {
        gPad->SetMargin(0.12,0.06,0.12,0.06);
        TText t; t.SetTextFont(42); t.SetTextSize(0.032); double y=0.94, dy=0.055;
        auto line=[&](const std::string& s){ t.DrawTextNDC(0.12, y, s.c_str()); y-=dy; };
        char b[256]; 
        snprintf(b,sizeof(b),"ADCpeak tails: th95=%.1f, th99=%.1f", adc95_th, adc99_th); line(b);
        line("  Detailed composition analysis would be computed here");
        y-=0.02; 
        snprintf(b,sizeof(b),"ToT tails: th95=%.0f, th99=%.0f", tot95_th, tot99_th); line(b);
        line("  Detailed composition analysis would be computed here");
        y-=0.02; 
        snprintf(b,sizeof(b),"Integral tails: th95=%.0f, th99=%.0f", int95_th, int99_th); line(b);
        line("  Detailed composition analysis would be computed here");
    }
    c_bktr->cd(2);
    {
        // Simple placeholder bar chart
        TH1F *hbar = new TH1F("h_placeholder","Backtracking analysis placeholder;Category;Count",4,0,4);
        hbar->SetStats(0);
        hbar->GetXaxis()->SetBinLabel(1,"MARLEY"); 
        hbar->GetXaxis()->SetBinLabel(2,"KNOWN!=MARLEY"); 
        hbar->GetXaxis()->SetBinLabel(3,"UNK ch-in-union"); 
        hbar->GetXaxis()->SetBinLabel(4,"UNK ch-out");
        hbar->SetBinContent(1, 100); hbar->SetBinContent(2, 50); 
        hbar->SetBinContent(3, 25); hbar->SetBinContent(4, 10);
        hbar->SetFillColorAlpha(kMagenta+2,0.35); hbar->SetLineColor(kMagenta+2);
        gPad->SetGridx(); gPad->SetGridy(); gPad->SetLogy();
        hbar->Draw("HIST");
    }
    c_bktr->SaveAs(pdf_output.c_str());
    delete c_bktr;

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
    
    // Clone histograms to avoid interference with zoom plots
    TH1F *h_peak_all_marley_clone = (TH1F*)h_peak_all_marley_fine->Clone("h_peak_all_marley_clone");
    TH1F *h_peak_X_marley_clone = (TH1F*)h_peak_X_marley_fine->Clone("h_peak_X_marley_clone");
    TH1F *h_peak_U_marley_clone = (TH1F*)h_peak_U_marley_fine->Clone("h_peak_U_marley_clone");
    TH1F *h_peak_V_marley_clone = (TH1F*)h_peak_V_marley_fine->Clone("h_peak_V_marley_clone");
    
    c1_m_zoom->cd(1); gPad->SetLogy(); h_peak_all_marley_clone->GetXaxis()->SetRangeUser(0, 250); h_peak_all_marley_clone->Draw("HIST");
    { TLegend *leg = new TLegend(0.5, 0.8, 0.7, 0.9); leg->AddEntry(h_peak_all_marley_clone, "All Planes (MARLEY)", "f"); leg->Draw(); }
    c1_m_zoom->cd(2); gPad->SetLogy(); h_peak_X_marley_clone->GetXaxis()->SetRangeUser(0, 250); h_peak_X_marley_clone->SetTitle("ADC Peak Histogram (MARLEY, Zoomed 0-250)"); h_peak_X_marley_clone->Draw("HIST");
    { TLegend *leg = new TLegend(0.5, 0.8, 0.7, 0.9); leg->AddEntry(h_peak_X_marley_clone, "Plane X (MARLEY)", "f"); leg->Draw(); }
    c1_m_zoom->cd(3); gPad->SetLogy(); h_peak_U_marley_clone->GetXaxis()->SetRangeUser(0, 250); h_peak_U_marley_clone->SetTitle("ADC Peak Histogram (MARLEY, Zoomed 0-250)"); h_peak_U_marley_clone->Draw("HIST");
    { TLegend *leg = new TLegend(0.5, 0.8, 0.7, 0.9); leg->AddEntry(h_peak_U_marley_clone, "Plane U (MARLEY)", "f"); leg->Draw(); }
    c1_m_zoom->cd(4); gPad->SetLogy(); h_peak_V_marley_clone->GetXaxis()->SetRangeUser(0, 250); h_peak_V_marley_clone->SetTitle("ADC Peak Histogram (MARLEY, Zoomed 0-250)"); h_peak_V_marley_clone->Draw("HIST");
    { TLegend *leg = new TLegend(0.5, 0.8, 0.7, 0.9); leg->AddEntry(h_peak_V_marley_clone, "Plane V (MARLEY)", "f"); leg->Draw(); }
    c1_m_zoom->SaveAs(pdf_output.c_str());
    
    // Cleanup cloned histograms
    delete h_peak_all_marley_clone;
    delete h_peak_X_marley_clone;
    delete h_peak_U_marley_clone;
    delete h_peak_V_marley_clone;

    // Add per-plane label histograms page
    if (verboseMode) LogInfo << "Creating per-plane generator label plots..." << std::endl;
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
    // if (verboseMode) LogInfo << "Creating per-event generator label plots..." << std::endl;
    // {
    //     // gather and sort event IDs
    //     std::vector<int> event_ids; event_ids.reserve(event_label_counts.size());
    //     for (auto &kv : event_label_counts) event_ids.push_back(kv.first);
    //     std::sort(event_ids.begin(), event_ids.end());
    //     const int per_page = 9; int total = static_cast<int>(event_ids.size());
    //     for (int start = 0; start < total; start += per_page) {
    //         int end = std::min(start + per_page, total);
    //         TCanvas *c_evt = new TCanvas((std::string("c_evt_") + std::to_string(start/per_page)).c_str(), "Generator labels per event", 1500, 1000);
    //         c_evt->Divide(3,3);
    //         std::vector<TH1F*> to_delete;
    //         for (int idx = start; idx < end; ++idx) {
    //             int pad = (idx - start) + 1;
    //             c_evt->cd(pad);
    //             int evt = event_ids[idx];
    //             const auto &counts = event_label_counts[evt];
    //             size_t nlabels = counts.size();
    //             // Title includes neutrino energy if available
    //             std::string htitle = std::string("Event ") + std::to_string(evt);
    //             auto itE = event_nu_energy.find(evt);
    //             if (itE != event_nu_energy.end()) {
    //                 // ASCII dash and proper TLatex: E_{#nu}
    //                 char buf[128]; snprintf(buf, sizeof(buf), " - E_{#nu}=%.1f MeV", itE->second);
    //                 htitle += buf;
    //             }
    //             TH1F *h = new TH1F((std::string("h_labels_evt_") + std::to_string(evt)).c_str(), htitle.c_str(), std::max<size_t>(1, nlabels), 0, std::max<size_t>(1, nlabels));
    //             // Horizontal bars for per-event counts
    //             h->SetOption("HBAR"); 
    //             // h->SetXTitle("TPs (after ToT cut)"); 
    //             h->SetYTitle(""); h->SetStats(0);
    //             int bin = 1; for (const auto &p : counts) { h->SetBinContent(bin, p.second); h->GetXaxis()->SetBinLabel(bin, p.first.c_str()); bin++; }
    //             // Remove Y axis (labels/ticks) to declutter per-event grid
    //             gPad->SetLeftMargin(0.12); gPad->SetBottomMargin(0.12); gPad->SetRightMargin(0.06);
    //             h->GetYaxis()->SetLabelSize(0);
    //             h->GetYaxis()->SetTitleSize(0);
    //             h->GetYaxis()->SetTickLength(0);
    //             h->GetYaxis()->SetAxisColor(0);
    //             // log scale on X for counts axis
    //             gPad->SetLogx();
    //             // add grid lines
    //             gPad->SetGridx();
    //             gPad->SetGridy();
    //             h->Draw("HBAR");
    //             to_delete.push_back(h);
    //         }
    //         c_evt->SaveAs(pdf_output.c_str());
    //         for (auto* h : to_delete) delete h; delete c_evt;
    //     }
    // }

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
        if (verboseMode) LogInfo << "Time offset analysis: " << unique_offsets.size() << " unique offset values across " << event_time_offsets.size() << " events" << std::endl;
        
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
    LogInfo << "Complete PDF report saved as: " << pdf_output << std::endl;
    producedFiles.push_back(pdf_output);


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

        // Final ROOT cleanup
        gROOT->GetListOfCanvases()->Clear();
        gROOT->GetListOfFunctions()->Clear();
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
