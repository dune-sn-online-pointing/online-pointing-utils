#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <filesystem>

#include "CmdLineParser.h"
#include "Logger.h"
#include "GenericToolbox.Utils.h"

// ROOT
#include <TFile.h>
#include <TTree.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TROOT.h>
#include <TText.h>

#include "cluster_to_root_libs.h"
#include "ParametersManager.h"
#include "utils.h"

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

static std::string toLower(std::string s){ std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); }); return s; }

int main(int argc, char* argv[]){
  CmdLineParser clp;
  clp.getDescription() << "> analyze_clusters app - Generate plots from Cluster ROOT files." << std::endl;
  clp.addDummyOption("Main options");
  clp.addOption("json",    {"-j","--json"}, "JSON file containing the configuration");
  clp.addOption("inputFile", {"-i", "--input-file"}, "Input file with list OR single ROOT file path (overrides JSON inputs)");
  clp.addOption("outFolder", {"--output-folder"}, "Output folder path (optional)");
  clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");
  clp.addDummyOption();
  LogInfo << clp.getDescription().str() << std::endl;
  LogInfo << "Usage: " << std::endl;
  LogInfo << clp.getConfigSummary() << std::endl << std::endl;
  clp.parseCmdLine(argc, argv);
  LogThrowIf(clp.isNoOptionTriggered(), "No option was provided.");

  // Load parameters
  ParametersManager::getInstance().loadParameters();

  // Avoid ROOT auto-ownership of histograms to prevent double deletes on TFile close
  TH1::AddDirectory(kFALSE);

  std::string json = clp.getOptionVal<std::string>("json");
  std::ifstream jf(json); LogThrowIf(!jf.is_open(), "Could not open JSON: " << json);
  nlohmann::json j; jf >> j;

  auto resolvePath = [](const std::string& p)->std::string{ std::error_code ec; auto abs=std::filesystem::absolute(p, ec); return ec? p : abs.string(); };

  // Use utility function for file finding (clusters files)
  std::vector<std::string> inputs = find_input_files(j, "_clusters");
  
  // Override with CLI input if provided
  if (clp.isOptionTriggered("inputFile")) {
      std::string input_file = clp.getOptionVal<std::string>("inputFile");
      inputs.clear();
      if (input_file.find("_clusters") != std::string::npos || input_file.find(".root") != std::string::npos) {
          inputs.push_back(input_file);
      } else {
          std::ifstream lf(input_file);
          std::string line;
          while (std::getline(lf, line)) {
              if (!line.empty() && line[0] != '#') {
                  inputs.push_back(line);
              }
          }
      }
  }

  LogInfo << "Number of valid files: " << inputs.size() << std::endl;
  LogThrowIf(inputs.empty(), "No valid input files found.");

  std::string outFolder;
  if (clp.isOptionTriggered("outFolder")) outFolder = clp.getOptionVal<std::string>("outFolder");
  else if (j.contains("output_folder")) outFolder = j.value("output_folder", std::string(""));

  std::vector<std::string> produced;

  for (const auto& clusters_file : inputs){
    LogInfo << "Input clusters file: " << clusters_file << std::endl;
    // Read clusters per plane; the file may contain multiple trees (U/V/X)
    TFile* f = TFile::Open(clusters_file.c_str());
    if (!f || f->IsZombie()){ LogError << "Cannot open: " << clusters_file << std::endl; if(f){f->Close(); delete f;} continue; }

    struct PlaneData{ std::string name; TTree* tree=nullptr; };
    std::vector<PlaneData> planes;
  if (auto* dir = f->GetDirectory("clusters")){
      TIter nextKey(dir->GetListOfKeys());
      while (TKey* key = (TKey*)nextKey()){
        if (std::string(key->GetClassName())=="TTree"){
          auto* t = dynamic_cast<TTree*>(key->ReadObj());
          if (!t) continue;
          std::string tname = t->GetName();
          // expected e.g. clusters_tree_X
          std::string plane=""; auto pos=tname.find_last_of('_'); if(pos!=std::string::npos) plane=tname.substr(pos+1);
      planes.push_back(PlaneData{plane, t});
        }
      }
    }
    if (planes.empty()){
      // legacy fallbacks
      for (auto p : {"U","V","X"}){
        auto* t = dynamic_cast<TTree*>(f->Get((std::string("clusters_tree_")+p).c_str()));
        if (t) planes.push_back({p, t});
      }
    }
    LogThrowIf(planes.empty(), "No clusters trees found in file: " << clusters_file);

    // Output PDF path
    std::string base = clusters_file.substr(clusters_file.find_last_of("/\\")+1);
    auto dot = base.find_last_of('.'); if (dot!=std::string::npos) base = base.substr(0, dot);
    std::string outDir = outFolder.empty()? clusters_file.substr(0, clusters_file.find_last_of("/\\")) : outFolder;
    if (outDir.empty()) outDir = ".";
    std::string pdf = outDir + "/" + base + ".pdf";

    // Accumulators across planes
    std::map<std::string, long long> label_counts_all;
    std::map<std::string, std::vector<long long>> label_counts_by_plane; // plane -> counts by label not needed, we keep total per label and per plane n_tps
    std::map<std::string, TH1F*> n_tps_plane_h;
  std::map<std::string, TH1F*> total_charge_plane_h;
  std::map<std::string, TH1F*> total_energy_plane_h;
  std::map<std::string, TH1F*> max_tp_charge_plane_h;
  std::map<std::string, TH1F*> total_length_plane_h;
  std::map<std::string, TH2F*> ntps_vs_total_charge_plane_h;
  std::vector<double> marley_enu; std::vector<double> marley_ncl; // per event MARLEY Cluster count vs Eν
  std::map<int,int> sn_clusters_per_event; // sum across planes
  
  // Marley TP fraction categorization counters (across all planes)
  int only_marley_clusters = 0;     // generator_tp_fraction = 1.0
  int partial_marley_clusters = 0;  // 0 < generator_tp_fraction < 1.0
  int no_marley_clusters = 0;       // generator_tp_fraction = 0.0

    auto ensureHist = [](std::map<std::string, TH1F*>& m, const std::string& key, const char* title){
      if (m.count(key)==0){ m[key] = new TH1F((key+"_h").c_str(), title, 100, 0, 1000); m[key]->SetStats(0); m[key]->SetDirectory(nullptr); }
      return m[key];
    };
    auto ensureHist2D = [](std::map<std::string, TH2F*>& m, const std::string& key, const char* title){
      if (m.count(key)==0){ m[key] = new TH2F((key+"_h2").c_str(), title, 100, 0, 100, 100, 0, 1e6); m[key]->SetDirectory(nullptr); }
      return m[key];
    };

    // Iterate planes
    for (auto& pd : planes){
      // Branches
  int event=0, n_tps=0; float true_ne=0; double total_charge=0, total_energy=0; std::string* true_label_ptr=nullptr;
      float true_dir_x=0, true_dir_y=0, true_dir_z=0; std::string* true_interaction_ptr=nullptr;
  float supernova_tp_fraction=0.f, generator_tp_fraction=0.f;
  // vector branches for derived quantities
  std::vector<int>* v_chan=nullptr; std::vector<int>* v_tstart=nullptr; std::vector<int>* v_sot=nullptr; std::vector<int>* v_adcint=nullptr;
      pd.tree->SetBranchAddress("event", &event);
      pd.tree->SetBranchAddress("n_tps", &n_tps);
      // prefer new names; fallback if needed
      if (pd.tree->GetBranch("true_neutrino_energy")) pd.tree->SetBranchAddress("true_neutrino_energy", &true_ne);
      else if (pd.tree->GetBranch("true_energy")) pd.tree->SetBranchAddress("true_energy", &true_ne);
      if (pd.tree->GetBranch("total_charge")) pd.tree->SetBranchAddress("total_charge", &total_charge);
      if (pd.tree->GetBranch("total_energy")) pd.tree->SetBranchAddress("total_energy", &total_energy);
      if (pd.tree->GetBranch("true_label")) pd.tree->SetBranchAddress("true_label", &true_label_ptr);
      if (pd.tree->GetBranch("true_interaction")) pd.tree->SetBranchAddress("true_interaction", &true_interaction_ptr);
  if (pd.tree->GetBranch("supernova_tp_fraction")) pd.tree->SetBranchAddress("supernova_tp_fraction", &supernova_tp_fraction);
  if (pd.tree->GetBranch("generator_tp_fraction")) pd.tree->SetBranchAddress("generator_tp_fraction", &generator_tp_fraction);
  // vectors
  if (pd.tree->GetBranch("tp_detector_channel")) pd.tree->SetBranchAddress("tp_detector_channel", &v_chan);
  if (pd.tree->GetBranch("tp_time_start")) pd.tree->SetBranchAddress("tp_time_start", &v_tstart);
  if (pd.tree->GetBranch("tp_samples_over_threshold")) pd.tree->SetBranchAddress("tp_samples_over_threshold", &v_sot);
  if (pd.tree->GetBranch("tp_adc_integral")) pd.tree->SetBranchAddress("tp_adc_integral", &v_adcint);

      // Histos per plane
  TH1F* h_ntps = ensureHist(n_tps_plane_h, std::string("n_tps_")+pd.name, (std::string("Cluster size (n_tps) - ")+pd.name).c_str());
  TH1F* h_charge = ensureHist(total_charge_plane_h, std::string("total_charge_")+pd.name, (std::string("Total charge - ")+pd.name).c_str());
  TH1F* h_energy = ensureHist(total_energy_plane_h, std::string("total_energy_")+pd.name, (std::string("Total energy - ")+pd.name).c_str());
  TH1F* h_maxadc = ensureHist(max_tp_charge_plane_h, std::string("max_tp_charge_")+pd.name, (std::string("Max TP adc_integral - ")+pd.name).c_str());
  TH1F* h_tlen = ensureHist(total_length_plane_h, std::string("total_length_")+pd.name, (std::string("Total length (cm) - ")+pd.name).c_str());
  TH2F* h2_ntps_charge = ensureHist2D(ntps_vs_total_charge_plane_h, std::string("ntps_vs_charge_")+pd.name, (std::string("n_{TPs} vs total charge - ")+pd.name+";n_{TPs};total charge").c_str());

      // Per-event MARLEY clustering tracking
  std::map<int,long long> marley_count_evt;
  std::map<int,double> evt_enu; // per-event neutrino energy (if available)

      Long64_t n = pd.tree->GetEntries();
      for (Long64_t i=0;i<n;++i){
        pd.tree->GetEntry(i);
        if (n_tps<=0) continue;
        if (true_label_ptr){ label_counts_all[*true_label_ptr]++; if (toLower(*true_label_ptr).find("marley")!=std::string::npos) marley_count_evt[event]++; }
        h_ntps->Fill(n_tps);
        h_charge->Fill(total_charge);
        h_energy->Fill(total_energy);
        // record event energy when available
        if (true_ne>0) evt_enu[event] = true_ne;
        
        // Categorize clusters by Marley TP content
        if (generator_tp_fraction == 1.0f) {
          only_marley_clusters++;
        } else if (generator_tp_fraction > 0.0f && generator_tp_fraction < 1.0f) {
          partial_marley_clusters++;
        } else if (generator_tp_fraction == 0.0f) {
          no_marley_clusters++;
        }
        
        // derived variables from vectors
        if (v_adcint && !v_adcint->empty()){
          int max_adc = *std::max_element(v_adcint->begin(), v_adcint->end());
          h_maxadc->Fill(max_adc);
        }
        if (v_tstart && v_sot && v_chan && !v_tstart->empty()){
          int tmin = *std::min_element(v_tstart->begin(), v_tstart->end());
          int tmax = tmin;
          for (size_t k=0;k<v_tstart->size();++k){ int tend = v_tstart->at(k) + (v_sot && k<v_sot->size()? v_sot->at(k) : 0); if (tend>tmax) tmax=tend; }
          double time_len_cm = (tmax - tmin) * 0.08; // 0.08 cm/tick
          int cmin = *std::min_element(v_chan->begin(), v_chan->end());
          int cmax = *std::max_element(v_chan->begin(), v_chan->end());
          double chan_len_cm = (cmax - cmin) * 0.5; // 0.5 cm/channel
          double tot_len = std::sqrt(time_len_cm*time_len_cm + chan_len_cm*chan_len_cm);
          h_tlen->Fill(tot_len);
        }
        // 2D
        if (h2_ntps_charge) h2_ntps_charge->Fill(n_tps, total_charge);
        // Supernova Cluster count per event (any fraction > 0)
        if (supernova_tp_fraction > 0.f) sn_clusters_per_event[event] += 1;
      }
      // After looping plane, capture MARLEY vs Eν for this plane
      for (const auto& kv : marley_count_evt){
        double enu = 0.0; auto it = evt_enu.find(kv.first); if (it != evt_enu.end()) enu = it->second;
        marley_enu.push_back(enu);
        marley_ncl.push_back((double)kv.second);
      }
    }

    // Start PDF
    TCanvas* c = new TCanvas("c_ac_title","Title",800,600); c->cd();
    c->SetFillColor(kWhite);
    auto t = new TText(0.5,0.8, "Cluster Analysis Report"); t->SetTextAlign(22); t->SetTextSize(0.06); t->SetNDC(); t->Draw();
    auto ftxt = new TText(0.5,0.6, Form("Input: %s", clusters_file.c_str())); ftxt->SetTextAlign(22); ftxt->SetTextSize(0.03); ftxt->SetNDC(); ftxt->Draw();
    c->SaveAs((pdf+"(").c_str()); delete c;

    // Page: counts by label (HBAR)
    {
      size_t nlabels = label_counts_all.size();
      TCanvas* cl = new TCanvas("c_ac_labels","Labels",1200,700);
      TH1F* h = new TH1F("h_ac_labels","Clusters by true label", std::max<size_t>(1,nlabels), 0, std::max<size_t>(1,nlabels));
      h->SetOption("HBAR"); h->SetStats(0); h->SetYTitle("");
      cl->SetLeftMargin(0.30); cl->SetLogx(); cl->SetGridx(); cl->SetGridy();
      int b=1; for (const auto& kv : label_counts_all){ h->SetBinContent(b, kv.second); h->GetXaxis()->SetBinLabel(b, kv.first.c_str()); b++; }
      h->Draw("HBAR"); cl->SaveAs(pdf.c_str()); delete h; delete cl;
    }

    // Page: per-plane Cluster size and totals
    {
      TCanvas* csz = new TCanvas("c_ac_sizes","Sizes",1200,900); csz->Divide(3,2);
      int pad=1; for (auto* h : {n_tps_plane_h["U"], n_tps_plane_h["V"], n_tps_plane_h["X"], total_charge_plane_h["U"], total_charge_plane_h["V"], total_charge_plane_h["X"]}){
        csz->cd(pad++); if(h){ gPad->SetLogy(); h->Draw("HIST"); }
      }
      csz->SaveAs(pdf.c_str()); delete csz;
      TCanvas* cen = new TCanvas("c_ac_energy","Energy",1200,500); cen->Divide(3,1);
      int p2=1; for (auto* h : {total_energy_plane_h["U"], total_energy_plane_h["V"], total_energy_plane_h["X"]}){ cen->cd(p2++); if(h){ gPad->SetLogy(); h->Draw("HIST"); } }
      cen->SaveAs(pdf.c_str()); delete cen;
    }

    // Page: per-plane max TP charge and total length
    {
      TCanvas* cmax = new TCanvas("c_ac_max","Max and length",1200,900); cmax->Divide(3,2);
      int pad=1; for (auto* h : {max_tp_charge_plane_h["U"], max_tp_charge_plane_h["V"], max_tp_charge_plane_h["X"], total_length_plane_h["U"], total_length_plane_h["V"], total_length_plane_h["X"]}){
        cmax->cd(pad++); if(h){ gPad->SetLogy(); h->Draw("HIST"); }
      }
      cmax->SaveAs(pdf.c_str()); delete cmax;
    }

    // Page: per-plane 2D n_tps vs total charge
    {
      TCanvas* c2d = new TCanvas("c_ac_2d","nTPs vs charge",1200,500); c2d->Divide(3,1);
      int pad=1; for (auto* h2 : {ntps_vs_total_charge_plane_h["U"], ntps_vs_total_charge_plane_h["V"], ntps_vs_total_charge_plane_h["X"]}){
        c2d->cd(pad++); if(h2){ gPad->SetRightMargin(0.12); h2->SetContour(100); h2->Draw("COLZ"); }
      }
      c2d->SaveAs(pdf.c_str()); delete c2d;
    }

    // Page: number of supernova clusters per event
    if (!sn_clusters_per_event.empty()){
      int maxv = 0; for (const auto& kv : sn_clusters_per_event) maxv = std::max(maxv, kv.second);
      TCanvas* csn = new TCanvas("c_ac_sn","SN clusters per event",900,600);
      TH1F* hsn = new TH1F("h_ac_sn","Supernova clusters per event;count;events", maxv+1, -0.5, maxv+0.5);
      for (const auto& kv : sn_clusters_per_event) hsn->Fill(kv.second);
      hsn->Draw("HIST"); csn->SaveAs(pdf.c_str()); delete hsn; delete csn;
    }

    // Page: MARLEY clusters per event vs neutrino energy (scatter)
    if (!marley_enu.empty()){
      TCanvas* csc = new TCanvas("c_ac_scatter","MARLEY clusters vs E#nu",900,600);
      TGraph* gr = new TGraph((int)marley_enu.size(), marley_enu.data(), marley_ncl.data());
      gr->SetTitle("MARLEY clusters vs E_{#nu};E_{#nu} [MeV];N_{clusters} (MARLEY)"); gr->SetMarkerStyle(20); gr->SetMarkerColor(kRed+1);
      gr->Draw("AP"); gPad->SetGridx(); gPad->SetGridy(); csc->SaveAs(pdf.c_str()); delete gr; delete csc;
    }

    // Page: Marley TP fraction categorization
    {
      TCanvas* cmarley = new TCanvas("c_ac_marley","Marley TP categorization",900,600);
      TH1F* hmarley = new TH1F("h_ac_marley","Clusters by Marley TP content;Category;Number of clusters", 3, 0, 3);
      hmarley->SetStats(0);
      hmarley->SetFillColor(kBlue+1);
      hmarley->SetFillStyle(3001);
      
      // Set bin labels and contents
      hmarley->GetXaxis()->SetBinLabel(1, "Only Marley TPs");
      hmarley->GetXaxis()->SetBinLabel(2, "Partial Marley TPs");
      hmarley->GetXaxis()->SetBinLabel(3, "No Marley TPs");
      
      hmarley->SetBinContent(1, only_marley_clusters);
      hmarley->SetBinContent(2, partial_marley_clusters);
      hmarley->SetBinContent(3, no_marley_clusters);
      
      cmarley->SetGridy();
      cmarley->SetBottomMargin(0.15);
      hmarley->Draw("HIST");
      
      // Add text annotations with counts
      auto text1 = new TText(0.8, only_marley_clusters + 0.05 * hmarley->GetMaximum(), Form("%d", only_marley_clusters));
      text1->SetTextAlign(22); text1->SetTextSize(0.04); text1->Draw();
      auto text2 = new TText(1.8, partial_marley_clusters + 0.05 * hmarley->GetMaximum(), Form("%d", partial_marley_clusters));
      text2->SetTextAlign(22); text2->SetTextSize(0.04); text2->Draw();
      auto text3 = new TText(2.8, no_marley_clusters + 0.05 * hmarley->GetMaximum(), Form("%d", no_marley_clusters));
      text3->SetTextAlign(22); text3->SetTextSize(0.04); text3->Draw();
      
      cmarley->SaveAs(pdf.c_str()); 
      delete hmarley; delete cmarley;
    }

    // Close PDF and file
    {
      TCanvas* cend = new TCanvas("c_ac_end","End",10,10); cend->SaveAs((pdf+")").c_str()); delete cend;
    }
    f->Close(); delete f;
    // cleanup hists
  for (auto& kv : n_tps_plane_h) delete kv.second; n_tps_plane_h.clear();
  for (auto& kv : total_charge_plane_h) delete kv.second; total_charge_plane_h.clear();
  for (auto& kv : total_energy_plane_h) delete kv.second; total_energy_plane_h.clear();
  for (auto& kv : max_tp_charge_plane_h) delete kv.second; max_tp_charge_plane_h.clear();
  for (auto& kv : total_length_plane_h) delete kv.second; total_length_plane_h.clear();
  for (auto& kv : ntps_vs_total_charge_plane_h) delete kv.second; ntps_vs_total_charge_plane_h.clear();

    // record produced
    produced.push_back(std::filesystem::absolute(pdf).string());
  }

  if (!produced.empty()){
    LogInfo << "\nSummary of produced files (" << produced.size() << "):" << std::endl;
    for (const auto& p : produced) LogInfo << " - " << p << std::endl;
  }
  return 0;
}
