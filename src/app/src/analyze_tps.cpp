#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>
#include <nlohmann/json.hpp>

#include "CmdLineParser.h"
#include "Logger.h"
#include "GenericToolbox.Utils.h"

// ROOT includes
#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TF1.h>
#include <TStyle.h>
#include <TROOT.h>
#include <TText.h>
#include <TDatime.h>
#include <chrono>

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
    
    // Read the configuration file
    std::ifstream i(json);
    LogThrowIf(not i.is_open(), "Could not open JSON file: " << json);
    
    nlohmann::json j;
    i >> j;
    
    std::string filename = j["filename"];
    LogInfo << "Input ROOT file: " << filename << std::endl;
    
    // Determine output folder: command line option takes precedence, otherwise use input file folder
    std::string outfolder;
    if (clp.isOptionTriggered("outFolder")) {
        outfolder = clp.getOptionVal<std::string>("outFolder");
        LogInfo << "Output folder (from command line): " << outfolder << std::endl;
    } else if (j.contains("output_folder") && !j["output_folder"].get<std::string>().empty()) {
        outfolder = j["output_folder"];
        LogInfo << "Output folder (from JSON): " << outfolder << std::endl;
    } else {
        // Extract folder from input file path
        outfolder = filename.substr(0, filename.find_last_of("/\\"));
        if (outfolder.empty()) outfolder = "."; // current directory if no path
        LogInfo << "Output folder (from input file path): " << outfolder << std::endl;
    }

    // Read optional parameters from JSON with defaults
    int tot_cut = j.value("tot_cut", 0);
    LogInfo << "ToT cut: " << tot_cut << std::endl;

    // Open the ROOT file
    TFile *file = TFile::Open(filename.c_str());
    if (!file || file->IsZombie()) {
        LogError << "Error opening file: " << filename << std::endl;
        return 1;
    }

    // Open the trees for the three planes
    TTree *tree_X = (TTree*)file->Get("clusters/clusters_tree_X");
    TTree *tree_U = (TTree*)file->Get("clusters/clusters_tree_U");
    TTree *tree_V = (TTree*)file->Get("clusters/clusters_tree_V");
    
    if (!tree_X || !tree_U || !tree_V) {
        LogError << "One or more trees not found in file: " << filename << std::endl;
        file->Close();
        return 1;
    }

    // Get the number of entries in each tree
    int nentries_X = tree_X->GetEntries();
    int nentries_U = tree_U->GetEntries();
    int nentries_V = tree_V->GetEntries();
    
    LogInfo << "Number of entries - X: " << nentries_X << ", U: " << nentries_U << ", V: " << nentries_V << std::endl;

    // Set up branch addresses for X plane
    std::vector<int>* tp_adc_peak = nullptr;
    std::vector<int>* tp_samples_over_threshold = nullptr;
    int n_tps;
    double total_charge;
    float true_particle_energy;
    std::string* true_label_X = nullptr;
    int event_X = 0;

    tree_X->SetBranchAddress("tp_adc_peak", &tp_adc_peak);
    tree_X->SetBranchAddress("tp_samples_over_threshold", &tp_samples_over_threshold);
    tree_X->SetBranchAddress("n_tps", &n_tps);
    tree_X->SetBranchAddress("total_charge", &total_charge);
    tree_X->SetBranchAddress("true_particle_energy", &true_particle_energy);
    tree_X->SetBranchAddress("true_label", &true_label_X);
    tree_X->SetBranchAddress("event", &event_X);

    // Set up branch addresses for U plane
    std::vector<int>* tp_adc_peak_U = nullptr;
    std::vector<int>* tp_samples_over_threshold_U = nullptr;
    int n_tps_U;
    double total_charge_U;
    float true_particle_energy_U;
    std::string* true_label_U = nullptr;
    int event_U = 0;
    
    tree_U->SetBranchAddress("tp_adc_peak", &tp_adc_peak_U);
    tree_U->SetBranchAddress("tp_samples_over_threshold", &tp_samples_over_threshold_U);
    tree_U->SetBranchAddress("n_tps", &n_tps_U);
    tree_U->SetBranchAddress("total_charge", &total_charge_U);
    tree_U->SetBranchAddress("true_particle_energy", &true_particle_energy_U);
    tree_U->SetBranchAddress("true_label", &true_label_U);
    tree_U->SetBranchAddress("event", &event_U);

    // Set up branch addresses for V plane
    std::vector<int>* tp_adc_peak_V = nullptr;
    std::vector<int>* tp_samples_over_threshold_V = nullptr;
    int n_tps_V;
    double total_charge_V;
    float true_particle_energy_V;
    std::string* true_label_V = nullptr;
    int event_V = 0;
    
    tree_V->SetBranchAddress("tp_adc_peak", &tp_adc_peak_V);
    tree_V->SetBranchAddress("tp_samples_over_threshold", &tp_samples_over_threshold_V);
    tree_V->SetBranchAddress("n_tps", &n_tps_V);
    tree_V->SetBranchAddress("total_charge", &total_charge_V);
    tree_V->SetBranchAddress("true_particle_energy", &true_particle_energy_V);
    tree_V->SetBranchAddress("true_label", &true_label_V);
    tree_V->SetBranchAddress("event", &event_V);

    // Create histograms for all planes and each individual plane
    std::string hist_suffix = "_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    TH1F *h_peak_all = new TH1F(("h_peak_all" + hist_suffix).c_str(), "ADC Peak (All Planes)", 100, 54.5, 800.5);
    TH1F *h_peak_X = new TH1F(("h_peak_X" + hist_suffix).c_str(), "ADC Peak (Plane X)", 200, 54.5, 800.5);
    TH1F *h_peak_U = new TH1F(("h_peak_U" + hist_suffix).c_str(), "ADC Peak (Plane U)", 200, 54.5, 800.5);
    TH1F *h_peak_V = new TH1F(("h_peak_V" + hist_suffix).c_str(), "ADC Peak (Plane V)", 200, 54.5, 800.5);


    // Reset histogram statistics (avoid global ROOT resets that can invalidate trees/branches)
    h_peak_all->Reset();
    h_peak_X->Reset();
    h_peak_U->Reset();
    h_peak_V->Reset();

    // Fill X plane
    LogInfo << "Processing X plane..." << std::endl;
    {
        bool warned_null_once = false;
        for (int i = 0; i < nentries_X; i++) {
            Long64_t nb = tree_X->GetEntry(i);
            if (nb <= 0 || tp_adc_peak == nullptr || tp_samples_over_threshold == nullptr) {
                if (!warned_null_once) {
                    LogWarning << "Null branch data or no bytes read for X at entry " << i << "; skipping (will suppress further warnings)." << std::endl;
                    warned_null_once = true;
                }
                continue;
            }
            const size_t n = std::min(tp_adc_peak->size(), tp_samples_over_threshold->size());
            for (size_t j = 0; j < n; j++) {
                if ((*tp_samples_over_threshold)[j] > tot_cut) {
                    h_peak_X->Fill((*tp_adc_peak)[j]);
                    h_peak_all->Fill((*tp_adc_peak)[j]);
                }
            }
        }
    }

    // Fill U plane
    LogInfo << "Processing U plane..." << std::endl;
    {
        bool warned_null_once = false;
        for (int i = 0; i < nentries_U; i++) {
            Long64_t nb = tree_U->GetEntry(i);
            if (nb <= 0 || tp_adc_peak_U == nullptr || tp_samples_over_threshold_U == nullptr) {
                if (!warned_null_once) {
                    LogWarning << "Null branch data or no bytes read for U at entry " << i << "; skipping (will suppress further warnings)." << std::endl;
                    warned_null_once = true;
                }
                continue;
            }
            const size_t n = std::min(tp_adc_peak_U->size(), tp_samples_over_threshold_U->size());
            for (size_t j = 0; j < n; j++) {
                if ((*tp_samples_over_threshold_U)[j] > tot_cut) {
                    h_peak_U->Fill((*tp_adc_peak_U)[j]);
                    h_peak_all->Fill((*tp_adc_peak_U)[j]);
                }
            }
        }
    }

    // Fill V plane
    LogInfo << "Processing V plane..." << std::endl;
    {
        bool warned_null_once = false;
        for (int i = 0; i < nentries_V; i++) {
            Long64_t nb = tree_V->GetEntry(i);
            if (nb <= 0 || tp_adc_peak_V == nullptr || tp_samples_over_threshold_V == nullptr) {
                if (!warned_null_once) {
                    LogWarning << "Null branch data or no bytes read for V at entry " << i << "; skipping (will suppress further warnings)." << std::endl;
                    warned_null_once = true;
                }
                continue;
            }
            const size_t n = std::min(tp_adc_peak_V->size(), tp_samples_over_threshold_V->size());
            for (size_t j = 0; j < n; j++) {
                if ((*tp_samples_over_threshold_V)[j] > tot_cut) {
                    h_peak_V->Fill((*tp_adc_peak_V)[j]);
                    h_peak_all->Fill((*tp_adc_peak_V)[j]);
                }
            }
        }
    }

    // Prepare label counting across all planes, per plane, and per event
    std::map<std::string, long long> label_tp_counts; // all planes
    std::map<std::string, long long> label_tp_counts_X_plane;
    std::map<std::string, long long> label_tp_counts_U_plane;
    std::map<std::string, long long> label_tp_counts_V_plane;
    std::map<int, std::map<std::string, long long>> event_label_counts; // combined across planes per event

    // Set ROOT style
    gStyle->SetOptStat(111111);

    // Create PDF report with multiple pages
    LogInfo << "Creating PDF report..." << std::endl;
    // Extract base filename and create PDF output path
    std::string base_filename = filename.substr(filename.find_last_of("/\\") + 1);
    size_t dot_pos = base_filename.find_last_of(".");
    if (dot_pos != std::string::npos) {
        base_filename = base_filename.substr(0, dot_pos);
    }
    std::string pdf_output = outfolder + "/" + base_filename + ".pdf";
    
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
    TText *file_info = new TText(0.5, 0.5, Form("Input file: %s", filename.c_str()));
    file_info->SetTextAlign(22);
    file_info->SetTextSize(0.025);
    file_info->SetTextFont(42);
    file_info->SetNDC();
    file_info->Draw();
    
    TText *stats_info = new TText(0.5, 0.45, Form("Number of entries - X: %d, U: %d, V: %d", nentries_X, nentries_U, nentries_V));
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

    // Configure histogram styles
    h_peak_all->SetOption("HIST");
    h_peak_all->SetXTitle("ADC Peak");
    h_peak_all->SetYTitle("Entries");
    h_peak_all->SetTitle("ADC Peak Histogram");
    h_peak_all->SetLineColor(kBlack);
    h_peak_all->SetFillColorAlpha(kBlack, 0.15);

    h_peak_X->SetOption("HIST");
    h_peak_X->SetXTitle("ADC Peak");
    h_peak_X->SetYTitle("Entries");
    h_peak_X->SetTitle("ADC Peak Histogram");
    h_peak_X->SetLineColor(kBlue);
    h_peak_X->SetFillColorAlpha(kBlue, 0.2);

    h_peak_U->SetOption("HIST");
    h_peak_U->SetXTitle("ADC Peak");
    h_peak_U->SetYTitle("Entries");
    h_peak_U->SetTitle("ADC Peak Histogram");
    h_peak_U->SetLineColor(kRed);
    h_peak_U->SetFillColorAlpha(kRed, 0.2);

    h_peak_V->SetOption("HIST");
    h_peak_V->SetXTitle("ADC Peak");
    h_peak_V->SetYTitle("Entries");
    h_peak_V->SetTitle("ADC Peak Histogram");
    h_peak_V->SetLineColor(kGreen+2);
    h_peak_V->SetFillColorAlpha(kGreen+2, 0.2);

    // Create canvas for ADC peak histograms (Page 2)
    LogInfo << "Creating ADC peak plots..." << std::endl;
    TCanvas *c1 = new TCanvas("c1", "ADC Peak by Plane", 1000, 800);
    c1->Divide(2,2);

    c1->cd(1);
    gPad->SetLogy();
    h_peak_all->Draw("HIST");
    TLegend *leg_all = new TLegend(0.5, 0.8, 0.7, 0.9);
    leg_all->AddEntry(h_peak_all, "All Planes", "f");
    leg_all->Draw();

    c1->cd(2);
    gPad->SetLogy();
    h_peak_X->Draw("HIST");
    TLegend *leg_X = new TLegend(0.5, 0.8, 0.7, 0.9);
    leg_X->AddEntry(h_peak_X, "Plane X", "f");
    leg_X->Draw();

    c1->cd(3);
    gPad->SetLogy();
    h_peak_U->Draw("HIST");
    TLegend *leg_U = new TLegend(0.5, 0.8, 0.7, 0.9);
    leg_U->AddEntry(h_peak_U, "Plane U", "f");
    leg_U->Draw();

    c1->cd(4);
    gPad->SetLogy();
    h_peak_V->Draw("HIST");
    TLegend *leg_V = new TLegend(0.5, 0.8, 0.7, 0.9);
    leg_V->AddEntry(h_peak_V, "Plane V", "f");
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
    h_peak_all->GetXaxis()->SetRangeUser(0, 250);
    h_peak_all->Draw("HIST");
    TLegend *leg_all_zoom = new TLegend(0.5, 0.8, 0.7, 0.9);
    leg_all_zoom->AddEntry(h_peak_all, "All Planes", "f");
    leg_all_zoom->Draw();

    c1_zoom->cd(2);
    gPad->SetLogy();
    h_peak_X->GetXaxis()->SetRangeUser(0, 250);
    h_peak_X->Draw("HIST");
    TLegend *leg_X_zoom = new TLegend(0.5, 0.8, 0.7, 0.9);
    leg_X_zoom->AddEntry(h_peak_X, "Plane X", "f");
    leg_X_zoom->Draw();

    c1_zoom->cd(3);
    gPad->SetLogy();
    h_peak_U->GetXaxis()->SetRangeUser(0, 250);
    h_peak_U->Draw("HIST");
    TLegend *leg_U_zoom = new TLegend(0.5, 0.8, 0.7, 0.9);
    leg_U_zoom->AddEntry(h_peak_U, "Plane U", "f");
    leg_U_zoom->Draw();

    c1_zoom->cd(4);
    gPad->SetLogy();
    h_peak_V->GetXaxis()->SetRangeUser(0, 250);
    h_peak_V->Draw("HIST");
    TLegend *leg_V_zoom = new TLegend(0.5, 0.8, 0.7, 0.9);
    leg_V_zoom->AddEntry(h_peak_V, "Plane V", "f");
    leg_V_zoom->Draw();

    // Save third page of PDF
    c1_zoom->SaveAs(pdf_output.c_str());
    LogInfo << "ADC peak plots (zoomed 0-250) saved to PDF (page 3)" << std::endl;

    // Build label counts by replaying the loops with labels
    LogInfo << "Counting TP per generator label..." << std::endl;
    // X plane labels
    {
        bool warned_null_once = false;
        for (int i = 0; i < nentries_X; i++) {
            Long64_t nb = tree_X->GetEntry(i);
            if (nb <= 0 || tp_adc_peak == nullptr || tp_samples_over_threshold == nullptr || true_label_X == nullptr) {
                if (!warned_null_once) { LogWarning << "Missing data for X at entry " << i << "; skipping label counting." << std::endl; warned_null_once = true; }
                continue;
            }
            const size_t n = std::min(tp_adc_peak->size(), tp_samples_over_threshold->size());
            for (size_t j = 0; j < n; j++) {
                if ((*tp_samples_over_threshold)[j] > tot_cut) {
                    const std::string &lab = *true_label_X;
                    label_tp_counts[lab]++;
                    label_tp_counts_X_plane[lab]++;
                    event_label_counts[event_X][lab]++;
                }
            }
        }
    }
    // U plane labels
    {
        bool warned_null_once = false;
        for (int i = 0; i < nentries_U; i++) {
            Long64_t nb = tree_U->GetEntry(i);
            if (nb <= 0 || tp_adc_peak_U == nullptr || tp_samples_over_threshold_U == nullptr || true_label_U == nullptr) {
                if (!warned_null_once) { LogWarning << "Missing data for U at entry " << i << "; skipping label counting." << std::endl; warned_null_once = true; }
                continue;
            }
            const size_t n = std::min(tp_adc_peak_U->size(), tp_samples_over_threshold_U->size());
            for (size_t j = 0; j < n; j++) {
                if ((*tp_samples_over_threshold_U)[j] > tot_cut) {
                    const std::string &lab = *true_label_U;
                    label_tp_counts[lab]++;
                    label_tp_counts_U_plane[lab]++;
                    event_label_counts[event_U][lab]++;
                }
            }
        }
    }
    // V plane labels
    {
        bool warned_null_once = false;
        for (int i = 0; i < nentries_V; i++) {
            Long64_t nb = tree_V->GetEntry(i);
            if (nb <= 0 || tp_adc_peak_V == nullptr || tp_samples_over_threshold_V == nullptr || true_label_V == nullptr) {
                if (!warned_null_once) { LogWarning << "Missing data for V at entry " << i << "; skipping label counting." << std::endl; warned_null_once = true; }
                continue;
            }
            const size_t n = std::min(tp_adc_peak_V->size(), tp_samples_over_threshold_V->size());
            for (size_t j = 0; j < n; j++) {
                if ((*tp_samples_over_threshold_V)[j] > tot_cut) {
                    const std::string &lab = *true_label_V;
                    label_tp_counts[lab]++;
                    label_tp_counts_V_plane[lab]++;
                    event_label_counts[event_V][lab]++;
                }
            }
        }
    }

    // Add per-plane label histograms page
    LogInfo << "Creating per-plane generator label plots..." << std::endl;
    {
        TCanvas *c_plane_labels = new TCanvas("c_plane_labels", "Generator labels per plane", 1200, 900);
        c_plane_labels->Divide(3,1);

        auto make_plane_hist = [&](const char* name, const char* title, const std::map<std::string,long long>& counts){
            size_t nlabels = counts.size();
            TH1F *h = new TH1F((std::string(name) + hist_suffix).c_str(), title, std::max<size_t>(1, nlabels), 0, std::max<size_t>(1, nlabels));
            h->SetOption("HIST");
            h->SetXTitle("Generator label");
            h->SetYTitle("TPs (after ToT cut)");
            int bin = 1;
            for (const auto &kv : counts) { h->SetBinContent(bin, kv.second); h->GetXaxis()->SetBinLabel(bin, kv.first.c_str()); bin++; }
            // styling
            double bottomMargin = 0.18; double labelSize = 0.035; size_t m = counts.size();
            if (m > 6 && m <= 12) { bottomMargin = 0.24; labelSize = 0.028; }
            else if (m > 12 && m <= 20) { bottomMargin = 0.30; labelSize = 0.022; }
            else if (m > 20) { bottomMargin = 0.36; labelSize = 0.018; }
            gPad->SetBottomMargin(bottomMargin); gPad->SetLeftMargin(0.12); gPad->SetRightMargin(0.05);
            h->GetXaxis()->SetLabelSize(labelSize); h->GetXaxis()->CenterLabels(true); h->LabelsOption("v","X");
            h->Draw("HIST");
            return h;
        };

        c_plane_labels->cd(1); TH1F* hX = make_plane_hist("h_labels_X", "X plane", label_tp_counts_X_plane);
        c_plane_labels->cd(2); TH1F* hU = make_plane_hist("h_labels_U", "U plane", label_tp_counts_U_plane);
        c_plane_labels->cd(3); TH1F* hV = make_plane_hist("h_labels_V", "V plane", label_tp_counts_V_plane);
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
                TH1F *h = new TH1F((std::string("h_labels_evt_") + std::to_string(evt) + hist_suffix).c_str(), (std::string("Event ") + std::to_string(evt)).c_str(), std::max<size_t>(1, nlabels), 0, std::max<size_t>(1, nlabels));
                h->SetOption("HIST"); h->SetXTitle("Generator label"); h->SetYTitle("TPs (after ToT cut)");
                int bin = 1; for (const auto &p : counts) { h->SetBinContent(bin, p.second); h->GetXaxis()->SetBinLabel(bin, p.first.c_str()); bin++; }
                double bottomMargin = 0.18; double labelSize = 0.03; size_t m = counts.size();
                if (m > 6 && m <= 12) { bottomMargin = 0.24; labelSize = 0.024; }
                else if (m > 12 && m <= 20) { bottomMargin = 0.30; labelSize = 0.018; }
                else if (m > 20) { bottomMargin = 0.36; labelSize = 0.014; }
                gPad->SetBottomMargin(bottomMargin); gPad->SetLeftMargin(0.12); gPad->SetRightMargin(0.05);
                h->GetXaxis()->SetLabelSize(labelSize); h->GetXaxis()->CenterLabels(true); h->LabelsOption("v","X");
                h->Draw("HIST");
                to_delete.push_back(h);
            }
            c_evt->SaveAs(pdf_output.c_str());
            for (auto* h : to_delete) delete h; delete c_evt;
        }
    }

    // Create canvas for generator label histogram (final page)
    LogInfo << "Creating generator label plot..." << std::endl;
    TCanvas *c_labels = new TCanvas("c_labels", "TP count by generator label", 1200, 700);
    size_t nlabels = label_tp_counts.size();
    if (nlabels == 0) {
        LogWarning << "No labels found to plot." << std::endl;
    }
    TH1F *h_labels = new TH1F((std::string("h_labels") + hist_suffix).c_str(), "TP count by generator label", std::max<size_t>(1, nlabels), 0, std::max<size_t>(1, nlabels));
    h_labels->SetOption("HIST");
    h_labels->SetXTitle("Generator label");
    h_labels->SetYTitle("Number of TPs (after ToT cut)");
    h_labels->SetLineColor(kMagenta+2);
    h_labels->SetFillColorAlpha(kMagenta+2, 0.25);
    
    // Styling: center labels and ensure they fit with appropriate margins
    double bottomMargin = 0.18;
    double labelSize = 0.035;
    if (nlabels > 6 && nlabels <= 12) { bottomMargin = 0.24; labelSize = 0.028; }
    else if (nlabels > 12 && nlabels <= 20) { bottomMargin = 0.30; labelSize = 0.022; }
    else if (nlabels > 20) { bottomMargin = 0.36; labelSize = 0.018; }
    c_labels->SetBottomMargin(bottomMargin);
    c_labels->SetLeftMargin(0.12);
    c_labels->SetRightMargin(0.05);
    h_labels->GetXaxis()->SetLabelSize(labelSize);
    h_labels->GetXaxis()->CenterLabels(true);
    h_labels->GetXaxis()->SetLabelOffset(0.01);
    h_labels->GetXaxis()->SetTitleOffset(1.1 + (bottomMargin - 0.18));
    
    int bin = 1;
    for (const auto &kv : label_tp_counts) {
        h_labels->SetBinContent(bin, kv.second);
        h_labels->GetXaxis()->SetBinLabel(bin, kv.first.c_str());
        bin++;
    }
    h_labels->LabelsOption("v", "X");
    h_labels->Draw("HIST");

    // Save final page
    std::string pdf_last_page = pdf_output + ")";
    c_labels->SaveAs(pdf_last_page.c_str());
    LogInfo << "Generator label plot saved to PDF (final page)" << std::endl;
    LogInfo << "Complete PDF report saved as: " << pdf_output << std::endl;


    // Comprehensive cleanup
    if (h_peak_all) { delete h_peak_all; h_peak_all = nullptr; }
    if (h_peak_X) { delete h_peak_X; h_peak_X = nullptr; }
    if (h_peak_U) { delete h_peak_U; h_peak_U = nullptr; }
    if (h_peak_V) { delete h_peak_V; h_peak_V = nullptr; }
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
    
    // Final ROOT cleanup
    gROOT->GetListOfCanvases()->Clear();
    gROOT->GetListOfFunctions()->Clear();


    LogInfo << "analyze_tps completed successfully!" << std::endl;
    return 0;
}
