#include "Clustering.h"
#include "Display.h"

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
    std::vector<int> adc_integral; // ADC integral for pentagon mode
    std::string label;
    std::string interaction;
    float enu = 0.0f;
    double total_charge = 0.0;
    double total_energy = 0.0;
    float marley_tp_fraction = 0.0f;
    float generator_tp_fraction = 0.0f;
    // Per-TP cluster membership and category (for events mode)
    std::vector<std::string> tp_category;  // Category for each TP
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
  DrawMode drawMode = PENTAGON; // Default to rectangle mode
  bool noTPs = false; // If true, show clusters as blobs instead of individual TPs
  bool useCmUnits = true; // If true, use cm units; if false, use detector units (channels/ticks)
}

// Helper function to determine cluster category
std::string getClusterCategory(float marley_tp_fraction, float generator_tp_fraction) {
  if (marley_tp_fraction == 1.0f) {
    return "Pure Marley";
  } else if (marley_tp_fraction == 0.0f && generator_tp_fraction == 0.0f) {
    return "Pure Noise";
  } else if (marley_tp_fraction == 0.0f && generator_tp_fraction > 0.0f) {
    return "Pure Background";
  } else if (marley_tp_fraction > 0.0f && marley_tp_fraction < 1.0f) {
    if (generator_tp_fraction == marley_tp_fraction) {
      return "Marley+Noise";
    } else {
      return "Marley+Background";
    }
  }
  return "Unknown";
}

// Helper function to get color for cluster category
int getCategoryColor(const std::string& category) {
  if (category == "Pure Marley") return kBlue;
  if (category == "Pure Noise") return kGray+2;
  if (category == "Marley+Noise") return kGreen+2;
  if (category == "Pure Background") return kRed;
  if (category == "Marley+Background") return kMagenta+2;
  return kBlack;
}

// static std::string toLower(std::string s){ std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c);}); return s; }

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

  // Current Cluster
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
  
  // Set axis titles based on units mode
  if (ViewerState::useCmUnits) {
    // Get conversion parameters
    double wire_pitch_cm = GET_PARAM_DOUBLE("geometry.wire_pitch_collection_cm"); // Default to collection
    if (it.plane == "U" || it.plane == "V") {
      wire_pitch_cm = GET_PARAM_DOUBLE("geometry.wire_pitch_induction_diagonal_cm");
    }
    double time_tick_cm = GET_PARAM_DOUBLE("timing.time_tick_cm");
    
    // Calculate physical ranges for axis labels
    double xmin_cm = xmin * wire_pitch_cm;
    double xmax_cm = xmax * wire_pitch_cm;
    double ymin_cm = ymin * time_tick_cm;
    double ymax_cm = ymax * time_tick_cm;
    
    frame->SetTitle(";Z [cm];X [cm]");
  } else {
    frame->SetTitle(";channel (contiguous);time [ticks]");
  }
  
  frame->GetXaxis()->CenterTitle();
  frame->GetYaxis()->CenterTitle();
  const char* mode_name = (drawMode == PENTAGON) ? "pentagon" : (drawMode == TRIANGLE) ? "triangle" : "rectangle";
  frame->GetZaxis()->SetTitle(Form("ADC (%s model)", mode_name));
  frame->GetZaxis()->CenterTitle();

  // Set margins to accommodate secondary axes and title
  gPad->SetRightMargin(0.25);  // Increased for Z-axis legend and Y-axis labels
  gPad->SetLeftMargin(0.12);
  gPad->SetBottomMargin(0.12);
  gPad->SetTopMargin(0.15);    // Increased for top X-axis + title
  gPad->SetGridx();
  gPad->SetGridy();

  // Get threshold based on plane
  double threshold_adc = 60.0;
  if (it.plane == "U") threshold_adc = GET_PARAM_DOUBLE("display.threshold_adc_u");
  else if (it.plane == "V") threshold_adc = GET_PARAM_DOUBLE("display.threshold_adc_v");
  else if (it.plane == "X") threshold_adc = GET_PARAM_DOUBLE("display.threshold_adc_x");

  if (ViewerState::noTPs) {
    // Blob mode: don't fill histogram, just set it up as background
    // Boxes will be drawn on top
    
  } else {
    // Normal TP mode: draw individual TPs
    for (size_t i=0;i<nTPs;++i){
      int ts = it.tstart[i];
      int tot = it.sot[i];
      int ch_actual = it.ch[i];
      int ch_contiguous = ch_to_idx[ch_actual];

      int samples_to_peak = (i < it.stopeak.size()) ? it.stopeak[i] : (tot > 0 ? tot / 2 : 0);
      int peak_adc = (i < it.adc_peak.size()) ? it.adc_peak[i] : 200;
      int peak_time = ts + samples_to_peak;
      int adc_integral = (i < it.adc_integral.size()) ? it.adc_integral[i] : peak_adc * tot / 2;

      if (debugMode) {
        LogInfo << "Drawing TP " << i << ": ch=" << ch_actual << " ts=" << ts 
                << " tot=" << tot << " end=" << (ts+tot) 
                << " samples_to_peak=" << samples_to_peak << " peak_time=" << peak_time << std::endl;
      }

      if (drawMode == PENTAGON) {
        fillHistogramPentagon(
          frame, ch_contiguous, ts, peak_time, tot,
          peak_adc, adc_integral, threshold_adc
        );
      } else if (drawMode == TRIANGLE) {
        fillHistogramTriangle(
          frame, ch_contiguous, ts, tot,
          samples_to_peak, peak_adc, threshold_adc
        );
      } else { // RECTANGLE
        fillHistogramRectangle(
          frame, ch_contiguous, ts, tot, adc_integral
        );
      }
    }
  }

  frame->SetMinimum(threshold_adc);
  // Use Viridis perceptually-uniform palette
  gStyle->SetPalette(55);
  frame->Draw("COLZ");
  
  // In blob mode, draw category-colored boxes for cluster regions
  if (ViewerState::noTPs) {
    // In events mode, we need to identify individual clusters within the aggregated event
    // and draw each cluster as a separate blob with its correct category color
    
    // Get conversion parameters for coordinate transformation
    double wire_pitch_cm = GET_PARAM_DOUBLE("geometry.wire_pitch_collection_cm");
    if (it.plane == "U" || it.plane == "V") {
      wire_pitch_cm = GET_PARAM_DOUBLE("geometry.wire_pitch_induction_diagonal_cm");
    }
    double time_tick_cm = GET_PARAM_DOUBLE("timing.time_tick_cm");
    
    // Draw each TP with its correct category color
    for (size_t i=0;i<nTPs;++i){
      int ts = it.tstart[i];
      int tot = it.sot[i];
      int te = ts + tot;
      int ch_actual = it.ch[i];
      int ch_contiguous = ch_to_idx[ch_actual];
      
      // Get category and color for this specific TP
      std::string category;
      if (it.isEvent && i < it.tp_category.size()) {
        // Events mode: use stored per-TP category
        category = it.tp_category[i];
      } else {
        // Clusters mode: use overall cluster category
        category = getClusterCategory(it.marley_tp_fraction, it.generator_tp_fraction);
      }
      int color = getCategoryColor(category);
      
      // Convert coordinates to correct units (cm if useCmUnits, otherwise detector units)
      double x1, x2, y1, y2;
      if (ViewerState::useCmUnits) {
        x1 = (ch_contiguous - 0.5) * wire_pitch_cm;
        x2 = (ch_contiguous + 0.5) * wire_pitch_cm;
        y1 = (ts - 0.5) * time_tick_cm;
        y2 = (te - 0.5) * time_tick_cm;
      } else {
        x1 = ch_contiguous - 0.5;
        x2 = ch_contiguous + 0.5;
        y1 = ts - 0.5;
        y2 = te - 0.5;
      }
      
      TBox* blob = new TBox(x1, y1, x2, y2);
      blob->SetFillColor(color);
      blob->SetFillStyle(1001);  // Solid fill
      blob->SetLineWidth(0);     // No outline
      blob->Draw("same");
    }

    
    // Add legend showing cluster categories
    TLegend* leg = new TLegend(0.15, 0.70, 0.40, 0.88);
    leg->SetHeader("Cluster Categories", "C");
    leg->SetTextSize(0.03);
    leg->SetFillStyle(1001);
    leg->SetFillColor(kWhite);
    leg->SetBorderSize(1);
    
    // Add all categories as reference
    TBox* box1 = new TBox(0,0,1,1); box1->SetFillColor(kBlue); box1->SetFillStyle(1001);
    leg->AddEntry(box1, "Pure Marley", "f");
    
    TBox* box2 = new TBox(0,0,1,1); box2->SetFillColor(kGray+2); box2->SetFillStyle(1001);
    leg->AddEntry(box2, "Pure Noise", "f");
    
    TBox* box3 = new TBox(0,0,1,1); box3->SetFillColor(kGreen+2); box3->SetFillStyle(1001);
    leg->AddEntry(box3, "Marley+Noise", "f");
    
    TBox* box4 = new TBox(0,0,1,1); box4->SetFillColor(kRed); box4->SetFillStyle(1001);
    leg->AddEntry(box4, "Pure Background", "f");
    
    TBox* box5 = new TBox(0,0,1,1); box5->SetFillColor(kMagenta+2); box5->SetFillStyle(1001);
    leg->AddEntry(box5, "Marley+Background", "f");

    
    leg->Draw();
  }
  
  // Relabel X-axis bins with actual channel numbers instead of contiguous indices (only in detector units mode)
  if (!ViewerState::useCmUnits) {
    auto xax = frame->GetXaxis();
    xax->LabelsOption("v");
    const int labelOffset = /* pad_bins */ 2 + 1; // pad_bins + 1
    for (const auto& kv : ch_to_idx) {
      int actual_ch = kv.first;
      int idx = kv.second;
      int bin = idx + labelOffset;
      xax->SetBinLabel(bin, Form("%d", actual_ch));
    }
  }

  // In cm units mode, rescale axes to show physical dimensions
  if (ViewerState::useCmUnits) {
    // Get conversion parameters
    double wire_pitch_cm = GET_PARAM_DOUBLE("geometry.wire_pitch_collection_cm"); // Default to collection
    if (it.plane == "U" || it.plane == "V") {
      wire_pitch_cm = GET_PARAM_DOUBLE("geometry.wire_pitch_induction_diagonal_cm");
    }
    double time_tick_cm = GET_PARAM_DOUBLE("timing.time_tick_cm");
    
    // Rescale the histogram axes
    frame->GetXaxis()->Set(nbinsX, xmin * wire_pitch_cm, xmax * wire_pitch_cm);
    frame->GetYaxis()->Set(nbinsY, ymin * time_tick_cm, ymax * time_tick_cm);
  }

  // Update main frame to have better title positioning
  frame->GetXaxis()->SetTitleOffset(1.1);
  frame->GetYaxis()->SetTitleOffset(1.3);
  frame->GetZaxis()->SetTitleOffset(1.4);
  gPad->Update();
  
  // Create secondary X axis (top) for channel dimension in cm
  // Position it slightly below the top edge to avoid title overlap
  // TGaxis *axis_x_cm = new TGaxis(xmin, ymax, xmax, ymax, xmin_cm, xmax_cm, 510, "-L");
  // axis_x_cm->SetTitle("Z [cm]");
  // axis_x_cm->SetTitleSize(0.028);
  // axis_x_cm->SetLabelSize(0.023);
  // axis_x_cm->SetTitleOffset(1.1);  // Push title away from labels
  // axis_x_cm->SetLabelOffset(0.005);
  // axis_x_cm->CenterTitle();
  // axis_x_cm->SetLineColor(kBlack);
  // axis_x_cm->SetTextColor(kBlack);
  // axis_x_cm->Draw();
  
  // Create secondary Y axis (inside) for drift distance in cm  
  // Shift axis left by one bin to avoid overlapping with Z colorbar
  // double axis_y_pos = xmax - 1.0;
  // TGaxis *axis_y_cm = new TGaxis(axis_y_pos, ymin, axis_y_pos, ymax, ymin_cm, ymax_cm, 510, "+L");
  // axis_y_cm->SetTitle("X [cm]");
  // axis_y_cm->SetTitleSize(0.028);
  // axis_y_cm->SetLabelSize(0.023);
  // axis_y_cm->SetTitleOffset(1.7);  // Push away from Z-axis legend
  // axis_y_cm->SetLabelOffset(0.015);  // Space from plot edge
  // axis_y_cm->CenterTitle();
  // axis_y_cm->SetLineColor(kBlack);
  // axis_y_cm->SetTextColor(kBlack);
  // axis_y_cm->Draw();

  // Compose title string for the plot (canvas title, not window)
  std::ostringstream title;
  
  // Check for X plane TPC volume mixing and time spread diagnostics
  std::string volume_warning = "";
  if (it.plane == "X") {
    int min_ch = *std::min_element(it.ch.begin(), it.ch.end());
    int max_ch = *std::max_element(it.ch.begin(), it.ch.end());
    
    // Get time range (in TPC ticks)
    int min_time = *std::min_element(it.tstart.begin(), it.tstart.end());
    int max_time = *std::max_element(it.tstart.begin(), it.tstart.end());
    int time_spread_ticks = max_time - min_time;
    double time_spread_cm = time_spread_ticks * GET_PARAM_DOUBLE("timing.time_tick_cm");
    
    // X plane channels: 1600-2079 (volume 0), 2080-2559 (volume 1)
    bool has_volume0 = false;
    bool has_volume1 = false;
    for (int ch : it.ch) {
      int ch_in_plane = ch % 2560;
      if (ch_in_plane >= 1600 && ch_in_plane < 2080) has_volume0 = true;
      if (ch_in_plane >= 2080 && ch_in_plane < 2560) has_volume1 = true;
    }
    
    if (has_volume0 && has_volume1) {
      volume_warning = " | WARNING: MIXED TPC VOLUMES!";
    }
    
    // title << "Ch range: " << min_ch << "-" << max_ch;
    if (has_volume0) title << " [Vol0]";
    if (has_volume1) title << " [Vol1]";
    // title << " | Time spread: " << time_spread_ticks << " ticks (" << std::fixed << std::setprecision(1) << time_spread_cm << " cm)";
    title << volume_warning << " | ";
  }
  
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
  clp.getDescription() << "> display - interactive MARLEY Cluster viewer (Prev/Next)" << std::endl;
  clp.addDummyOption("Main options");
  clp.addOption("clusters", {"--clusters-file"}, "Input clusters ROOT file (required, must contain 'clusters' in filename)");
  clp.addOption("mode", {"--mode"}, "Display mode: clusters | events (default: clusters)");
  clp.addOption("drawMode", {"--draw-mode"}, "Drawing mode: triangle | pentagon | rectangle (default: pentagon)");
  clp.addOption("units", {"--units"}, "Axis units: cm | det (detector units: channels/ticks) (default: cm)");
  clp.addTriggerOption("onlyMarley", {"--only-marley"}, "In events mode, show only MARLEY clusters");
  clp.addTriggerOption("noTPs", {"--no-tps"}, "Show clusters as blobs without individual TPs (with category legend)");
  clp.addOption("json", {"-j","--json"}, "JSON with input and parameters (optional)");
  clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");
  clp.addTriggerOption("debugMode", {"-d"}, "Run in debug mode (more detailed than verbose)");
  clp.addDummyOption();
  LogInfo << clp.getDescription().str() << std::endl;
  LogInfo << "Usage:\n" << clp.getConfigSummary() << std::endl;
  clp.parseCmdLine(argc, argv);

  if (clp.isNoOptionTriggered()){
    LogError << "No options provided." << std::endl;
    return 1;
  }

  // verbosity updating global variables
  if (clp.isOptionTriggered("verboseMode")) { verboseMode = true; }
  if (clp.isOptionTriggered("debugMode")) { verboseMode =true; debugMode = true; }
  gErrorIgnoreLevel = rootVerbosity;  

  TApplication app("display", &argc, argv);

  std::string clustersFile;
  std::string mode = "clusters"; 
  std::string drawModeStr = "pentagon";
  std::string unitsStr = "cm";
  bool onlyMarley=false;
  bool noTPs=false;

  if (clp.isOptionTriggered("json")){
    std::string jpath = clp.getOptionVal<std::string>("json");
    std::ifstream jf(jpath); if (!jf.is_open()){ LogError << "Cannot open JSON: " << jpath << std::endl; return 1; }
    nlohmann::json j; jf >> j;
    if (j.contains("clusters_file")) clustersFile = j.value("clusters_file", std::string(""));
    if (j.contains("mode")) mode = j.value("mode", mode);
    if (j.contains("draw_mode")) drawModeStr = j.value("draw_mode", drawModeStr);
    if (j.contains("units")) unitsStr = j.value("units", unitsStr);
    if (j.contains("only_marley")) onlyMarley = j.value("only_marley", onlyMarley);
    if (j.contains("no_tps")) noTPs = j.value("no_tps", noTPs);
  }
  if (clp.isOptionTriggered("clusters")) clustersFile = clp.getOptionVal<std::string>("clusters");
  if (clp.isOptionTriggered("mode")) mode = clp.getOptionVal<std::string>("mode");
  if (clp.isOptionTriggered("drawMode")) drawModeStr = clp.getOptionVal<std::string>("drawMode");
  if (clp.isOptionTriggered("units")) unitsStr = clp.getOptionVal<std::string>("units");
  if (clp.isOptionTriggered("onlyMarley")) onlyMarley = true;
  if (clp.isOptionTriggered("noTPs")) noTPs = true;
  
  ViewerState::noTPs = noTPs;

  // Set draw mode
  if (toLower(drawModeStr) == "triangle") {
    ViewerState::drawMode = TRIANGLE;
  } else if (toLower(drawModeStr) == "pentagon") {
    ViewerState::drawMode = PENTAGON;
  } else {
    ViewerState::drawMode = RECTANGLE;
  }

  // Set units mode
  if (toLower(unitsStr) == "det") {
    ViewerState::useCmUnits = false;
  } else {
    ViewerState::useCmUnits = true; // default is cm
  }

  // Load parameters explicitly (in case global initialization failed)
  try {
    ParametersManager::getInstance().loadParameters();
    LogInfo << "Parameters loaded successfully" << std::endl;
  } catch (const std::exception& e) {
    LogError << "Failed to load parameters: " << e.what() << std::endl;
    return 1;
  }

  // print all info
  LogInfo << "Input clusters file: " << clustersFile << std::endl;
  LogInfo << "Display mode: " << mode << std::endl;
  const char* mode_name = (ViewerState::drawMode == PENTAGON) ? "pentagon" : (ViewerState::drawMode == TRIANGLE) ? "triangle" : "rectangle";
  LogInfo << "Draw mode: " << mode_name << std::endl;
  LogInfo << "Axis units: " << (ViewerState::useCmUnits ? "cm" : "detector (channels/ticks)") << std::endl;
  LogInfo << "Only MARLEY clusters in events mode: " << (onlyMarley ? "enabled" : "disabled") << std::endl;
  LogInfo << "Show clusters as blobs (--no-tps): " << (noTPs ? "enabled" : "disabled") << std::endl;


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
    float marley_frac = 0.0f, gen_frac = 0.0f;
    std::vector<int>* v_ch=nullptr; std::vector<int>* v_ts=nullptr; std::vector<int>* v_sot=nullptr;
    std::vector<int>* v_stopeak=nullptr; std::vector<int>* v_adc_peak=nullptr; std::vector<int>* v_adc_integral=nullptr;
    
    if (t->GetBranch("n_tps")) t->SetBranchAddress("n_tps", &n_tps);
    if (t->GetBranch("true_neutrino_energy")) t->SetBranchAddress("true_neutrino_energy", &enu);
    if (t->GetBranch("total_charge")) t->SetBranchAddress("total_charge", &totQ);
    if (t->GetBranch("total_energy")) t->SetBranchAddress("total_energy", &totE);
    if (t->GetBranch("true_label")) t->SetBranchAddress("true_label", &label);
    if (t->GetBranch("true_interaction")) t->SetBranchAddress("true_interaction", &inter);
    if (t->GetBranch("event")) t->SetBranchAddress("event", &evt);
    if (t->GetBranch("marley_tp_fraction")) t->SetBranchAddress("marley_tp_fraction", &marley_frac);
    if (t->GetBranch("generator_tp_fraction")) t->SetBranchAddress("generator_tp_fraction", &gen_frac);
    if (t->GetBranch("tp_detector_channel")) t->SetBranchAddress("tp_detector_channel", &v_ch);
    if (t->GetBranch("tp_time_start")) t->SetBranchAddress("tp_time_start", &v_ts);
    if (t->GetBranch("tp_samples_over_threshold")) t->SetBranchAddress("tp_samples_over_threshold", &v_sot);
    if (t->GetBranch("tp_samples_to_peak")) t->SetBranchAddress("tp_samples_to_peak", &v_stopeak);
    if (t->GetBranch("tp_adc_peak")) t->SetBranchAddress("tp_adc_peak", &v_adc_peak);
    if (t->GetBranch("tp_adc_integral")) t->SetBranchAddress("tp_adc_integral", &v_adc_integral);
    
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
        it.marley_tp_fraction = marley_frac;
        it.generator_tp_fraction = gen_frac;
        
        if (v_ch) it.ch = *v_ch;
        if (v_ts){ it.tstart.reserve(v_ts->size()); for (int ts : *v_ts) it.tstart.push_back(toTPCticks(ts)); }
        if (v_sot) it.sot = *v_sot; // SoT already in TPC ticks
        if (v_stopeak) it.stopeak = *v_stopeak;
        if (v_adc_peak) it.adc_peak = *v_adc_peak;
        if (v_adc_integral) it.adc_integral = *v_adc_integral;
        
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
        // For event mode, we aggregate fractions (use average or first cluster's values)
        if (it.nClusters == 1) {
          it.marley_tp_fraction = marley_frac;
          it.generator_tp_fraction = gen_frac;
        }
        
        // Determine category for this cluster
        std::string cluster_category = getClusterCategory(marley_frac, gen_frac);
        
        if (v_ch){ it.ch.insert(it.ch.end(), v_ch->begin(), v_ch->end()); }
        if (v_ts){ it.tstart.reserve(it.tstart.size()+v_ts->size()); for (int ts : *v_ts) it.tstart.push_back(toTPCticks(ts)); }
        if (v_sot){ it.sot.insert(it.sot.end(), v_sot->begin(), v_sot->end()); }
        if (v_stopeak){ it.stopeak.insert(it.stopeak.end(), v_stopeak->begin(), v_stopeak->end()); }
        if (v_adc_peak){ it.adc_peak.insert(it.adc_peak.end(), v_adc_peak->begin(), v_adc_peak->end()); }
        if (v_adc_integral){ it.adc_integral.insert(it.adc_integral.end(), v_adc_integral->begin(), v_adc_integral->end()); }
        
        // Store category for each TP in this cluster
        if (v_ch) {
          for (size_t tp_idx = 0; tp_idx < v_ch->size(); ++tp_idx) {
            it.tp_category.push_back(cluster_category);
          }
        }
      }
      // Only keep events with more than 1 TP
      for (auto &kv : agg){ if (kv.second.ch.size() > 1) ViewerState::items.emplace_back(std::move(kv.second)); }
    }
  };
  
  readPlaneClusters("X"); readPlaneClusters("U"); readPlaneClusters("V");
  f->Close(); delete f;

  LogInfo << "Loaded " << ViewerState::items.size() << " MARLEY clusters total (all planes)" << std::endl;

  if (ViewerState::items.empty()){
    LogWarning << "No MARLEY clusters found with current settings." << std::endl;
  }

  ViewerState::canvas = new TCanvas("display", "MARLEY Cluster viewer", 1200, 800);
  ViewerState::idx = 0;
  drawCurrent();
  app.Run();
  return 0;
}
