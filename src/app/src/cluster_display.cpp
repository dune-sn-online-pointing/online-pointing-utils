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
  size_t nTPs = std::min(it.ch.size(), std::min(it.tstart.size(), it.sot.size()));
  if (nTPs == 0) return;

  // Determine axis ranges (ticks and channels)
  int tmin = INT_MAX, tmax = INT_MIN, cmin = INT_MAX, cmax = INT_MIN;
  for (size_t i=0;i<nTPs;++i){
    int ts = it.tstart[i];
    int te = it.tstart[i] + it.sot[i];
    int ch = it.ch[i];
    if (ts < tmin) tmin = ts; if (te > tmax) tmax = te;
    if (ch < cmin) cmin = ch; if (ch > cmax) cmax = ch;
  }
  if (tmin>tmax) { tmin=0; tmax=1; }
  if (cmin>cmax) { cmin=0; cmax=1; }
  int tpad = it.isEvent ? 20 : 5;
  int cpad = it.isEvent ? 10 : 2;
  // Swap axes: X = channel, Y = time [ticks]
  double xmin = cmin - cpad, xmax = cmax + cpad;
  double ymin = tmin - tpad, ymax = tmax + tpad;

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
  // Draw axis frame without creating named histograms (X=channel, Y=time)
  gPad->DrawFrame(xmin, ymin, xmax, ymax, ";channel;time [ticks]");
  gPad->SetGridx();
  gPad->SetGridy();

  // Draw each TP as a vertical box: fixed channel (x-range +/-0.45), y-range from time_start to time_start+ToT
  int color = kBlue+1;
  for (size_t i=0;i<nTPs;++i){
    double ts = it.tstart[i];
    double te = it.tstart[i] + it.sot[i];
    double ch = it.ch[i];
    auto* b = new TBox(ch-0.45, ts, ch+0.45, te);
    b->SetFillColorAlpha(color, 0.35);
    b->SetLineColor(kBlack);
    b->Draw();
  }

  // Annotation
  latex = new TLatex(); latex->SetNDC(); latex->SetTextSize(0.030); latex->SetTextAlign(13);
  double y = 0.88, dy=0.06; char buf[256];
  if (it.isEvent){
    snprintf(buf,sizeof(buf),"Event %d | plane %s | nTPs=%zu | nClusters=%d", it.eventId, it.plane.c_str(), nTPs, it.nClusters); latex->DrawLatex(0.12,y,buf); y-=dy;
    snprintf(buf,sizeof(buf),"mode=events, filter=%s", it.marleyOnly? "MARLEY-only" : "All clusters"); latex->DrawLatex(0.12,y,buf);
  } else {
    snprintf(buf,sizeof(buf),"Cluster %d/%d | plane %s | nTPs=%zu", idx+1, (int)items.size(), it.plane.c_str(), nTPs); latex->DrawLatex(0.12,y,buf); y-=dy;
    snprintf(buf,sizeof(buf),"true_label=%s, interaction=%s", it.label.c_str(), it.interaction.c_str()); latex->DrawLatex(0.12,y,buf); y-=dy;
    snprintf(buf,sizeof(buf),"E_nu=%.1f MeV, total_charge=%.0f, total_energy=%.0f", it.enu, it.total_charge, it.total_energy); latex->DrawLatex(0.12,y,buf);
  }

  gPad->Modified(); gPad->Update();
  canvas->Modified(); canvas->Update();
}

int main(int argc, char** argv){
  CmdLineParser clp;
  clp.getDescription() << "> cluster_display - interactive MARLEY cluster viewer (Prev/Next)" << std::endl;
  clp.addDummyOption("Main options");
  clp.addOption("filename", {"-f","--filename"}, "Input TPs ROOT file (optional if --clusters-file is given)");
  clp.addOption("clusters", {"--clusters-file"}, "Input clusters ROOT file");
  clp.addOption("mode", {"--mode"}, "Display mode: clusters | events (default: clusters)");
  clp.addTriggerOption("onlyMarley", {"--only-marley"}, "In events mode, show only MARLEY clusters");
  clp.addOption("json", {"-j","--json"}, "JSON with input and parameters (optional)");
  clp.addOption("tick", {"--tick-limit"}, "Tick proximity for clustering (default 3)");
  clp.addOption("chan", {"--channel-limit"}, "Channel proximity for clustering (default 1)");
  clp.addOption("min", {"--min-tps"}, "Minimum TPs per cluster (default 1)");
  clp.addOption("tot", {"--tot-cut"}, "TP ToT cut before clustering (default 0)");
  clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");
  clp.addDummyOption();
  LogInfo << clp.getDescription().str() << std::endl;
  LogInfo << "Usage:\n" << clp.getConfigSummary() << std::endl;
  clp.parseCmdLine(argc, argv);

  if (clp.isNoOptionTriggered()){
    LogError << "No options provided." << std::endl;
    return 1;
  }

  std::string filename;
  std::string clustersFile;
  std::string mode = "clusters"; bool onlyMarley=false;
  int tick_limit = 3, channel_limit = 1, min_tps = 1, tot_cut = 0;

  if (clp.isOptionTriggered("json")){
    std::string jpath = clp.getOptionVal<std::string>("json");
    std::ifstream jf(jpath); if (!jf.is_open()){ LogError << "Cannot open JSON: " << jpath << std::endl; return 1; }
    nlohmann::json j; jf >> j;
    if (j.contains("filename")) filename = j.value("filename", std::string(""));
  if (j.contains("clusters_file")) clustersFile = j.value("clusters_file", std::string(""));
  if (j.contains("mode")) mode = j.value("mode", mode);
  if (j.contains("only_marley")) onlyMarley = j.value("only_marley", onlyMarley);
    tick_limit = j.value("tick_limit", tick_limit);
    channel_limit = j.value("channel_limit", channel_limit);
    min_tps = j.value("min_tps_to_cluster", min_tps);
    tot_cut = j.value("tot_cut", tot_cut);
  }
  if (clp.isOptionTriggered("filename")) filename = clp.getOptionVal<std::string>("filename");
  if (clp.isOptionTriggered("clusters")) clustersFile = clp.getOptionVal<std::string>("clusters");
  if (clp.isOptionTriggered("mode")) mode = clp.getOptionVal<std::string>("mode");
  if (clp.isOptionTriggered("onlyMarley")) onlyMarley = true;
  if (clp.isOptionTriggered("tick")) tick_limit = clp.getOptionVal<int>("tick");
  if (clp.isOptionTriggered("chan")) channel_limit = clp.getOptionVal<int>("chan");
  if (clp.isOptionTriggered("min")) min_tps = clp.getOptionVal<int>("min");
  if (clp.isOptionTriggered("tot")) tot_cut = clp.getOptionVal<int>("tot");

  // If clusters file provided, read clusters; else fall back to TPs and clustering on the fly
  if (!clustersFile.empty()){
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
      Long64_t n = t->GetEntries();
      if (toLower(mode)=="clusters"){
        for (Long64_t i=0;i<n;++i){
          t->GetEntry(i);
          std::string lab = label? *label : std::string("UNKNOWN");
          std::string low = toLower(lab);
          if (low.find("marley") == std::string::npos) continue;
          ViewerState::Item it; it.plane = plane; it.label = lab; it.interaction = inter? *inter : std::string(""); it.enu = enu; it.total_charge = totQ; it.total_energy = totE;
          if (v_ch) it.ch = *v_ch;
          if (v_ts){ it.tstart.reserve(v_ts->size()); for (int ts : *v_ts) it.tstart.push_back(toTPCticks(ts)); }
          if (v_sot) it.sot = *v_sot; // SoT already in TPC ticks
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
        }
        // Only keep events with more than 1 TP
        for (auto &kv : agg){ if (kv.second.ch.size() > 1) ViewerState::items.emplace_back(std::move(kv.second)); }
      }
    };
    readPlaneClusters("U"); readPlaneClusters("V"); readPlaneClusters("X");
    f->Close(); delete f;
  } else {
    LogThrowIf(filename.empty(), "Provide input TPs ROOT file via --filename or JSON when --clusters-file is not given.");
    // Read TPs
    std::map<int, std::vector<TriggerPrimitive>> tps_by_event;
    std::map<int, std::vector<TrueParticle>> true_by_event;
    std::map<int, std::vector<Neutrino>> nu_by_event;
    read_tps_from_root(filename, tps_by_event, true_by_event, nu_by_event);
    // Apply ToT cut
    if (tot_cut > 0){ for (auto &kv : tps_by_event){ auto &vec = kv.second; vec.erase(std::remove_if(vec.begin(), vec.end(), [&](const TriggerPrimitive& tp){ return (int)tp.GetSamplesOverThreshold() <= tot_cut; }), vec.end()); } }
    // On-the-fly clustering
    for (auto &kv : tps_by_event){
      int evtId = kv.first;
      auto &vec = kv.second; std::vector<TriggerPrimitive*> vU, vV, vX;
      for (auto &tp : vec){ if (tp.GetView()=="U") vU.push_back(&tp); else if (tp.GetView()=="V") vV.push_back(&tp); else if (tp.GetView()=="X") vX.push_back(&tp); }
    auto addPlane = [&](std::vector<TriggerPrimitive*>& v, const char* plane){
        std::vector<cluster> cs = cluster_maker(v, tick_limit, channel_limit, min_tps, 0);
        if (toLower(mode)=="clusters"){
      for (auto &c : cs){ std::string low = toLower(c.get_true_label()); if (low.find("marley") == std::string::npos) continue; ViewerState::Item it; it.plane=plane; it.label=c.get_true_label(); it.interaction=c.get_true_interaction(); it.enu=c.get_true_neutrino_energy(); it.total_charge=c.get_total_charge(); it.total_energy=c.get_total_energy(); auto ctps=c.get_tps(); it.ch.reserve(ctps.size()); it.tstart.reserve(ctps.size()); it.sot.reserve(ctps.size()); for (auto* tp : ctps){ it.ch.push_back(tp->GetDetectorChannel()); it.tstart.push_back(toTPCticks((int)tp->GetTimeStart())); it.sot.push_back((int)tp->GetSamplesOverThreshold()); } ViewerState::items.emplace_back(std::move(it)); }
        } else {
      ViewerState::Item agg; agg.isEvent=true; agg.eventId=evtId; agg.plane=plane; agg.marleyOnly=onlyMarley;
      for (auto &c : cs){ std::string low = toLower(c.get_true_label()); if (onlyMarley && low.find("marley")==std::string::npos) continue; auto ctps=c.get_tps(); agg.nClusters += 1; for (auto* tp : ctps){ agg.ch.push_back(tp->GetDetectorChannel()); agg.tstart.push_back(toTPCticks((int)tp->GetTimeStart())); agg.sot.push_back((int)tp->GetSamplesOverThreshold()); } }
      if (agg.ch.size() > 1) ViewerState::items.emplace_back(std::move(agg));
        }
      };
      addPlane(vU, "U"); addPlane(vV, "V"); addPlane(vX, "X");
    }
  }

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
