#include <algorithm>
#include <chrono>
#include <cctype>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <climits>

#include <nlohmann/json.hpp>

#include "CmdLineParser.h"
#include "Logger.h"
#include "GenericToolbox.Utils.h"

#include <TApplication.h>
#include <TCanvas.h>
#include <TButton.h>
#include <TPad.h>
#include <TFile.h>
#include <TTree.h>
#include <TText.h>
#include <TLine.h>
#include <TBox.h>
#include <TLatex.h>
#include <TH2F.h>
#include <TStyle.h>

#include "cluster_to_root_libs.h"
#include "cluster.h"
#include "TriggerPrimitive.hpp"

LoggerInit([]{ Logger::getUserHeader() << "[" << FILENAME << "]";});

// Global viewer state (simple for ROOT UI callbacks)
namespace ViewerState {
  struct Item {
    std::string plane;
    std::vector<int> ch;
    std::vector<int> tstart;
    std::vector<int> sot;
    std::vector<int> stopeak;  // samples_to_peak for triangle shape
    std::vector<int> adc_peak; // peak ADC values
    std::string label;
    std::string interaction;
    float enu = 0.0f;
    double total_charge = 0.0;
    double total_energy = 0.0;
  // Event-mode metadata
  bool isEvent = false;
  int eventId = -1;
  int nClusters = 0;
  bool marleyOnly = false;
  };
  std::vector<Item> items; // MARLEY clusters across planes
  int idx = 0;
  TCanvas* canvas = nullptr;
  TH2F* frame = nullptr;
  TButton* btnPrev = nullptr;
  TButton* btnNext = nullptr;
  TLatex* latex = nullptr;
  TPad* padCtrl = nullptr; // left control pad for buttons
  TPad* padGrid = nullptr; // right plotting area
}

static std::string toLower(std::string s){ std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c);}); return s; }
static inline int toTPCticks(int tdcTicks){ return tdcTicks/32; }

// Internal callbacks (we'll hook buttons via function pointers to avoid Cling symbol lookup)
static void PrevImpl();
static void NextImpl();

void drawCurrent();

static void PrevImpl(){
  using namespace ViewerState;
  if (items.empty()) return;
  idx = std::max(0, idx - 1);
  drawCurrent();
}
static void NextImpl(){
  using namespace ViewerState;
  if (items.empty()) return;
  idx = std::min((int)items.size()-1, idx + 1);
  drawCurrent();
}

void drawCurrent(){
  using namespace ViewerState;
  if (!canvas || items.empty()) return;
  canvas->cd();

  // Current cluster
  const auto& it = items.at(std::clamp(idx, 0, (int)items.size()-1));
  size_t nTPs = std::min({it.ch.size(), it.tstart.size(), it.sot.size()});
  if (nTPs == 0) return;

  // Determine axis ranges (ticks and channels) - map channels to contiguous integers
  int tmin = INT_MAX, tmax = INT_MIN;
  std::set<int> unique_channels;
  for (size_t i=0;i<nTPs;++i){
    int ts = it.tstart[i];
    int te = it.tstart[i] + it.sot[i];
    unique_channels.insert(it.ch[i]);
    if (ts < tmin) tmin = ts; if (te > tmax) tmax = te;
  }
  
  // Create channel mapping: actual channel -> contiguous index
  std::map<int, int> ch_to_idx;
  int cidx = 0;
  for (int ch : unique_channels) {
    ch_to_idx[ch] = cidx++;
  }
  
  int cmin = 0, cmax = cidx - 1;
  if (tmin>tmax) { tmin=0; tmax=1; }
  if (cmin>cmax) { cmin=0; cmax=1; }

  // Add 2 bins of padding in all directions
  const int pad_bins = 2;
  const int nbinsX = cmax - cmin + 1 + 2*pad_bins;
  const int nbinsY = (tmax - tmin) + 1 + 2*pad_bins;
  const double xmin = cmin - pad_bins - 0.5;
  const double xmax = cmax + pad_bins + 0.5;
  const double ymin = tmin - pad_bins - 0.5;
  const double ymax = tmax + pad_bins + 0.5;

  // Lazily create control/plot pads once
  if (ViewerState::padCtrl == nullptr || ViewerState::padGrid == nullptr){
    // Left control pad
    ViewerState::padCtrl = new TPad("pCtrl", "controls", 0.0, 0.0, 0.18, 1.0);
    ViewerState::padCtrl->SetMargin(0.06,0.04,0.04,0.04);
    ViewerState::padCtrl->SetFillColor(kWhite);
    ViewerState::padCtrl->Draw();
    // Right plot area
    ViewerState::padGrid = new TPad("pGrid", "plots", 0.18, 0.0, 1.0, 1.0);
    ViewerState::padGrid->SetMargin(0.12,0.06,0.12,0.08);
    ViewerState::padGrid->Draw();
    // Buttons in the control pad using function-pointer commands
    ViewerState::padCtrl->cd();
    std::string prevCmd = Form("((void(*)())%p)()", (void*)(&PrevImpl));
    btnPrev = new TButton("Prev", prevCmd.c_str(), 0.10, 0.08, 0.90, 0.46);
    btnPrev->SetFillColor(kGray);
    btnPrev->SetTextSize(0.25);
    btnPrev->Draw();
    std::string nextCmd = Form("((void(*)())%p)()", (void*)(&NextImpl));
    btnNext = new TButton("Next", nextCmd.c_str(), 0.10, 0.54, 0.90, 0.92);
    btnNext->SetFillColor(kGray);
    btnNext->SetTextSize(0.25);
    btnNext->Draw();
  }

  // Draw into the plot pad only
  ViewerState::padGrid->cd();
  gPad->Clear();
  if (frame) { delete frame; frame = nullptr; }

  static int heatmapCounter = 0;
  frame = new TH2F(Form("cluster_heatmap_%d", heatmapCounter++), "", nbinsX, xmin, xmax, nbinsY, ymin, ymax);
  frame->SetDirectory(nullptr);
  frame->SetStats(false);
  frame->SetTitle(";channel (contiguous);time [ticks]");
  frame->GetXaxis()->CenterTitle();
  frame->GetYaxis()->CenterTitle();
  frame->GetZaxis()->SetTitle("ADC (triangle model)");
  frame->GetZaxis()->CenterTitle();

  gPad->SetRightMargin(0.18);
  gPad->SetLeftMargin(0.12);
  gPad->SetBottomMargin(0.12);
  gPad->SetGridx();
  gPad->SetGridy();

  const double threshold_adc = 60.0;

  for (size_t i=0;i<nTPs;++i){
    int ts = it.tstart[i];
    int tot = it.sot[i];
    int te = ts + std::max(1, tot);
    int ch_actual = it.ch[i];
    int ch_contiguous = ch_to_idx[ch_actual];

    int samples_to_peak = (i < it.stopeak.size()) ? it.stopeak[i] : (tot > 0 ? tot / 2 : 0);
    int peak_adc = (i < it.adc_peak.size()) ? it.adc_peak[i] : 200;
    int peak_time = ts + samples_to_peak;

    for (int t = ts; t < te; ++t) {
      double intensity = peak_adc;
      if (t <= peak_time) {
        if (peak_time != ts) {
          double frac = double(t - ts) / double(peak_time - ts);
          intensity = threshold_adc + frac * (peak_adc - threshold_adc);
        }
      }
      else {
        int fall_span = (te - 1) - peak_time;
        if (fall_span > 0) {
          double frac = double(t - peak_time) / double(fall_span);
          intensity = peak_adc - frac * (peak_adc - threshold_adc);
        }
      }

      // No ceiling on ADC
      int binx = frame->GetXaxis()->FindBin(ch_contiguous);
      int biny = frame->GetYaxis()->FindBin(t);
      double current = frame->GetBinContent(binx, biny);
      if (intensity > current) {
        frame->SetBinContent(binx, biny, intensity);
      }
    }
  }

  frame->SetMinimum(threshold_adc);
  gStyle->SetPalette(kBird);
  frame->Draw("COLZ");

  // Compose title string for the plot (canvas title, not window)
  std::ostringstream title;
  if (it.isEvent){
    title << "Event " << it.eventId << " | plane " << it.plane << " | nTPs=" << nTPs << " | nClusters=" << it.nClusters;
    title << " | mode=events, filter=" << (it.marleyOnly ? "MARLEY-only" : "All clusters");
  } else {
    title << "Cluster " << (idx+1) << "/" << items.size() << " | plane " << it.plane;
    if (it.eventId >= 0) title << " | event " << it.eventId;
    title << " | nTPs=" << nTPs;
    title << " | E_nu=" << it.enu << " MeV, total_charge=" << it.total_charge << ", total_energy=" << it.total_energy;
  }
  frame->SetTitle(title.str().c_str());

  gPad->Modified(); gPad->Update();
  canvas->Modified(); canvas->Update();
}

int main(int argc, char** argv){
  CmdLineParser clp;
  clp.getDescription() << "> cluster_display - interactive MARLEY cluster viewer (Prev/Next)" << std::endl;
  clp.addDummyOption("Main options");
  clp.addOption("clusters", {"--clusters-file"}, "Input clusters ROOT file (required, must contain 'clusters' in filename)");
  clp.addOption("mode", {"--mode"}, "Display mode: clusters | events (default: clusters)");
  clp.addTriggerOption("onlyMarley", {"--only-marley"}, "In events mode, show only MARLEY clusters");
  clp.addOption("json", {"-j","--json"}, "JSON with input and parameters (optional)");
  clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");
  clp.addDummyOption();
  LogInfo << clp.getDescription().str() << std::endl;
  LogInfo << "Usage:\n" << clp.getConfigSummary() << std::endl;
  clp.parseCmdLine(argc, argv);

  if (clp.isNoOptionTriggered()){
    LogError << "No options provided." << std::endl;
    return 1;
  }

  std::string clustersFile;
  std::string mode = "clusters"; bool onlyMarley=false;

  if (clp.isOptionTriggered("json")){
    std::string jpath = clp.getOptionVal<std::string>("json");
    std::ifstream jf(jpath); if (!jf.is_open()){ LogError << "Cannot open JSON: " << jpath << std::endl; return 1; }
    nlohmann::json j; jf >> j;
    if (j.contains("clusters_file")) clustersFile = j.value("clusters_file", std::string(""));
    if (j.contains("mode")) mode = j.value("mode", mode);
    if (j.contains("only_marley")) onlyMarley = j.value("only_marley", onlyMarley);
  }
  if (clp.isOptionTriggered("clusters")) clustersFile = clp.getOptionVal<std::string>("clusters");
  if (clp.isOptionTriggered("mode")) mode = clp.getOptionVal<std::string>("mode");
  if (clp.isOptionTriggered("onlyMarley")) onlyMarley = true;

  // Validate clusters file
  LogThrowIf(clustersFile.empty(), "Clusters file is required! Provide via --clusters-file or JSON.");
  if (clustersFile.find("clusters") == std::string::npos) {
    LogWarning << "Warning: File '" << clustersFile << "' does not contain 'clusters' in name. Are you sure this is a clusters file?" << std::endl;
  }

  // Read clusters file
  TFile* f = TFile::Open(clustersFile.c_str());
  LogThrowIf(!f || f->IsZombie(), std::string("Cannot open clusters file: ")+clustersFile);
  
  auto readPlaneClusters = [&](const char* plane){
    TTree* t = nullptr;
    if (auto* dir = f->GetDirectory("clusters")) t = dynamic_cast<TTree*>(dir->Get((std::string("clusters_tree_")+plane).c_str()));
    if (!t) t = dynamic_cast<TTree*>(f->Get((std::string("clusters_tree_")+plane).c_str()));
    if (!t) return;
    
    // Branches
    int n_tps = 0;
    float enu = 0.f; double totQ = 0.0, totE = 0.0; std::string* label=nullptr; std::string* inter=nullptr; int evt= -1;
    std::vector<int>* v_ch=nullptr; std::vector<int>* v_ts=nullptr; std::vector<int>* v_sot=nullptr;
    std::vector<int>* v_stopeak=nullptr; std::vector<int>* v_adc_peak=nullptr;
    
    if (t->GetBranch("n_tps")) t->SetBranchAddress("n_tps", &n_tps);
    if (t->GetBranch("true_neutrino_energy")) t->SetBranchAddress("true_neutrino_energy", &enu);
    if (t->GetBranch("total_charge")) t->SetBranchAddress("total_charge", &totQ);
    if (t->GetBranch("total_energy")) t->SetBranchAddress("total_energy", &totE);
    if (t->GetBranch("true_label")) t->SetBranchAddress("true_label", &label);
    if (t->GetBranch("true_interaction")) t->SetBranchAddress("true_interaction", &inter);
    if (t->GetBranch("event")) t->SetBranchAddress("event", &evt);
    if (t->GetBranch("tp_detector_channel")) t->SetBranchAddress("tp_detector_channel", &v_ch);
    if (t->GetBranch("tp_time_start")) t->SetBranchAddress("tp_time_start", &v_ts);
    if (t->GetBranch("tp_samples_over_threshold")) t->SetBranchAddress("tp_samples_over_threshold", &v_sot);
    if (t->GetBranch("tp_samples_to_peak")) t->SetBranchAddress("tp_samples_to_peak", &v_stopeak);
    if (t->GetBranch("tp_adc_peak")) t->SetBranchAddress("tp_adc_peak", &v_adc_peak);
    
    Long64_t n = t->GetEntries();
    if (toLower(mode)=="clusters"){
      for (Long64_t i=0;i<n;++i){
        t->GetEntry(i);
        std::string lab = label? *label : std::string("UNKNOWN");
        std::string low = toLower(lab);
        if (low.find("marley") == std::string::npos) continue;
        
        ViewerState::Item it; 
        it.plane = plane; 
        it.label = lab; 
        it.interaction = inter? *inter : std::string(""); 
        it.enu = enu; 
        it.total_charge = totQ; 
        it.total_energy = totE;
  it.eventId = evt;
        
        if (v_ch) it.ch = *v_ch;
        if (v_ts){ it.tstart.reserve(v_ts->size()); for (int ts : *v_ts) it.tstart.push_back(toTPCticks(ts)); }
        if (v_sot) it.sot = *v_sot; // SoT already in TPC ticks
        if (v_stopeak) it.stopeak = *v_stopeak;
        if (v_adc_peak) it.adc_peak = *v_adc_peak;
        
        ViewerState::items.emplace_back(std::move(it));
      }
    } else {
      // Aggregate by event for this plane
      std::map<int, ViewerState::Item> agg; // eventId -> Item
      for (Long64_t i=0;i<n;++i){
        t->GetEntry(i);
        std::string lab = label? *label : std::string("UNKNOWN");
        std::string low = toLower(lab);
        if (onlyMarley && low.find("marley") == std::string::npos) continue;
        
        auto& it = agg[evt];
        it.isEvent = true; it.eventId = evt; it.plane = plane; it.marleyOnly = onlyMarley; it.nClusters += 1;
        
        if (v_ch){ it.ch.insert(it.ch.end(), v_ch->begin(), v_ch->end()); }
        if (v_ts){ it.tstart.reserve(it.tstart.size()+v_ts->size()); for (int ts : *v_ts) it.tstart.push_back(toTPCticks(ts)); }
        if (v_sot){ it.sot.insert(it.sot.end(), v_sot->begin(), v_sot->end()); }
        if (v_stopeak){ it.stopeak.insert(it.stopeak.end(), v_stopeak->begin(), v_stopeak->end()); }
        if (v_adc_peak){ it.adc_peak.insert(it.adc_peak.end(), v_adc_peak->begin(), v_adc_peak->end()); }
      }
      // Only keep events with more than 1 TP
      for (auto &kv : agg){ if (kv.second.ch.size() > 1) ViewerState::items.emplace_back(std::move(kv.second)); }
    }
  };
  
  readPlaneClusters("U"); readPlaneClusters("V"); readPlaneClusters("X");
  f->Close(); delete f;

  if (ViewerState::items.empty()){
    LogWarning << "No MARLEY clusters found with current settings." << std::endl;
  }

  int fake_argc = 0; char** fake_argv = nullptr; TApplication app("cluster_display", &fake_argc, fake_argv);
  ViewerState::canvas = new TCanvas("cluster_display", "MARLEY cluster viewer", 1200, 800);
  ViewerState::idx = 0;
  drawCurrent();
  app.Run();
  return 0;
}
