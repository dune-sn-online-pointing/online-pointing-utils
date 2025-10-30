#include "Clustering.h"

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

// Helper function to add page number to canvas
void addPageNumber(TCanvas* canvas, int pageNum, int totalPages) {
  canvas->cd();
  TText* pageText = new TText(0.95, 0.97, Form("Page %d/%d", pageNum, totalPages));
  pageText->SetTextAlign(31); // right-aligned
  pageText->SetTextSize(0.02);
  pageText->SetNDC();
  pageText->Draw();
}

// Helper function to create binned averages with equal bin sizes
// Returns a TGraphErrors with averaged points
TGraphErrors* createBinnedAverageGraph(const std::vector<double>& x_data, const std::vector<double>& y_data) {
  if (x_data.size() != y_data.size() || x_data.empty()) {
    return nullptr;
  }
  
  // Create pairs and sort by x value
  std::vector<std::pair<double, double>> data;
  for (size_t i = 0; i < x_data.size(); i++) {
    data.push_back({x_data[i], y_data[i]});
  }
  std::sort(data.begin(), data.end());
  
  // Define bins with equal size of 5 MeV
  std::vector<double> bin_edges;
  
  // Find data range
  double x_min = 0.0;  // Start from 0
  double x_max = 70.0; // Maximum energy we care about
  
  // Create equal-sized bins of 5 MeV
  double bin_width = 5.0;
  for (double edge = x_min; edge <= x_max; edge += bin_width) {
    bin_edges.push_back(edge);
  }
  
  // Calculate averages in each bin
  std::vector<double> bin_centers, bin_y_avg, bin_x_err, bin_y_err;
  
  size_t data_idx = 0;
  for (size_t bin_idx = 0; bin_idx < bin_edges.size() - 1; bin_idx++) {
    double bin_low = bin_edges[bin_idx];
    double bin_high = bin_edges[bin_idx + 1];
    
    std::vector<double> x_in_bin, y_in_bin;
    
    // Collect points in this bin
    while (data_idx < data.size() && data[data_idx].first < bin_high) {
      if (data[data_idx].first >= bin_low) {
        x_in_bin.push_back(data[data_idx].first);
        y_in_bin.push_back(data[data_idx].second);
      }
      data_idx++;
    }
    
    // Calculate averages if we have points
    if (!x_in_bin.empty()) {
      double x_mean = std::accumulate(x_in_bin.begin(), x_in_bin.end(), 0.0) / x_in_bin.size();
      double y_mean = std::accumulate(y_in_bin.begin(), y_in_bin.end(), 0.0) / y_in_bin.size();
      
      // Calculate standard errors
      double x_std = 0, y_std = 0;
      for (size_t i = 0; i < x_in_bin.size(); i++) {
        x_std += (x_in_bin[i] - x_mean) * (x_in_bin[i] - x_mean);
        y_std += (y_in_bin[i] - y_mean) * (y_in_bin[i] - y_mean);
      }
      x_std = std::sqrt(x_std / x_in_bin.size()) / std::sqrt(x_in_bin.size());
      y_std = std::sqrt(y_std / y_in_bin.size()) / std::sqrt(y_in_bin.size());
      
      bin_centers.push_back(x_mean);
      bin_y_avg.push_back(y_mean);
      bin_x_err.push_back(x_std);
      bin_y_err.push_back(y_std);
    }
    
    // Reset index for next bin (allow overlap)
    data_idx = 0;
    while (data_idx < data.size() && data[data_idx].first < bin_high) {
      data_idx++;
    }
  }
  
  // Create TGraphErrors
  TGraphErrors* gr = new TGraphErrors(bin_centers.size(),
                                       bin_centers.data(),
                                       bin_y_avg.data(),
                                       bin_x_err.data(),
                                       bin_y_err.data());
  return gr;
}

// Helper function to extract clustering parameters from filename
std::map<std::string, int> extractClusteringParams(const std::string& filename) {
  std::map<std::string, int> params;
  params["tick"] = -1; params["ch"] = -1; params["min"] = -1; params["tot"] = -1;
  std::regex tick_regex("_tick(\\d+)"), ch_regex("_ch(\\d+)"), min_regex("_min(\\d+)"), tot_regex("_tot(\\d+)");
  std::smatch match;
  if (std::regex_search(filename, match, tick_regex) && match.size() > 1) params["tick"] = std::stoi(match[1].str());
  if (std::regex_search(filename, match, ch_regex) && match.size() > 1) params["ch"] = std::stoi(match[1].str());
  if (std::regex_search(filename, match, min_regex) && match.size() > 1) params["min"] = std::stoi(match[1].str());
  if (std::regex_search(filename, match, tot_regex) && match.size() > 1) params["tot"] = std::stoi(match[1].str());
  return params;
}

std::string formatClusteringConditions(const std::map<std::string, int>& params) {
  std::stringstream ss;
  bool first = true;
  if (params.at("ch") >= 0) { ss << "ch:" << params.at("ch"); first = false; }
  if (params.at("tick") >= 0) { if (!first) ss << ", "; ss << "tick:" << params.at("tick"); first = false; }
  if (params.at("tot") >= 0) { if (!first) ss << ", "; ss << "tot:" << params.at("tot"); first = false; }
  if (params.at("min") >= 0) { if (!first) ss << ", "; ss << "min:" << params.at("min"); }
  return ss.str();
}


int main(int argc, char* argv[]){
  // Enable ROOT batch mode to avoid GUI crashes
  gROOT->SetBatch(kTRUE);
  
  CmdLineParser clp;
  clp.getDescription() << "> analyze_clusters app - Generate plots from Cluster ROOT files." << std::endl;
  clp.addDummyOption("Main options");
  clp.addOption("json",    {"-j","--json"}, "JSON file containing the configuration");
  clp.addOption("inputFile", {"-i", "--input-file"}, "Input file with list OR single ROOT file path (overrides JSON inputs)");
  clp.addOption("outFolder", {"--output-folder"}, "Output folder path (optional)");
  clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");
  clp.addTriggerOption("debugMode", {"-d"}, "Run in debug mode (more detailed than verbose)");
  clp.addDummyOption();
  LogInfo << clp.getDescription().str() << std::endl;
  LogInfo << "Usage: " << std::endl;
  LogInfo << clp.getConfigSummary() << std::endl << std::endl;
  clp.parseCmdLine(argc, argv);
  LogThrowIf(clp.isNoOptionTriggered(), "No option was provided.");

  // verbosity updating global variables
  if (clp.isOptionTriggered("verboseMode")) verboseMode = true;
  if (clp.isOptionTriggered("debugMode")) { verboseMode =true; debugMode = true; }
  gErrorIgnoreLevel = rootVerbosity;

  // Load parameters
  ParametersManager::getInstance().loadParameters();


  // Check if tpstream files should be used for SimIDE energy calculation will be after JSON is loaded below

  // Avoid ROOT auto-ownership of histograms to prevent double deletes on TFile close
  TH1::AddDirectory(kFALSE);

  gStyle->SetOptStat(111111); // no stats box on plots


  std::string json = clp.getOptionVal<std::string>("json");
  std::ifstream jf(json);
  LogThrowIf(!jf.is_open(), "Could not open JSON: " << json);
  nlohmann::json j;
  jf >> j;
  // Check if tpstream files should be used for SimIDE energy calculation
  bool use_simide_energy = j.value("use_simide_energy", false);

  std::vector<std::string> inputs;
  // CLI first
  if (clp.isOptionTriggered("inputFile")) {
    std::string input_file = clp.getOptionVal<std::string>("inputFile");
    inputs.clear();
    // Accept any file containing _clusters and ending in .root
    std::regex clusters_regex(".*_clusters.*\\.root$");
    if (std::regex_match(input_file, clusters_regex)) {
      inputs.push_back(input_file);
    } else {
      std::ifstream lf(input_file);
      std::string line;
      while (std::getline(lf, line)) {
        if (!line.empty() && line[0] != '#') {
          if (std::regex_match(line, clusters_regex)) {
            inputs.push_back(line);
          }
        }
      }
    }
  }

  // Use utility function for file finding (clusters files)
  if (inputs.empty()) inputs = find_input_files(j, "clusters");

  LogInfo << "Number of valid files: " << inputs.size() << std::endl;
  LogThrowIf(inputs.empty(), "No valid input files found.");

  // Check if we should limit the number of files to process
  int max_files = j.value("max_files", -1);
  if (max_files > 0) {
    LogInfo << "Max files: " << max_files << std::endl;
  } else {
    LogInfo << "Max files: unlimited" << std::endl;
  }

  // Determine output folder: CLI > reports_folder > outputFolder
  std::string outFolder;
  if (clp.isOptionTriggered("outFolder"))
    outFolder = clp.getOptionVal<std::string>("outFolder");
  else if (j.contains("reports_folder"))
    outFolder = j.value("reports_folder", std::string(""));
  else if (j.contains("outputFolder"))
    outFolder = j.value("outputFolder", std::string(""));

  LogInfo << "Output folder: " << (outFolder.empty() ? "data" : outFolder) << std::endl;

  std::vector<std::string> produced;

  // SimIDE energy data structures (if enabled) - declared outside loop
  std::map<int, double> event_total_simide_energy_X; // sum of SimIDE energies per event (Collection plane X)
  std::map<int, double> event_total_simide_energy_U; // sum of SimIDE energies per event (U plane)
  std::map<int, double> event_total_simide_energy_V; // sum of SimIDE energies per event (V plane)
  std::vector<double> vec_total_simide_energy;
  std::vector<double> vec_total_simide_energy_U;
  std::vector<double> vec_total_simide_energy_V;
  std::vector<double> vec_total_cluster_charge_for_simide;
  std::vector<double> vec_total_cluster_charge_U_for_simide;
  std::vector<double> vec_total_cluster_charge_V_for_simide;

  // Histograms for ADC and energy by cluster family (global, accumulated across all files)
  TH1F* h_adc_pure_marley = new TH1F("h_adc_pure_marley", "Total ADC Integral: Pure Marley;Total ADC Integral;Clusters", 50, 0, 200000);
  TH1F* h_adc_pure_noise = new TH1F("h_adc_pure_noise", "Total ADC Integral: Pure Noise;Total ADC Integral;Clusters", 50, 0, 200000);
  TH1F* h_adc_hybrid = new TH1F("h_adc_hybrid", "Total ADC Integral: Hybrid (Marley+Noise);Total ADC Integral;Clusters", 50, 0, 200000);
  TH1F* h_adc_background = new TH1F("h_adc_background", "Total ADC Integral: Pure Background;Total ADC Integral;Clusters", 50, 0, 200000);
  TH1F* h_adc_mixed_signal_bkg = new TH1F("h_adc_mixed_signal_bkg", "Total ADC Integral: Mixed Marley+Background;Total ADC Integral;Clusters", 50, 0, 200000);
  h_adc_pure_marley->SetDirectory(nullptr);
  h_adc_pure_noise->SetDirectory(nullptr);
  h_adc_hybrid->SetDirectory(nullptr);
  h_adc_background->SetDirectory(nullptr);
  h_adc_mixed_signal_bkg->SetDirectory(nullptr);

  int bin_energy = 140, max_energy = 70;
  TH1F* h_energy_pure_marley = new TH1F("h_energy_pure_marley", "Total Energy: Pure Marley;Total Energy [MeV];Clusters", bin_energy, 0, max_energy);
  TH1F* h_energy_pure_noise = new TH1F("h_energy_pure_noise", "Total Energy: Pure Noise;Total Energy [MeV];Clusters", bin_energy, 0, max_energy);
  TH1F* h_energy_hybrid = new TH1F("h_energy_hybrid", "Total Energy: Hybrid (Marley+Noise);Total Energy [MeV];Clusters", bin_energy, 0, max_energy);
  TH1F* h_energy_background = new TH1F("h_energy_background", "Total Energy: Pure Background;Total Energy [MeV];Clusters", bin_energy, 0, max_energy);
  TH1F* h_energy_mixed_signal_bkg = new TH1F("h_energy_mixed_signal_bkg", "Total Energy: Mixed Marley+Background;Total Energy [MeV];Clusters", bin_energy, 0, max_energy);
  h_energy_pure_marley->SetDirectory(nullptr);
  h_energy_pure_noise->SetDirectory(nullptr);
  h_energy_hybrid->SetDirectory(nullptr);
  h_energy_background->SetDirectory(nullptr);
  h_energy_mixed_signal_bkg->SetDirectory(nullptr);

  // Additional global histograms for interesting quantities
  TH1F* h_true_particle_energy = new TH1F("h_true_part_e", "True particle energy;Energy [MeV];Clusters", 100, 0, 60000);
  h_true_particle_energy->SetDirectory(nullptr);
  TH1F* h_true_neutrino_energy = new TH1F("h_true_nu_e", "True neutrino energy;Energy [MeV];Clusters", 50, 0, 100);
  h_true_neutrino_energy->SetDirectory(nullptr);
  TH1F* h_min_distance = new TH1F("h_min_dist", "Min distance from true position;Distance [cm];Clusters", 50, 0, 50);
  h_min_distance->SetDirectory(nullptr);
  TH2F* h2_particle_vs_cluster_energy = new TH2F("h2_part_clust_e", "Particle energy vs cluster energy;True particle energy [MeV];Cluster total energy [MeV]", 100, 0, 70, 100, 0, 10000);
  h2_particle_vs_cluster_energy->SetDirectory(nullptr);
  TH2F* h2_neutrino_vs_cluster_energy = new TH2F("h2_nu_clust_e", "Neutrino energy vs cluster energy;True neutrino energy [MeV];Cluster total energy [MeV]", 50, 0, 100, 100, 0, 10000);
  h2_neutrino_vs_cluster_energy->SetDirectory(nullptr);
  TH2F* h2_total_particle_energy_vs_total_charge = new TH2F("h2_tot_part_e_vs_charge", "Total visible energy vs total cluster charge per event (All Planes);Total visible particle energy [MeV];Total cluster charge [ADC]", 100, 0, 70, 100, 0, 18000);
  h2_total_particle_energy_vs_total_charge->SetDirectory(nullptr);

  // Global accumulators (fill across all files, plot after loop)
  std::map<std::string, long long> label_counts_all;
  std::map<std::string, TH1F*> n_tps_plane_h;
  std::map<std::string, TH1F*> total_charge_plane_h;
  std::map<std::string, TH1F*> total_energy_plane_h;
  std::map<std::string, TH1F*> max_tp_charge_plane_h;
  std::map<std::string, TH1F*> total_length_plane_h;
  std::map<std::string, TH2F*> ntps_vs_total_charge_plane_h;
  std::map<std::string, double> min_cluster_charge;
  
  // MARLEY event accumulators
  std::vector<double> marley_enu;
  std::vector<double> marley_ncl;
  std::map<int, double> marley_total_energy_per_event;
  std::map<int, double> event_neutrino_energy;
  std::map<int, int> sn_clusters_per_event;
  
  // Per-event particle energy accumulators (per plane and global)
  std::map<int, double> event_total_particle_energy;      // X plane
  std::map<int, double> event_total_particle_energy_U;    // U plane
  std::map<int, double> event_total_particle_energy_V;    // V plane
  std::map<int, double> event_total_particle_energy_global; // Global across all planes
  
  // Per-event cluster charge accumulators
  std::map<int, double> event_total_cluster_charge;   // X plane
  std::map<int, double> event_total_cluster_charge_U; // U plane
  std::map<int, double> event_total_cluster_charge_V; // V plane
  
  // Track unique particles per event (per plane)
  std::map<int, std::set<std::tuple<float, float, float, float>>> event_unique_particles;        // X plane
  std::map<int, std::set<std::tuple<float, float, float, float>>> event_unique_particles_U;      // U plane
  std::map<int, std::set<std::tuple<float, float, float, float>>> event_unique_particles_V;      // V plane
  std::map<int, std::set<std::tuple<float, float, float, float>>> event_unique_particles_global; // Global
  
  // Vectors for calibration graphs
  std::vector<double> vec_total_particle_energy;
  std::vector<double> vec_total_cluster_charge;
  std::vector<double> vec_total_particle_energy_U;
  std::vector<double> vec_total_cluster_charge_U;
  std::vector<double> vec_total_particle_energy_V;
  std::vector<double> vec_total_cluster_charge_V;
  
  // Marley TP fraction categorization counters
  int only_marley_clusters = 0;
  int partial_marley_clusters = 0;
  int no_marley_clusters = 0;

  // Generate combined PDF path from first input file
  std::string combined_pdf;
  
  // Determine combined PDF output path
  std::string reports_folder = j.value("reports_folder", "data");
  // create reports folder if it does not exist
  std::filesystem::create_directories(reports_folder);
  std::string input_dir_name = "clusters";
  if (!inputs.empty()) {
    std::string first_file = inputs[0];
    size_t last_slash = first_file.find_last_of("/\\");
    if (last_slash != std::string::npos && last_slash > 0) {
      std::string parent_dir = first_file.substr(0, last_slash);
      size_t parent_last_slash = parent_dir.find_last_of("/\\");
      if (parent_last_slash != std::string::npos) {
        input_dir_name = parent_dir.substr(parent_last_slash + 1);
      } else {
        input_dir_name = parent_dir;
      }
    }
  }
  combined_pdf = reports_folder + "/" + input_dir_name + "_report.pdf";
  int file_count = 0;
  for (const auto& clusters_file : inputs){

    file_count++;
    if (max_files > 0 && file_count > max_files) {
      LogInfo << "Reached max_files limit (" << max_files << "), stopping." << std::endl;
      break;
    }
    LogInfo << "Input clusters file: " << clusters_file << std::endl;
    // Read clusters per plane; the file may contain multiple trees (U/V/X)
    TFile* f = TFile::Open(clusters_file.c_str());
    if (!f || f->IsZombie()){
      LogError << "Cannot open: " << clusters_file << std::endl;
      if(f){f->Close(); delete f;}
      continue;
    }

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
          std::string plane="";
          auto pos=tname.find_last_of('_');
          if(pos!=std::string::npos) plane=tname.substr(pos+1);
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
    if(planes.empty()){
      LogError << "No clusters trees found in file: " << clusters_file << std::endl;
      f->Close(); delete f;
      continue;
    }

    // Charge to energy conversion factors (ADC/MeV)
    const double ADC_TO_MEV_COLLECTION = ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_collection");
    const double ADC_TO_MEV_INDUCTION = ParametersManager::getInstance().getDouble("conversion.adc_to_energy_factor_induction");
    const double ADC_TO_MEV_X = ADC_TO_MEV_COLLECTION;
    const double ADC_TO_MEV_U = ADC_TO_MEV_INDUCTION;
    const double ADC_TO_MEV_V = ADC_TO_MEV_INDUCTION;

    // Helper to ensure per-plane histograms exist
    auto ensureHist = [&](std::map<std::string, TH1F*>& m, const std::string& key, const char* title, int nbins=100, double xmin=0, double xmax=100){
      if (m.count(key)==0){
        m[key] = new TH1F((key+"_h").c_str(), title, nbins, xmin, xmax);
        m[key]->SetStats(0);
        m[key]->SetDirectory(nullptr);
      }
      return m[key];
    };
    auto ensureHist2D = [&](std::map<std::string, TH2F*>& m, const std::string& key, const char* title){
      if (m.count(key)==0){
        m[key] = new TH2F((key+"_h2").c_str(), title, 60, 0, 60, 100, 0, 300); // Changed from 100,0,100,100,0,1e6 to 60,0,60,100,0,300
        m[key]->SetDirectory(nullptr);
      }
      return m[key];
    };

    // Iterate planes
    for (auto& pd : planes){
      // Branches
      int event=0, n_tps=0;
      float true_ne=0, true_pe=0;
      double total_charge=0, total_energy=0;
      float min_dist=0;
      std::string* true_label_ptr=nullptr;
      float true_dir_x=0, true_dir_y=0, true_dir_z=0;
      float true_pos_x=0, true_pos_y=0, true_pos_z=0;
      std::string* true_interaction_ptr=nullptr;
      float supernova_tp_fraction=0.f, generator_tp_fraction=0.f, marley_tp_fraction=0.f;
      bool has_marley_tp_fraction_branch = false; // Track if new branch exists
      // vector branches for derived quantities
      std::vector<int>* v_chan=nullptr;
      std::vector<int>* v_tstart=nullptr;
      std::vector<int>* v_sot=nullptr;
      std::vector<int>* v_adcint=nullptr;
      std::vector<double>* v_simide_energy=nullptr;
      // match_id branches (for matched_clusters files)
      int match_id=-1, match_type=-1;
      bool has_match_id = false;
      
      pd.tree->SetBranchAddress("event", &event);
      pd.tree->SetBranchAddress("n_tps", &n_tps);
      // prefer new names; fallback if needed
      if (pd.tree->GetBranch("true_neutrino_energy")) pd.tree->SetBranchAddress("true_neutrino_energy", &true_ne);
      else if (pd.tree->GetBranch("true_energy")) pd.tree->SetBranchAddress("true_energy", &true_ne);
      if (pd.tree->GetBranch("true_particle_energy")) pd.tree->SetBranchAddress("true_particle_energy", &true_pe);
      if (pd.tree->GetBranch("min_distance_from_true_pos")) pd.tree->SetBranchAddress("min_distance_from_true_pos", &min_dist);
      if (pd.tree->GetBranch("total_charge")) pd.tree->SetBranchAddress("total_charge", &total_charge);
      if (pd.tree->GetBranch("total_energy")) pd.tree->SetBranchAddress("total_energy", &total_energy);
      if (pd.tree->GetBranch("true_label")) pd.tree->SetBranchAddress("true_label", &true_label_ptr);
      if (pd.tree->GetBranch("true_interaction")) pd.tree->SetBranchAddress("true_interaction", &true_interaction_ptr);
      if (pd.tree->GetBranch("supernova_tp_fraction")) pd.tree->SetBranchAddress("supernova_tp_fraction", &supernova_tp_fraction);
      if (pd.tree->GetBranch("generator_tp_fraction")) pd.tree->SetBranchAddress("generator_tp_fraction", &generator_tp_fraction);
      if (pd.tree->GetBranch("marley_tp_fraction")) {
        pd.tree->SetBranchAddress("marley_tp_fraction", &marley_tp_fraction);
        has_marley_tp_fraction_branch = true;
      }
      if (pd.tree->GetBranch("match_id")) {
        pd.tree->SetBranchAddress("match_id", &match_id);
        has_match_id = true;
      }
      if (pd.tree->GetBranch("match_type")) {
        pd.tree->SetBranchAddress("match_type", &match_type);
      }
      if (pd.tree->GetBranch("true_pos_x")) pd.tree->SetBranchAddress("true_pos_x", &true_pos_x);
      if (pd.tree->GetBranch("true_pos_y")) pd.tree->SetBranchAddress("true_pos_y", &true_pos_y);
      if (pd.tree->GetBranch("true_pos_z")) pd.tree->SetBranchAddress("true_pos_z", &true_pos_z);
      // vectors
      if (pd.tree->GetBranch("tp_detector_channel")) pd.tree->SetBranchAddress("tp_detector_channel", &v_chan);
      if (pd.tree->GetBranch("tp_time_start")) pd.tree->SetBranchAddress("tp_time_start", &v_tstart);
      if (pd.tree->GetBranch("tp_samples_over_threshold")) pd.tree->SetBranchAddress("tp_samples_over_threshold", &v_sot);
      if (pd.tree->GetBranch("tp_adc_integral")) pd.tree->SetBranchAddress("tp_adc_integral", &v_adcint);
      if (pd.tree->GetBranch("tp_simide_energy")) pd.tree->SetBranchAddress("tp_simide_energy", &v_simide_energy);

      // Report if this is a matched_clusters file
      if (has_match_id && pd.name == planes[0].name) { // Only log once per file
        LogInfo << "  File contains match_id information (matched_clusters file)" << std::endl;
      }

      // Histos per plane
      TH1F* h_ntps = ensureHist(n_tps_plane_h, std::string("n_tps_")+pd.name, (std::string("Cluster size (n_tps) - ")+pd.name+";n_{TPs};Clusters").c_str(), 40, 0, 40);
      TH1F* h_charge = ensureHist(total_charge_plane_h, std::string("total_charge_")+pd.name, (std::string("Total charge - ")+pd.name+";Total charge [ADC];Clusters").c_str(), 100, 0, 300);
      TH1F* h_energy = ensureHist(total_energy_plane_h, std::string("total_energy_")+pd.name, (std::string("Total energy - ")+pd.name+";Total energy [MeV];Clusters").c_str(), 100, 0, 1000);
      TH1F* h_maxadc = ensureHist(max_tp_charge_plane_h, std::string("max_tp_charge_")+pd.name, (std::string("Max TP adc_integral - ")+pd.name+";Max ADC integral;Clusters").c_str(), 100, 0, 1000);
      TH1F* h_tlen = ensureHist(total_length_plane_h, std::string("total_length_")+pd.name, (std::string("Total length (cm) - ")+pd.name+";Length [cm];Clusters").c_str(), 100, 0, 1000);
      TH2F* h2_ntps_charge = ensureHist2D(ntps_vs_total_charge_plane_h, std::string("ntps_vs_charge_")+pd.name, (std::string("n_{TPs} vs total charge - ")+pd.name+";n_{TPs};Total charge [ADC]").c_str());

      // Per-event MARLEY clustering tracking
      std::map<int,long long> marley_count_evt;
      std::map<int,double> evt_enu; // per-event neutrino energy (if available)

      Long64_t n = pd.tree->GetEntries();
      for (Long64_t i=0;i<n;++i){
        pd.tree->GetEntry(i);
        if (n_tps<=0) continue;
        
        // Track minimum cluster charge per plane
        if (min_cluster_charge.count(pd.name) == 0 || total_charge < min_cluster_charge[pd.name]) {
          min_cluster_charge[pd.name] = total_charge;
        }
        
        if (true_label_ptr){
          label_counts_all[*true_label_ptr]++;
          if (toLower(*true_label_ptr).find("marley")!=std::string::npos){
            marley_count_evt[event]++;
            marley_total_energy_per_event[event] += total_energy;
          }
        }
        h_ntps->Fill(n_tps);
        h_charge->Fill(total_charge);
        h_energy->Fill(total_energy);
        
        // Fill global histograms
        if (true_pe > 0) h_true_particle_energy->Fill(true_pe);
        if (true_ne > 0) h_true_neutrino_energy->Fill(true_ne);
        if (min_dist >= 0) h_min_distance->Fill(min_dist);
        if (true_pe > 0 && total_energy > 0) h2_particle_vs_cluster_energy->Fill(true_pe, total_energy);
        if (true_ne > 0 && total_energy > 0) h2_neutrino_vs_cluster_energy->Fill(true_ne, total_energy);
        
        // record event energy when available
        if (true_ne>0) {
          evt_enu[event] = true_ne;
          event_neutrino_energy[event] = true_ne;
        }
        
        // Track unique particles and accumulate their energies (excluding neutrinos) - per plane
        // Use particle energy and position to identify unique particles
        if (true_pe > 0) {
          // Create a unique identifier for this particle using energy and position
          auto particle_id = std::make_tuple(true_pe, true_pos_x, true_pos_y, true_pos_z);
          // Insert returns pair<iterator, bool> where bool is true if insertion happened
          if (pd.name == "X") {
            if (event_unique_particles[event].insert(particle_id).second) {
              // This is a new unique particle for X plane, add its energy
              event_total_particle_energy[event] += true_pe;
            }
          } else if (pd.name == "U") {
            if (event_unique_particles_U[event].insert(particle_id).second) {
              // This is a new unique particle for U plane, add its energy
              event_total_particle_energy_U[event] += true_pe;
            }
          } else if (pd.name == "V") {
            if (event_unique_particles_V[event].insert(particle_id).second) {
              // This is a new unique particle for V plane, add its energy
              event_total_particle_energy_V[event] += true_pe;
            }
          }
          // Also track globally across all planes (for per-event total plot)
          if (event_unique_particles_global[event].insert(particle_id).second) {
            // This is a new unique particle globally, add its energy (no double counting)
            event_total_particle_energy_global[event] += true_pe;
          }
        }
        
        // Accumulate per-event total cluster charge per plane
        if (pd.name == "X") {
          event_total_cluster_charge[event] += total_charge;
        } else if (pd.name == "U") {
          event_total_cluster_charge_U[event] += total_charge;
        } else if (pd.name == "V") {
          event_total_cluster_charge_V[event] += total_charge;
        }

        // Accumulate per-event total SimIDE energy per plane (from tp_simide_energy branch)
        if (use_simide_energy && v_simide_energy && !v_simide_energy->empty()) {
          double simide_energy_sum = 0.0;
          for (auto e : *v_simide_energy) simide_energy_sum += e;
          if (pd.name == "X") {
            event_total_simide_energy_X[event] += simide_energy_sum;
          } else if (pd.name == "U") {
            event_total_simide_energy_U[event] += simide_energy_sum;
          } else if (pd.name == "V") {
            event_total_simide_energy_V[event] += simide_energy_sum;
          }
        }

        // Categorize clusters by Marley TP content
        // Use marley_tp_fraction if available, otherwise fall back to generator_tp_fraction (old behavior)
        float marley_frac = has_marley_tp_fraction_branch ? marley_tp_fraction : generator_tp_fraction;
        if (marley_frac == 1.0f) {
          only_marley_clusters++;
        } else if (marley_frac > 0.0f && marley_frac < 1.0f) {
          partial_marley_clusters++;
        } else if (marley_frac == 0.0f) {
          no_marley_clusters++;
        }

        // --- Fill ADC integral histograms by cluster family ---
        // We need to look at the TP-level generator information to classify properly
        // Use the v_adcint vector to compute total ADC integral
        if (v_adcint && !v_adcint->empty()) {
          double adc_sum = 0;
          for (auto val : *v_adcint) adc_sum += val;
          
          // Convert ADC to energy based on plane
          double adc_to_mev = ADC_TO_MEV_X; // default to collection
          if (pd.name == "U") adc_to_mev = ADC_TO_MEV_U;
          else if (pd.name == "V") adc_to_mev = ADC_TO_MEV_V;
          double energy_sum = adc_sum / adc_to_mev;
          
          if (has_marley_tp_fraction_branch) {
            // NEW BEHAVIOR: Use both marley_tp_fraction and generator_tp_fraction
            // marley_tp_fraction: fraction of TPs with generator == "marley"
            // generator_tp_fraction: fraction of TPs with generator != "UNKNOWN" (includes marley + backgrounds)
            
            if (marley_tp_fraction == 1.0f) {
              // Pure marley (all TPs are marley)
              h_adc_pure_marley->Fill(adc_sum);
              if (pd.name == "X") h_energy_pure_marley->Fill(energy_sum);
            } else if (marley_tp_fraction == 0.0f && generator_tp_fraction == 0.0f) {
              // Pure noise (no generator info at all)
              h_adc_pure_noise->Fill(adc_sum);
              if (pd.name == "X") h_energy_pure_noise->Fill(energy_sum);
            } else if (marley_tp_fraction == 0.0f && generator_tp_fraction > 0.0f) {
              // Pure background (has generator info, but no marley)
              h_adc_background->Fill(adc_sum);
              if (pd.name == "X") h_energy_background->Fill(energy_sum);
            } else if (marley_tp_fraction > 0.0f && marley_tp_fraction < 1.0f) {
              // Has some marley TPs
              if (generator_tp_fraction == marley_tp_fraction) {
                // Only marley and UNKNOWN (no backgrounds) - Hybrid marley+noise
                h_adc_hybrid->Fill(adc_sum);
                if (pd.name == "X") h_energy_hybrid->Fill(energy_sum);
              } else {
                // Has marley + backgrounds (and possibly UNKNOWN) - Mixed signal+background
                h_adc_mixed_signal_bkg->Fill(adc_sum);
                if (pd.name == "X") h_energy_mixed_signal_bkg->Fill(energy_sum);
              }
            }
          } else {
            // OLD BEHAVIOR: Fall back to using generator_tp_fraction and true_label
            // This is for backward compatibility with old cluster files
            
            if (generator_tp_fraction == 1.0f) {
              // Assume pure marley if all TPs have generator info
              h_adc_pure_marley->Fill(adc_sum);
              if (pd.name == "X") h_energy_pure_marley->Fill(energy_sum);
            } else if (generator_tp_fraction == 0.0f) {
              // Pure noise (no generator info)
              h_adc_pure_noise->Fill(adc_sum);
              if (pd.name == "X") h_energy_pure_noise->Fill(energy_sum);
            } else if (generator_tp_fraction > 0.0f && generator_tp_fraction < 1.0f) {
              // Hybrid (mix of with-generator and without-generator)
              h_adc_hybrid->Fill(adc_sum);
              if (pd.name == "X") h_energy_hybrid->Fill(energy_sum);
            }
            
            // Check if it's a background (not marley, not unknown)
            if (true_label_ptr && *true_label_ptr != "marley" && *true_label_ptr != "UNKNOWN" && !true_label_ptr->empty()) {
              h_adc_background->Fill(adc_sum);
              if (pd.name == "X") h_energy_background->Fill(energy_sum);
            }
          }
        }

        // derived variables from vectors
        if (v_adcint && !v_adcint->empty()){
          int max_adc = *std::max_element(v_adcint->begin(), v_adcint->end());
          h_maxadc->Fill(max_adc);
        }
        if (v_tstart && v_sot && v_chan && !v_tstart->empty()){
          int tmin = *std::min_element(v_tstart->begin(), v_tstart->end());
          int tmax = tmin;
          for (size_t k=0;k<v_tstart->size();++k){
            int tend = v_tstart->at(k) + (v_sot && k<v_sot->size()? v_sot->at(k) : 0);
            if (tend>tmax) tmax=tend;
          }
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
        double enu = 0.0;
        auto it = evt_enu.find(kv.first);
        if (it != evt_enu.end()) enu = it->second;
        marley_enu.push_back(enu);
        marley_ncl.push_back((double)kv.second);
      }
    }
    
    // Debug: Print summary of cluster categorization
    if (verboseMode) {
      LogInfo << "\n=== Cluster Categorization Summary ===" << std::endl;
      LogInfo << "Pure Marley clusters: " << h_adc_pure_marley->GetEntries() << std::endl;
      LogInfo << "Pure Noise clusters: " << h_adc_pure_noise->GetEntries() << std::endl;
      LogInfo << "Marley+Noise (hybrid) clusters: " << h_adc_hybrid->GetEntries() << std::endl;
      LogInfo << "Pure Background clusters: " << h_adc_background->GetEntries() << std::endl;
      LogInfo << "Marley+Background (mixed) clusters: " << h_adc_mixed_signal_bkg->GetEntries() << std::endl;
      LogInfo << "======================================\n" << std::endl;
    }
    
    // After processing all planes, prepare data for the calibration graph
    // Collection plane (X)
    for (const auto& kv : event_total_particle_energy) {
      int evt = kv.first;
      double tot_part_e = kv.second;
      auto it = event_total_cluster_charge.find(evt);
      if (it != event_total_cluster_charge.end()) {
        double tot_charge = it->second;
        vec_total_particle_energy.push_back(tot_part_e);
        vec_total_cluster_charge.push_back(tot_charge);
      }
    }
    
    // Fill per-event total histogram (global across all planes, no double counting)
    for (const auto& kv : event_total_particle_energy_global) {
      int evt = kv.first;
      double tot_part_e_global = kv.second;
      // Sum charges across all planes for this event
      double tot_charge_all_planes = 0;
      auto it_X = event_total_cluster_charge.find(evt);
      auto it_U = event_total_cluster_charge_U.find(evt);
      auto it_V = event_total_cluster_charge_V.find(evt);
      if (it_X != event_total_cluster_charge.end()) tot_charge_all_planes += it_X->second;
      if (it_U != event_total_cluster_charge_U.end()) tot_charge_all_planes += it_U->second;
      if (it_V != event_total_cluster_charge_V.end()) tot_charge_all_planes += it_V->second;
      
      if (tot_charge_all_planes > 0) {
        h2_total_particle_energy_vs_total_charge->Fill(tot_part_e_global, tot_charge_all_planes);
      }
    }
    // U plane
    for (const auto& kv : event_total_particle_energy_U) {
      int evt = kv.first;
      double tot_part_e = kv.second;
      auto it = event_total_cluster_charge_U.find(evt);
      if (it != event_total_cluster_charge_U.end()) {
        double tot_charge = it->second;
        vec_total_particle_energy_U.push_back(tot_part_e);
        vec_total_cluster_charge_U.push_back(tot_charge);
      }
    }
    // V plane
    for (const auto& kv : event_total_particle_energy_V) {
      int evt = kv.first;
      double tot_part_e = kv.second;
      auto it = event_total_cluster_charge_V.find(evt);
      if (it != event_total_cluster_charge_V.end()) {
        double tot_charge = it->second;
        vec_total_particle_energy_V.push_back(tot_part_e);
        vec_total_cluster_charge_V.push_back(tot_charge);
      }
    }
    
    // Prepare SimIDE energy vectors if available
    if (use_simide_energy && !event_total_simide_energy_X.empty()) {
      for (const auto& kv : event_total_simide_energy_X) {
        int evt = kv.first;
        double simide_e = kv.second;
        auto it = event_total_cluster_charge.find(evt);
        if (it != event_total_cluster_charge.end()) {
          vec_total_simide_energy.push_back(simide_e);
          vec_total_cluster_charge_for_simide.push_back(it->second);
        }
      }
      for (const auto& kv : event_total_simide_energy_U) {
        int evt = kv.first;
        double simide_e = kv.second;
        auto it = event_total_cluster_charge_U.find(evt);
        if (it != event_total_cluster_charge_U.end()) {
          vec_total_simide_energy_U.push_back(simide_e);
          vec_total_cluster_charge_U_for_simide.push_back(it->second);
        }
      }
      for (const auto& kv : event_total_simide_energy_V) {
        int evt = kv.first;
        double simide_e = kv.second;
        auto it = event_total_cluster_charge_V.find(evt);
        if (it != event_total_cluster_charge_V.end()) {
          vec_total_simide_energy_V.push_back(simide_e);
          vec_total_cluster_charge_V_for_simide.push_back(it->second);
        }
      }
      
      if (verboseMode && !vec_total_simide_energy.empty()) {
        LogInfo << "SimIDE energy data (X): " << vec_total_simide_energy.size() << " events" << std::endl;
        LogInfo << "SimIDE energy range: " << *std::min_element(vec_total_simide_energy.begin(), vec_total_simide_energy.end())
                << " - " << *std::max_element(vec_total_simide_energy.begin(), vec_total_simide_energy.end()) << " MeV" << std::endl;
      }
    }
    
    // Print some debug info
    if (verboseMode && !vec_total_particle_energy.empty()) {
      LogInfo << "Calibration data: " << vec_total_particle_energy.size() << " events" << std::endl;
      LogInfo << "Energy range: " << *std::min_element(vec_total_particle_energy.begin(), vec_total_particle_energy.end())
              << " - " << *std::max_element(vec_total_particle_energy.begin(), vec_total_particle_energy.end()) << " MeV" << std::endl;
      LogInfo << "Charge range: " << *std::min_element(vec_total_cluster_charge.begin(), vec_total_cluster_charge.end())
              << " - " << *std::max_element(vec_total_cluster_charge.begin(), vec_total_cluster_charge.end()) << " ADC" << std::endl;
    }

    // Close the file - data accumulation complete for this file
    f->Close();
    delete f;
  } // End of file loop

  // ============================================================================
  // GENERATE PLOTS FROM ACCUMULATED DATA (after processing all files)
  // ============================================================================
  LogInfo << "Generating combined analysis report from " << file_count << " file(s)..." << std::endl;

  // Start PDF with title page
  int pageNum = 1;
  int totalPages = use_simide_energy ? 23 : 20; // Updated: +3 for SimIDE, +3 for per-view energy histograms
  
  std::string pdf = combined_pdf;
  
  TCanvas* c = new TCanvas("c_ac_title","Title",800,600); c->cd();
  c->SetFillColor(kWhite);
  auto t = new TText(0.5,0.85, "Cluster Analysis Report");
  t->SetTextAlign(22); t->SetTextSize(0.06); t->SetNDC(); t->Draw();
  
  // Show directory and list of all input files
  if (!inputs.empty()) {
    std::string first_file = inputs[0];
    std::string path_part = first_file.substr(0, first_file.find_last_of("/\\")+1);
    auto ptxt = new TText(0.5,0.70, Form("Directory: %s", path_part.c_str()));
    ptxt->SetTextAlign(22); ptxt->SetTextSize(0.025); ptxt->SetNDC(); ptxt->Draw();
    
    float ypos = 0.62;
    auto ftxt = new TText(0.5, ypos, Form("Combined analysis of %d file(s):", (int)inputs.size()));
    ftxt->SetTextAlign(22); ftxt->SetTextSize(0.025); ftxt->SetNDC(); ftxt->Draw();
    ypos -= 0.035;
    
    // List input files (limit to first 15 to avoid overflow)
    int max_files_to_show = 15;
    for (int i = 0; i < std::min((int)inputs.size(), max_files_to_show); i++) {
      std::string file_part = inputs[i].substr(inputs[i].find_last_of("/\\")+1);
      auto entry_txt = new TText(0.5, ypos, Form("  %s", file_part.c_str()));
      entry_txt->SetTextAlign(22); entry_txt->SetTextSize(0.018); entry_txt->SetNDC(); entry_txt->Draw();
      ypos -= 0.024;
    }
    if ((int)inputs.size() > max_files_to_show) {
      auto more_txt = new TText(0.5, ypos, Form("  ... and %d more files", (int)inputs.size() - max_files_to_show));
      more_txt->SetTextAlign(22); more_txt->SetTextSize(0.018); more_txt->SetNDC(); more_txt->Draw();
      ypos -= 0.024;
    }
  }
  
  // Extract clustering parameters from first filename
  std::string file_part = inputs.empty() ? "" : inputs[0].substr(inputs[0].find_last_of("/\\")+1);
  std::string params_str = "";
  size_t pos = file_part.find("tick");
    if (pos != std::string::npos) {
      size_t end_pos = file_part.find("_clusters");
      if (end_pos != std::string::npos) {
        params_str = file_part.substr(pos, end_pos - pos);
        // Parse parameters
        auto param_txt = new TText(0.5, 0.50, "Clustering Parameters:");
        param_txt->SetTextAlign(22); param_txt->SetTextSize(0.03); param_txt->SetNDC(); param_txt->SetTextFont(62); param_txt->Draw();
        
        // Extract values
        std::string tick_val, ch_val, min_val, tot_val;
        std::stringstream ss(params_str);
        std::string token;
        while (std::getline(ss, token, '_')) {
          if (token.find("tick") == 0) tick_val = token.substr(4);
          else if (token.find("ch") == 0) ch_val = token.substr(2);
          else if (token.find("min") == 0) min_val = token.substr(3);
          else if (token.find("tot") == 0) tot_val = token.substr(3);
        }
        
        float ypos = 0.44;
        if (!tick_val.empty()) {
          auto txt = new TText(0.5, ypos, Form("Time tolerance: %s ticks", tick_val.c_str()));
          txt->SetTextAlign(22); txt->SetTextSize(0.025); txt->SetNDC(); txt->Draw();
          ypos -= 0.04;
        }
        if (!ch_val.empty()) {
          auto txt = new TText(0.5, ypos, Form("Channel tolerance: %s", ch_val.c_str()));
          txt->SetTextAlign(22); txt->SetTextSize(0.025); txt->SetNDC(); txt->Draw();
          ypos -= 0.04;
        }
        if (!min_val.empty()) {
          auto txt = new TText(0.5, ypos, Form("Min cluster size: %s TPs", min_val.c_str()));
          txt->SetTextAlign(22); txt->SetTextSize(0.025); txt->SetNDC(); txt->Draw();
          ypos -= 0.04;
        }
        if (!tot_val.empty()) {
          auto txt = new TText(0.5, ypos, Form("Min TOT threshold: %s samples (time over threshold)", tot_val.c_str()));
          txt->SetTextAlign(22); txt->SetTextSize(0.025); txt->SetNDC(); txt->Draw();
        }
      }
    }
    
    // Display minimum cluster charges per plane
    if (!min_cluster_charge.empty()) {
      float ypos = 0.25;
      auto min_charge_txt = new TText(0.5, ypos, "Minimum Cluster Charge:");
      min_charge_txt->SetTextAlign(22); min_charge_txt->SetTextSize(0.03); min_charge_txt->SetNDC(); min_charge_txt->SetTextFont(62); min_charge_txt->Draw();
      ypos -= 0.04;
      
      if (min_cluster_charge.count("U")) {
        auto txt = new TText(0.5, ypos, Form("Induction U: %.1f ADC", min_cluster_charge["U"]));
        txt->SetTextAlign(22); txt->SetTextSize(0.025); txt->SetNDC(); txt->Draw();
        ypos -= 0.03;
      }
      if (min_cluster_charge.count("V")) {
        auto txt = new TText(0.5, ypos, Form("Induction V: %.1f ADC", min_cluster_charge["V"]));
        txt->SetTextAlign(22); txt->SetTextSize(0.025); txt->SetNDC(); txt->Draw();
        ypos -= 0.03;
      }
      if (min_cluster_charge.count("X")) {
        auto txt = new TText(0.5, ypos, Form("Collection X: %.1f ADC", min_cluster_charge["X"]));
        txt->SetTextAlign(22); txt->SetTextSize(0.025); txt->SetNDC(); txt->Draw();
      }
    }
    
    // Add timestamp
    TDatime now;
    auto date_txt = new TText(0.2, 0.15, Form("Generated on: %s", now.AsString()));
    date_txt->SetTextAlign(22); date_txt->SetTextSize(0.02); date_txt->SetNDC(); date_txt->Draw();
    
    c->SaveAs((pdf+"(").c_str());
    delete c;

    // Page: counts by label (HBAR)
    if (!label_counts_all.empty()) {
      pageNum++;
      size_t nlabels = label_counts_all.size();
      TCanvas* cl = new TCanvas("c_ac_labels","Labels",1200,700);
      TH1F* h = new TH1F("h_ac_labels","Clusters by true label", std::max<size_t>(1,nlabels), 0, std::max<size_t>(1,nlabels));
      h->SetOption("HBAR"); h->SetStats(0); h->SetYTitle("");
      cl->SetLeftMargin(0.30); cl->SetLogx(); cl->SetGridx(); cl->SetGridy();
      int b=1;
      for (const auto& kv : label_counts_all){
        h->SetBinContent(b, kv.second);
        h->GetXaxis()->SetBinLabel(b, kv.first.c_str());
        b++;
      }
      h->Draw("HBAR");
      addPageNumber(cl, pageNum, totalPages);
      cl->SaveAs(pdf.c_str());
      delete h;
      delete cl;
    }

    // Page: per-plane Cluster size and totals
    if (!n_tps_plane_h.empty()) {
      pageNum++;
      TCanvas* csz = new TCanvas("c_ac_sizes","Sizes",1200,900); csz->Divide(3,2);
      int pad=1;
      // Use actual plane names from the map
      std::vector<std::string> plane_order = {"U", "V", "X"};
      for (const auto& p : plane_order) {
        csz->cd(pad++);
        if(n_tps_plane_h.count(std::string("n_tps_")+p) && n_tps_plane_h[std::string("n_tps_")+p]){ 
          gPad->SetLogy(); 
          n_tps_plane_h[std::string("n_tps_")+p]->Draw("HIST"); 
        }
      }
      for (const auto& p : plane_order) {
        csz->cd(pad++);
        if(total_charge_plane_h.count(std::string("total_charge_")+p) && total_charge_plane_h[std::string("total_charge_")+p]){ 
          gPad->SetLogy(); 
          total_charge_plane_h[std::string("total_charge_")+p]->Draw("HIST"); 
        }
      }
      csz->cd(0);
      addPageNumber(csz, pageNum, totalPages);
      csz->SaveAs(pdf.c_str());
      delete csz;
      
      pageNum++;
      TCanvas* cen = new TCanvas("c_ac_energy","Energy",1200,500); cen->Divide(3,1);
      int p2=1;
      for (const auto& p : plane_order) {
        cen->cd(p2++);
        if(total_energy_plane_h.count(std::string("total_energy_")+p) && total_energy_plane_h[std::string("total_energy_")+p]){ 
          gPad->SetLogy(); 
          total_energy_plane_h[std::string("total_energy_")+p]->Draw("HIST"); 
        }
      }
      cen->cd(0);
      addPageNumber(cen, pageNum, totalPages);
      cen->SaveAs(pdf.c_str());
      delete cen;
    }

    // Page: per-plane max TP charge and total length
    if (!max_tp_charge_plane_h.empty()) {
      pageNum++;
      TCanvas* cmax = new TCanvas("c_ac_max","Max and length",1200,900); cmax->Divide(3,2);
      int pad=1;
      std::vector<std::string> plane_order = {"U", "V", "X"};
      for (const auto& p : plane_order) {
        cmax->cd(pad++);
        if(max_tp_charge_plane_h.count(std::string("max_tp_charge_")+p) && max_tp_charge_plane_h[std::string("max_tp_charge_")+p]){ 
          gPad->SetLogy(); 
          max_tp_charge_plane_h[std::string("max_tp_charge_")+p]->Draw("HIST"); 
        }
      }
      for (const auto& p : plane_order) {
        cmax->cd(pad++);
        if(total_length_plane_h.count(std::string("total_length_")+p) && total_length_plane_h[std::string("total_length_")+p]){ 
          gPad->SetLogy(); 
          total_length_plane_h[std::string("total_length_")+p]->Draw("HIST"); 
        }
      }
      cmax->cd(0);
      addPageNumber(cmax, pageNum, totalPages);
      cmax->SaveAs(pdf.c_str());
      delete cmax;
    }

    // Page: per-plane 2D n_tps vs total charge
    if (!ntps_vs_total_charge_plane_h.empty()) {
      pageNum++;
      TCanvas* c2d = new TCanvas("c_ac_2d","nTPs vs charge",1200,500); c2d->Divide(3,1);
      int pad=1;
      std::vector<std::string> plane_order = {"U", "V", "X"};
      for (const auto& p : plane_order) {
        c2d->cd(pad++);
        if(ntps_vs_total_charge_plane_h.count(std::string("ntps_vs_charge_")+p) && ntps_vs_total_charge_plane_h[std::string("ntps_vs_charge_")+p]){ 
          gPad->SetRightMargin(0.12); 
          ntps_vs_total_charge_plane_h[std::string("ntps_vs_charge_")+p]->SetContour(100); 
          ntps_vs_total_charge_plane_h[std::string("ntps_vs_charge_")+p]->Draw("COLZ"); 
        }
      }
      c2d->cd(0);
      addPageNumber(c2d, pageNum, totalPages);
      c2d->SaveAs(pdf.c_str());
      delete c2d;
    }

    // Page: number of supernova clusters per event
    if (!sn_clusters_per_event.empty()){
      pageNum++;
      int maxv = 0;
      for (const auto& kv : sn_clusters_per_event) maxv = std::max(maxv, kv.second);
      maxv = std::min(maxv, 40); // Cap at 40 for better visualization
      TCanvas* csn = new TCanvas("c_ac_sn","SN clusters per event",900,600);
      TH1F* hsn = new TH1F("h_ac_sn","Supernova clusters per event;Cluster count;Events", maxv+1, -0.5, maxv+0.5);
      for (const auto& kv : sn_clusters_per_event) {
        if (kv.second <= maxv) hsn->Fill(kv.second);
      }
      hsn->Draw("HIST");
      addPageNumber(csn, pageNum, totalPages);
      csn->SaveAs(pdf.c_str());
      delete hsn;
      delete csn;
    }

    // Page: MARLEY clusters per event vs neutrino energy (scatter)
    if (!marley_enu.empty()){
      pageNum++;
      TCanvas* csc = new TCanvas("c_ac_scatter","MARLEY clusters vs E#nu",900,600);
      TGraph* gr = new TGraph((int)marley_enu.size(), marley_enu.data(), marley_ncl.data());
      gr->SetTitle("MARLEY clusters vs E_{#nu};E_{#nu} [MeV];N_{clusters} (MARLEY)");
      gr->SetMarkerStyle(20);
      gr->SetMarkerColor(kRed+1);
      gr->Draw("AP");
      gPad->SetGridx();
      gPad->SetGridy();
      addPageNumber(csc, pageNum, totalPages);
      csc->SaveAs(pdf.c_str());
      delete gr;
      delete csc;
    }

    // Page: Total energy of MARLEY clusters per event vs neutrino energy
    if (!marley_total_energy_per_event.empty() && !event_neutrino_energy.empty()){
      pageNum++;
      std::vector<double> enu_vec, energy_vec;
      for (const auto& kv : marley_total_energy_per_event) {
        int evt = kv.first;
        double tot_e = kv.second;
        auto it = event_neutrino_energy.find(evt);
        if (it != event_neutrino_energy.end()) {
          enu_vec.push_back(it->second);
          energy_vec.push_back(tot_e);
        }
      }
      if (!enu_vec.empty()) {
        TCanvas* cen_scatter = new TCanvas("c_ac_energy_scatter","MARLEY total energy vs E#nu",900,600);
        TGraph* gr_en = new TGraph((int)enu_vec.size(), enu_vec.data(), energy_vec.data());
        gr_en->SetTitle("Total energy of MARLEY clusters vs E_{#nu};E_{#nu} [MeV];Total energy [MeV]");
        gr_en->SetMarkerStyle(20);
        gr_en->SetMarkerColor(kBlue+1);
        gr_en->Draw("AP");
        
        // Perform linear fit
        TF1* fit = new TF1("fit_marley_energy", "pol1", 0, 100);
        fit->SetLineColor(kRed);
        fit->SetLineWidth(2);
        gr_en->Fit(fit, "Q"); // Q for quiet mode
        fit->Draw("SAME");
        
        // Get fit parameters
        double slope = fit->GetParameter(1);
        double intercept = fit->GetParameter(0);
        double slope_err = fit->GetParError(1);
        double intercept_err = fit->GetParError(0);
        double chi2 = fit->GetChisquare();
        int ndf = fit->GetNDF();
        
        // Display fit results on canvas
        TLatex latex;
        latex.SetNDC();
        latex.SetTextSize(0.03);
        latex.SetTextColor(kRed);
        latex.DrawLatex(0.15, 0.85, Form("Fit: E_{total} = %.3f #times E_{#nu} + %.3f", slope, intercept));
        latex.DrawLatex(0.15, 0.81, Form("Slope: %.3f #pm %.3f", slope, slope_err));
        latex.DrawLatex(0.15, 0.77, Form("Intercept: %.3f #pm %.3f", intercept, intercept_err));
        latex.DrawLatex(0.15, 0.73, Form("#chi^{2}/ndf = %.2f/%d = %.2f", chi2, ndf, (ndf > 0 ? chi2/ndf : 0)));
        
        gPad->SetGridx();
        gPad->SetGridy();
        addPageNumber(cen_scatter, pageNum, totalPages);
        cen_scatter->SaveAs(pdf.c_str());
        delete fit;
        delete gr_en;
        delete cen_scatter;
      }
    }

    // Page: True particle and neutrino energy distributions
    if (h_true_particle_energy->GetEntries() > 0 || h_true_neutrino_energy->GetEntries() > 0) {
      pageNum++;
      TCanvas* ce_dist = new TCanvas("c_ac_energy_dist","Energy distributions",1200,600);
      ce_dist->Divide(2,1);
      
      ce_dist->cd(1);
      if (h_true_particle_energy->GetEntries() > 0) {
        gPad->SetLogy();
        h_true_particle_energy->Draw("HIST");
      }
      
      ce_dist->cd(2);
      if (h_true_neutrino_energy->GetEntries() > 0) {
        // Linear scale (not log) as requested
        h_true_neutrino_energy->Draw("HIST");
      }
      
      ce_dist->cd(0);
      addPageNumber(ce_dist, pageNum, totalPages);
      ce_dist->SaveAs(pdf.c_str());
      delete ce_dist;
    }

    // Page: 2D correlations
    if (h2_particle_vs_cluster_energy->GetEntries() > 0 || h2_neutrino_vs_cluster_energy->GetEntries() > 0) {
      pageNum++;
      TCanvas* c2d_corr = new TCanvas("c_ac_2d_corr","Energy correlations",1200,600);
      c2d_corr->Divide(2,1);
      
      c2d_corr->cd(1);
      if (h2_particle_vs_cluster_energy->GetEntries() > 0) {
        gPad->SetRightMargin(0.12);
        gPad->SetLogz();
        h2_particle_vs_cluster_energy->SetContour(100);
        h2_particle_vs_cluster_energy->Draw("COLZ");
      }
      
      c2d_corr->cd(2);
      if (h2_neutrino_vs_cluster_energy->GetEntries() > 0) {
        gPad->SetRightMargin(0.12);
        gPad->SetLogz();
        h2_neutrino_vs_cluster_energy->SetContour(100);
        h2_neutrino_vs_cluster_energy->Draw("COLZ");
      }
      
      c2d_corr->cd(0);
      addPageNumber(c2d_corr, pageNum, totalPages);
      c2d_corr->SaveAs(pdf.c_str());
      delete c2d_corr;
    }

    // Page: Total particle energy vs total cluster charge per event (Collection Plane X)
    if (!vec_total_particle_energy.empty()) {
      pageNum++;
      TCanvas* c_tot_corr = new TCanvas("c_ac_tot_corr","Total energy vs charge per event",900,700);
      c_tot_corr->SetBottomMargin(0.12);
      c_tot_corr->SetTopMargin(0.10);
      c_tot_corr->SetLeftMargin(0.12);
      c_tot_corr->SetRightMargin(0.05);
      
      // Create TGraph from vectors (background points)
      TGraph* gr_calib = new TGraph((int)vec_total_particle_energy.size(), 
                                     vec_total_particle_energy.data(), 
                                     vec_total_cluster_charge.data());
      gr_calib->SetTitle("Total visible energy vs total cluster charge per event (Collection Plane X);Total visible particle energy [MeV];Total cluster charge [ADC]");
      gr_calib->SetMarkerStyle(20);
      gr_calib->SetMarkerSize(0.8);
      gr_calib->SetMarkerColor(kBlue);
      gr_calib->SetMarkerColorAlpha(kBlue, 0.15);
      gr_calib->GetXaxis()->SetLimits(0, 70);
      gr_calib->Draw("AP");
      
      // Create binned average graph
      TGraphErrors* gr_binned = createBinnedAverageGraph(vec_total_particle_energy, vec_total_cluster_charge);
      gr_binned->SetMarkerStyle(20);
      gr_binned->SetMarkerSize(1.2);
      gr_binned->SetMarkerColor(kBlue);
      gr_binned->SetLineColor(kBlue);
      gr_binned->Draw("P SAME");
      
      // Perform linear fit on binned data
      TF1* fit_tot = new TF1("fit_tot_energy_charge", "pol1", 0, 70);
      fit_tot->SetLineColor(kRed);
      fit_tot->SetLineWidth(2);
      gr_binned->Fit(fit_tot, "Q");
      fit_tot->Draw("SAME");
      
      // Get fit parameters
      double slope = fit_tot->GetParameter(1);
      double intercept = fit_tot->GetParameter(0);
      double chi2 = fit_tot->GetChisquare();
      int ndf = fit_tot->GetNDF();
      
      // Display fit results in a box
      TPaveText* pt_tot = new TPaveText(0.15, 0.75, 0.55, 0.88, "NDC");
      pt_tot->SetFillColor(kWhite);
      pt_tot->SetBorderSize(1);
      pt_tot->SetTextAlign(12);
      pt_tot->SetTextSize(0.030);
      pt_tot->AddText(Form("Fit: Charge = %.1f #times E_{visible} + %.1f", slope, intercept));
      pt_tot->AddText(Form("Slope: %.1f ADC/MeV", slope));
      pt_tot->AddText(Form("Intercept: %.1f ADC", intercept));
      pt_tot->AddText(Form("#chi^{2}/ndf = %.2f", (ndf > 0 ? chi2/ndf : 0)));
      pt_tot->Draw();
      
      gPad->SetGridx();
      gPad->SetGridy();
      addPageNumber(c_tot_corr, pageNum, totalPages);
      c_tot_corr->SaveAs(pdf.c_str());
      delete pt_tot;
      delete fit_tot;
      delete gr_binned;
      delete gr_calib;
      delete c_tot_corr;
    }

    // Page: Total particle energy vs total cluster charge per event (U Plane)
    if (!vec_total_particle_energy_U.empty()) {
      pageNum++;
      TCanvas* c_tot_corr_U = new TCanvas("c_ac_tot_corr_U","Total energy vs charge per event (U)",900,700);
      c_tot_corr_U->SetBottomMargin(0.12);
      c_tot_corr_U->SetTopMargin(0.10);
      c_tot_corr_U->SetLeftMargin(0.12);
      c_tot_corr_U->SetRightMargin(0.05);
      
      // Create TGraph from vectors (background points)
      TGraph* gr_calib_U = new TGraph((int)vec_total_particle_energy_U.size(), 
                                       vec_total_particle_energy_U.data(), 
                                       vec_total_cluster_charge_U.data());
      gr_calib_U->SetTitle("Total visible energy vs total cluster charge per event (U Plane);Total visible particle energy [MeV];Total cluster charge [ADC]");
      gr_calib_U->SetMarkerStyle(20);
      gr_calib_U->SetMarkerSize(0.8);
      gr_calib_U->SetMarkerColor(kGreen+2);
      gr_calib_U->SetMarkerColorAlpha(kGreen+2, 0.15);
      gr_calib_U->GetXaxis()->SetLimits(0, 70);
      gr_calib_U->Draw("AP");
      
      // Create binned average graph
      TGraphErrors* gr_binned_U = createBinnedAverageGraph(vec_total_particle_energy_U, vec_total_cluster_charge_U);
      gr_binned_U->SetMarkerStyle(20);
      gr_binned_U->SetMarkerSize(1.2);
      gr_binned_U->SetMarkerColor(kGreen+2);
      gr_binned_U->SetLineColor(kGreen+2);
      gr_binned_U->Draw("P SAME");
      
      // Perform linear fit on binned data
      TF1* fit_tot_U = new TF1("fit_tot_energy_charge_U", "pol1", 0, 70);
      fit_tot_U->SetLineColor(kRed);
      fit_tot_U->SetLineWidth(2);
      gr_binned_U->Fit(fit_tot_U, "Q");
      fit_tot_U->Draw("SAME");
      
      // Get fit parameters
      double slope_U = fit_tot_U->GetParameter(1);
      double intercept_U = fit_tot_U->GetParameter(0);
      double chi2_U = fit_tot_U->GetChisquare();
      int ndf_U = fit_tot_U->GetNDF();
      
      // Display fit results in a box
      TPaveText* pt_tot_U = new TPaveText(0.15, 0.75, 0.55, 0.88, "NDC");
      pt_tot_U->SetFillColor(kWhite);
      pt_tot_U->SetBorderSize(1);
      pt_tot_U->SetTextAlign(12);
      pt_tot_U->SetTextSize(0.030);
      pt_tot_U->AddText(Form("Fit: Charge = %.1f #times E_{visible} + %.1f", slope_U, intercept_U));
      pt_tot_U->AddText(Form("Slope: %.1f ADC/MeV", slope_U));
      pt_tot_U->AddText(Form("Intercept: %.1f ADC", intercept_U));
      pt_tot_U->AddText(Form("#chi^{2}/ndf = %.2f", (ndf_U > 0 ? chi2_U/ndf_U : 0)));
      pt_tot_U->Draw();
      
      gPad->SetGridx();
      gPad->SetGridy();
      addPageNumber(c_tot_corr_U, pageNum, totalPages);
      c_tot_corr_U->SaveAs(pdf.c_str());
      delete pt_tot_U;
      delete fit_tot_U;
      delete gr_binned_U;
      delete gr_calib_U;
      delete c_tot_corr_U;
    }

    // Page: Total particle energy vs total cluster charge per event (V Plane)
    if (!vec_total_particle_energy_V.empty()) {
      pageNum++;
      TCanvas* c_tot_corr_V = new TCanvas("c_ac_tot_corr_V","Total energy vs charge per event (V)",900,700);
      c_tot_corr_V->SetBottomMargin(0.12);
      c_tot_corr_V->SetTopMargin(0.10);
      c_tot_corr_V->SetLeftMargin(0.12);
      c_tot_corr_V->SetRightMargin(0.05);
      
      // Create TGraph from vectors (background points)
      TGraph* gr_calib_V = new TGraph((int)vec_total_particle_energy_V.size(), 
                                       vec_total_particle_energy_V.data(), 
                                       vec_total_cluster_charge_V.data());
      gr_calib_V->SetTitle("Total visible energy vs total cluster charge per event (V Plane);Total visible particle energy [MeV];Total cluster charge [ADC]");
      gr_calib_V->SetMarkerStyle(20);
      gr_calib_V->SetMarkerSize(0.8);
      gr_calib_V->SetMarkerColor(kMagenta+2);
      gr_calib_V->SetMarkerColorAlpha(kMagenta+2, 0.15);
      gr_calib_V->GetXaxis()->SetLimits(0, 70);
      gr_calib_V->Draw("AP");
      
      // Create binned average graph
      TGraphErrors* gr_binned_V = createBinnedAverageGraph(vec_total_particle_energy_V, vec_total_cluster_charge_V);
      gr_binned_V->SetMarkerStyle(20);
      gr_binned_V->SetMarkerSize(1.2);
      gr_binned_V->SetMarkerColor(kMagenta+2);
      gr_binned_V->SetLineColor(kMagenta+2);
      gr_binned_V->Draw("P SAME");
      
      // Perform linear fit on binned data
      TF1* fit_tot_V = new TF1("fit_tot_energy_charge_V", "pol1", 0, 70);
      fit_tot_V->SetLineColor(kRed);
      fit_tot_V->SetLineWidth(2);
      gr_binned_V->Fit(fit_tot_V, "Q");
      fit_tot_V->Draw("SAME");
      
      // Get fit parameters
      double slope_V = fit_tot_V->GetParameter(1);
      double intercept_V = fit_tot_V->GetParameter(0);
      double chi2_V = fit_tot_V->GetChisquare();
      int ndf_V = fit_tot_V->GetNDF();
      
      // Display fit results in a box
      TPaveText* pt_tot_V = new TPaveText(0.15, 0.75, 0.55, 0.88, "NDC");
      pt_tot_V->SetFillColor(kWhite);
      pt_tot_V->SetBorderSize(1);
      pt_tot_V->SetTextAlign(12);
      pt_tot_V->SetTextSize(0.030);
      pt_tot_V->AddText(Form("Fit: Charge = %.1f #times E_{visible} + %.1f", slope_V, intercept_V));
      pt_tot_V->AddText(Form("Slope: %.1f ADC/MeV", slope_V));
      pt_tot_V->AddText(Form("Intercept: %.1f ADC", intercept_V));
      pt_tot_V->AddText(Form("#chi^{2}/ndf = %.2f", (ndf_V > 0 ? chi2_V/ndf_V : 0)));
      pt_tot_V->Draw();
      
      gPad->SetGridx();
      gPad->SetGridy();
      addPageNumber(c_tot_corr_V, pageNum, totalPages);
      c_tot_corr_V->SaveAs(pdf.c_str());
      delete pt_tot_V;
      delete fit_tot_V;
      delete gr_binned_V;
      delete gr_calib_V;
      delete c_tot_corr_V;
    }

    // Page: SimIDE visible energy vs total cluster charge per event (Collection Plane X)
    if (use_simide_energy && !vec_total_simide_energy.empty()) {
      pageNum++;
      TCanvas* c_simide_corr = new TCanvas("c_ac_simide_corr","SimIDE energy vs charge per event",900,700);
      c_simide_corr->SetBottomMargin(0.12);
      c_simide_corr->SetTopMargin(0.10);
      c_simide_corr->SetLeftMargin(0.12);
      c_simide_corr->SetRightMargin(0.05);
      
      // Create TGraph for all individual points (background)
      TGraph* gr_simide_all = new TGraph((int)vec_total_simide_energy.size(), 
                                          vec_total_simide_energy.data(), 
                                          vec_total_cluster_charge_for_simide.data());
      gr_simide_all->SetTitle("Total SimIDE visible energy vs total cluster charge per event (Collection Plane X);Total SimIDE energy [MeV];Total cluster charge [ADC]");
      gr_simide_all->SetMarkerStyle(20);
      gr_simide_all->SetMarkerSize(0.8);
      gr_simide_all->SetMarkerColor(kBlue);
      gr_simide_all->SetMarkerColorAlpha(kBlue, 0.3);
      gr_simide_all->GetXaxis()->SetLimits(0, 70);
      gr_simide_all->Draw("AP");
      
      // Create binned average graph
      TGraphErrors* gr_simide = createBinnedAverageGraph(vec_total_simide_energy, vec_total_cluster_charge_for_simide);
      if (gr_simide) {
        gr_simide->SetMarkerStyle(21);
        gr_simide->SetMarkerSize(1.6);
        gr_simide->SetMarkerColor(kRed+1);
        gr_simide->SetLineColor(kRed+1);
        gr_simide->SetLineWidth(2);
        gr_simide->Draw("P SAME");
        
        // Perform linear fit on binned averages
  TF1* fit_simide = new TF1("fit_simide_energy_charge", "pol1", 0, 70);
        fit_simide->SetLineColor(kRed);
        fit_simide->SetLineWidth(3);
        gr_simide->Fit(fit_simide, "RQ"); // R for range, Q for quiet mode
        
        // Get fit parameters
        double slope_sim = fit_simide->GetParameter(1);
        double intercept_sim = fit_simide->GetParameter(0);
        double chi2_sim = fit_simide->GetChisquare();
        int ndf_sim = fit_simide->GetNDF();
        
        // Draw the fit line explicitly on top
        fit_simide->Draw("L SAME");
        
        // Display fit results in a box
        TPaveText* pt_sim = new TPaveText(0.15, 0.75, 0.55, 0.88, "NDC");
        pt_sim->SetFillColor(kWhite);
        pt_sim->SetBorderSize(1);
        pt_sim->SetTextAlign(12);
        pt_sim->SetTextSize(0.030);
        pt_sim->AddText(Form("Fit: Charge = %.1f #times E_{SimIDE} + %.1f", slope_sim, intercept_sim));
        pt_sim->AddText(Form("Slope: %.1f ADC/MeV", slope_sim));
        pt_sim->AddText(Form("Intercept: %.1f ADC", intercept_sim));
        pt_sim->AddText(Form("#chi^{2}/ndf = %.2f", (ndf_sim > 0 ? chi2_sim/ndf_sim : 0)));
        pt_sim->Draw();
      }
      
      gPad->SetGridx();
      gPad->SetGridy();
      addPageNumber(c_simide_corr, pageNum, totalPages);
      c_simide_corr->SaveAs(pdf.c_str());
      delete gr_simide_all;
      delete c_simide_corr;
    }

    // Page: SimIDE visible energy vs total cluster charge per event (U Plane)
    if (use_simide_energy && !vec_total_simide_energy_U.empty()) {
      pageNum++;
      TCanvas* c_simide_corr_U = new TCanvas("c_ac_simide_corr_U","SimIDE energy vs charge per event (U)",900,700);
      c_simide_corr_U->SetBottomMargin(0.12);
      c_simide_corr_U->SetTopMargin(0.10);
      c_simide_corr_U->SetLeftMargin(0.12);
      c_simide_corr_U->SetRightMargin(0.05);
      
      // Create TGraph for all individual points (background)
      TGraph* gr_simide_U_all = new TGraph((int)vec_total_simide_energy_U.size(), 
                                            vec_total_simide_energy_U.data(), 
                                            vec_total_cluster_charge_U_for_simide.data());
      gr_simide_U_all->SetTitle("Total SimIDE visible energy vs total cluster charge per event (U Plane);Total SimIDE energy [MeV];Total cluster charge [ADC]");
      gr_simide_U_all->SetMarkerStyle(20);
      gr_simide_U_all->SetMarkerSize(0.8);
      gr_simide_U_all->SetMarkerColor(kGreen+2);
      gr_simide_U_all->SetMarkerColorAlpha(kGreen+2, 0.3);
      gr_simide_U_all->GetXaxis()->SetLimits(0, 70);
      gr_simide_U_all->Draw("AP");
      
      // Create binned average graph
      TGraphErrors* gr_simide_U = createBinnedAverageGraph(vec_total_simide_energy_U, vec_total_cluster_charge_U_for_simide);
      if (gr_simide_U) {
        gr_simide_U->SetMarkerStyle(21);
        gr_simide_U->SetMarkerSize(1.6);
        gr_simide_U->SetMarkerColor(kRed+1);
        gr_simide_U->SetLineColor(kRed+1);
        gr_simide_U->SetLineWidth(2);
        gr_simide_U->Draw("P SAME");
        
        // Perform linear fit on binned averages
  TF1* fit_simide_U = new TF1("fit_simide_energy_charge_U", "pol1", 0, 70);
        fit_simide_U->SetLineColor(kRed);
        fit_simide_U->SetLineWidth(3);
        gr_simide_U->Fit(fit_simide_U, "RQ"); // R for range, Q for quiet mode
        
        // Get fit parameters
        double slope_sim_U = fit_simide_U->GetParameter(1);
        double intercept_sim_U = fit_simide_U->GetParameter(0);
        double chi2_sim_U = fit_simide_U->GetChisquare();
        int ndf_sim_U = fit_simide_U->GetNDF();
        
        // Draw the fit line explicitly on top
        fit_simide_U->Draw("L SAME");
        
        // Display fit results in a box
        TPaveText* pt_sim_U = new TPaveText(0.15, 0.75, 0.55, 0.88, "NDC");
        pt_sim_U->SetFillColor(kWhite);
        pt_sim_U->SetBorderSize(1);
        pt_sim_U->SetTextAlign(12);
        pt_sim_U->SetTextSize(0.030);
        pt_sim_U->AddText(Form("Fit: Charge = %.1f #times E_{SimIDE} + %.1f", slope_sim_U, intercept_sim_U));
        pt_sim_U->AddText(Form("Slope: %.1f ADC/MeV", slope_sim_U));
        pt_sim_U->AddText(Form("Intercept: %.1f ADC", intercept_sim_U));
        pt_sim_U->AddText(Form("#chi^{2}/ndf = %.2f", (ndf_sim_U > 0 ? chi2_sim_U/ndf_sim_U : 0)));
        pt_sim_U->Draw();
      }
      
      gPad->SetGridx();
      gPad->SetGridy();
      addPageNumber(c_simide_corr_U, pageNum, totalPages);
      c_simide_corr_U->SaveAs(pdf.c_str());
      delete gr_simide_U_all;
      delete c_simide_corr_U;
    }

    // Page: SimIDE visible energy vs total cluster charge per event (V Plane)
    if (use_simide_energy && !vec_total_simide_energy_V.empty()) {
      pageNum++;
      TCanvas* c_simide_corr_V = new TCanvas("c_ac_simide_corr_V","SimIDE energy vs charge per event (V)",900,700);
      c_simide_corr_V->SetBottomMargin(0.12);
      c_simide_corr_V->SetTopMargin(0.10);
      c_simide_corr_V->SetLeftMargin(0.12);
      c_simide_corr_V->SetRightMargin(0.05);
      
      // Create TGraph for all individual points (background)
      TGraph* gr_simide_V_all = new TGraph((int)vec_total_simide_energy_V.size(), 
                                            vec_total_simide_energy_V.data(), 
                                            vec_total_cluster_charge_V_for_simide.data());
      gr_simide_V_all->SetTitle("Total SimIDE visible energy vs total cluster charge per event (V Plane);Total SimIDE energy [MeV];Total cluster charge [ADC]");
      gr_simide_V_all->SetMarkerStyle(20);
      gr_simide_V_all->SetMarkerSize(0.8);
      gr_simide_V_all->SetMarkerColor(kMagenta+2);
      gr_simide_V_all->SetMarkerColorAlpha(kMagenta+2, 0.3);
      gr_simide_V_all->GetXaxis()->SetLimits(0, 70);
      gr_simide_V_all->Draw("AP");
      
      // Create binned average graph
      TGraphErrors* gr_simide_V = createBinnedAverageGraph(vec_total_simide_energy_V, vec_total_cluster_charge_V_for_simide);
      if (gr_simide_V) {
        gr_simide_V->SetMarkerStyle(21);
        gr_simide_V->SetMarkerSize(1.6);
        gr_simide_V->SetMarkerColor(kRed+1);
        gr_simide_V->SetLineColor(kRed+1);
        gr_simide_V->SetLineWidth(2);
        gr_simide_V->Draw("P SAME");
        
        // Perform linear fit on binned averages
  TF1* fit_simide_V = new TF1("fit_simide_energy_charge_V", "pol1", 0, 70);
        fit_simide_V->SetLineColor(kRed);
        fit_simide_V->SetLineWidth(3);
        gr_simide_V->Fit(fit_simide_V, "RQ"); // R for range, Q for quiet mode
        
        // Get fit parameters
        double slope_sim_V = fit_simide_V->GetParameter(1);
        double intercept_sim_V = fit_simide_V->GetParameter(0);
        double chi2_sim_V = fit_simide_V->GetChisquare();
        int ndf_sim_V = fit_simide_V->GetNDF();
        
        // Draw the fit line explicitly on top
        fit_simide_V->Draw("L SAME");
        
        // Display fit results in a box
        TPaveText* pt_sim_V = new TPaveText(0.15, 0.75, 0.55, 0.88, "NDC");
        pt_sim_V->SetFillColor(kWhite);
        pt_sim_V->SetBorderSize(1);
        pt_sim_V->SetTextAlign(12);
        pt_sim_V->SetTextSize(0.030);
        pt_sim_V->AddText(Form("Fit: Charge = %.1f #times E_{SimIDE} + %.1f", slope_sim_V, intercept_sim_V));
        pt_sim_V->AddText(Form("Slope: %.1f ADC/MeV", slope_sim_V));
        pt_sim_V->AddText(Form("Intercept: %.1f ADC", intercept_sim_V));
        pt_sim_V->AddText(Form("#chi^{2}/ndf = %.2f", (ndf_sim_V > 0 ? chi2_sim_V/ndf_sim_V : 0)));
        pt_sim_V->Draw();
      }
      
      gPad->SetGridx();
      gPad->SetGridy();
      addPageNumber(c_simide_corr_V, pageNum, totalPages);
      c_simide_corr_V->SaveAs(pdf.c_str());
      delete gr_simide_V_all;
      delete c_simide_corr_V;
    }

    // Page: Marley TP fraction categorization
    if (only_marley_clusters > 0 || partial_marley_clusters > 0 || no_marley_clusters > 0) {
      pageNum++;
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

      // Calculate total and percentages
      int total_clusters = only_marley_clusters + partial_marley_clusters + no_marley_clusters;
      double pct1 = (total_clusters > 0) ? 100.0 * only_marley_clusters / total_clusters : 0.0;
      double pct2 = (total_clusters > 0) ? 100.0 * partial_marley_clusters / total_clusters : 0.0;
      double pct3 = (total_clusters > 0) ? 100.0 * no_marley_clusters / total_clusters : 0.0;

      // Add text annotations with percentages instead of absolute numbers
      auto text1 = new TText(0.8, only_marley_clusters + 0.05 * hmarley->GetMaximum(), Form("%.1f%%", pct1));
      text1->SetTextAlign(22); text1->SetTextSize(0.04); text1->Draw();
      auto text2 = new TText(1.8, partial_marley_clusters + 0.05 * hmarley->GetMaximum(), Form("%.1f%%", pct2));
      text2->SetTextAlign(22); text2->SetTextSize(0.04); text2->Draw();
      auto text3 = new TText(2.8, no_marley_clusters + 0.05 * hmarley->GetMaximum(), Form("%.1f%%", pct3));
      text3->SetTextAlign(22); text3->SetTextSize(0.04); text3->Draw();

      addPageNumber(cmarley, pageNum, totalPages);
      cmarley->SaveAs(pdf.c_str());
      delete hmarley;
      delete cmarley;
    }

    // --- New Page: Total ADC Integral by Cluster Family ---
    if (h_adc_pure_marley->GetEntries() > 0 || h_adc_pure_noise->GetEntries() > 0 || 
      h_adc_hybrid->GetEntries() > 0 || h_adc_background->GetEntries() > 0 ||
      h_adc_mixed_signal_bkg->GetEntries() > 0) {
      pageNum++;
      TCanvas* c_adc = new TCanvas("c_adc_family", "Total ADC Integral by Cluster Family", 900, 700);
      
      // Set line colors
      h_adc_pure_marley->SetLineColor(kBlue);
      h_adc_pure_noise->SetLineColor(kGray+2);
      h_adc_hybrid->SetLineColor(kGreen+2);
      h_adc_background->SetLineColor(kRed);
      h_adc_mixed_signal_bkg->SetLineColor(kMagenta+2);
      h_adc_pure_marley->SetLineWidth(2);
      h_adc_pure_noise->SetLineWidth(2);
      h_adc_hybrid->SetLineWidth(2);
      h_adc_background->SetLineWidth(2);
      h_adc_mixed_signal_bkg->SetLineWidth(2);
      
      // Set fill colors with transparency for better visualization
      h_adc_pure_marley->SetFillColorAlpha(kBlue, 0.3);
      h_adc_pure_noise->SetFillColorAlpha(kGray+2, 0.3);
      h_adc_hybrid->SetFillColorAlpha(kGreen+2, 0.3);
      h_adc_background->SetFillColorAlpha(kRed, 0.3);
      h_adc_mixed_signal_bkg->SetFillColorAlpha(kMagenta+2, 0.3);

      // Hide stat boxes
      h_adc_pure_marley->SetStats(0);
      h_adc_pure_noise->SetStats(0);
      h_adc_hybrid->SetStats(0);
      h_adc_background->SetStats(0);
      h_adc_mixed_signal_bkg->SetStats(0);

      // Find max to set proper axis range
      double max_val = 0;
      max_val = std::max(max_val, h_adc_pure_marley->GetMaximum());
      max_val = std::max(max_val, h_adc_pure_noise->GetMaximum());
      max_val = std::max(max_val, h_adc_hybrid->GetMaximum());
      max_val = std::max(max_val, h_adc_background->GetMaximum());
      max_val = std::max(max_val, h_adc_mixed_signal_bkg->GetMaximum());
      
      h_adc_pure_marley->SetMaximum(max_val * 1.2);
      h_adc_pure_marley->SetMinimum(0.5);  // Set minimum for log scale to see low bins
      h_adc_pure_marley->SetTitle("Total ADC Integral by Cluster Family;Total ADC Integral;Clusters");
      h_adc_pure_marley->Draw("HIST");
      h_adc_pure_noise->Draw("HIST SAME");
      h_adc_hybrid->Draw("HIST SAME");
      h_adc_background->Draw("HIST SAME");
      h_adc_mixed_signal_bkg->Draw("HIST SAME");

      TLegend* leg = new TLegend(0.55,0.60,0.88,0.88);
      leg->AddEntry(h_adc_pure_marley, Form("Pure Marley (%.0f)", h_adc_pure_marley->GetEntries()), "l");
      leg->AddEntry(h_adc_pure_noise, Form("Pure Noise (%.0f)", h_adc_pure_noise->GetEntries()), "l");
      leg->AddEntry(h_adc_hybrid, Form("Marley+Noise (%.0f)", h_adc_hybrid->GetEntries()), "l");
      leg->AddEntry(h_adc_background, Form("Pure Background (%.0f)", h_adc_background->GetEntries()), "l");
      leg->AddEntry(h_adc_mixed_signal_bkg, Form("Marley+Background (%.0f)", h_adc_mixed_signal_bkg->GetEntries()), "l");
      leg->Draw();

      gPad->SetLogy();
      gPad->SetGridx();
      gPad->SetGridy();

      addPageNumber(c_adc, pageNum, totalPages);
      c_adc->SaveAs(pdf.c_str());
      delete leg;
      delete c_adc;
    }

    // --- New Page: Total Energy (from ADC) by Cluster Family ---
    if (h_energy_pure_marley->GetEntries() > 0 || h_energy_pure_noise->GetEntries() > 0 ||
        h_energy_hybrid->GetEntries() > 0 || h_energy_background->GetEntries() > 0 ||
        h_energy_mixed_signal_bkg->GetEntries() > 0) {
      pageNum++;
      TCanvas* c_energy = new TCanvas("c_energy_family", "Total Energy by Cluster Family", 900, 700);

      // Set line colors
      h_energy_pure_marley->SetLineColor(kBlue);
      h_energy_pure_noise->SetLineColor(kGray+2);
      h_energy_hybrid->SetLineColor(kGreen+2);
      h_energy_background->SetLineColor(kRed);
      h_energy_mixed_signal_bkg->SetLineColor(kMagenta+2);
      h_energy_pure_marley->SetLineWidth(2);
      h_energy_pure_noise->SetLineWidth(2);
      h_energy_hybrid->SetLineWidth(2);
      h_energy_background->SetLineWidth(2);
      h_energy_mixed_signal_bkg->SetLineWidth(2);

      // Set fill colors with transparency for better visualization
      h_energy_pure_marley->SetFillColorAlpha(kBlue, 0.3);
      h_energy_pure_noise->SetFillColorAlpha(kGray+2, 0.3);
      h_energy_hybrid->SetFillColorAlpha(kGreen+2, 0.3);
      h_energy_background->SetFillColorAlpha(kRed, 0.3);
      h_energy_mixed_signal_bkg->SetFillColorAlpha(kMagenta+2, 0.3);

      // Hide stat boxes
      h_energy_pure_marley->SetStats(0);
      h_energy_pure_noise->SetStats(0);
      h_energy_hybrid->SetStats(0);
      h_energy_background->SetStats(0);
      h_energy_mixed_signal_bkg->SetStats(0);

      // Set custom x-axis labels: every 2 MeV from 0 to 10, then every 5 MeV until 70
      TAxis* xaxis = h_energy_pure_marley->GetXaxis();
      // xaxis->SetNdivisions(510, kFALSE); // 5 major, 10 minor divisions per major
      xaxis->SetTickLength(0.03);
      // Remove existing labels
      for (int i = 1; i <= xaxis->GetNbins(); ++i) xaxis->SetBinLabel(i, "");
      // Set manual labels
      for (int mev = 0; mev <= 10; mev += 2) {
        int bin = xaxis->FindBin(mev);
        xaxis->SetBinLabel(bin, Form("%d", mev));
      }
      for (int mev = 15; mev <= 70; mev += 5) {
        int bin = xaxis->FindBin(mev);
        xaxis->SetBinLabel(bin, Form("%d", mev));
      }


      // Find max to set proper axis range
      double max_val_e = 0;
      max_val_e = std::max(max_val_e, h_energy_pure_marley->GetMaximum());
      max_val_e = std::max(max_val_e, h_energy_pure_noise->GetMaximum());
      max_val_e = std::max(max_val_e, h_energy_hybrid->GetMaximum());
      max_val_e = std::max(max_val_e, h_energy_background->GetMaximum());
      max_val_e = std::max(max_val_e, h_energy_mixed_signal_bkg->GetMaximum());
      
      h_energy_pure_marley->SetMaximum(max_val_e * 1.2);
      h_energy_pure_marley->SetMinimum(0.5);  // Set minimum for log scale to see low bins
      h_energy_pure_marley->SetTitle("Total Energy by Cluster Family;Total Energy [MeV];Clusters");
      h_energy_pure_marley->Draw("HIST");
      h_energy_pure_noise->Draw("HIST SAME");
      h_energy_hybrid->Draw("HIST SAME");
      h_energy_background->Draw("HIST SAME");
      h_energy_mixed_signal_bkg->Draw("HIST SAME");
      // Draw vertical dotted lines at every 2 units on X
      double x_min = h_energy_pure_marley->GetXaxis()->GetXmin();
      double x_max = h_energy_pure_marley->GetXaxis()->GetXmax();
      // Draw vertical lines only at labeled ticks
      for (int mev = 0; mev <= 10; mev += 2) {
        if (mev < x_min || mev > x_max) continue;
        TLine* line = new TLine(mev, 0, mev, max_val_e * 1.2);
        line->SetLineStyle(2); // dotted style
        line->SetLineColor(kBlack);
        line->SetLineWidth(1);
        line->Draw("SAME");
      }
      for (int mev = 15; mev <= 70; mev += 5) {
        if (mev < x_min || mev > x_max) continue;
        TLine* line = new TLine(mev, 0, mev, max_val_e * 1.2);
        line->SetLineStyle(2); // dotted style
        line->SetLineColor(kBlack);
        line->SetLineWidth(1);
        line->Draw("SAME");
      }

      TLegend* leg_e = new TLegend(0.55,0.60,0.88,0.88);
      leg_e->AddEntry(h_energy_pure_marley, Form("Pure Marley (%.0f)", h_energy_pure_marley->GetEntries()), "l");
      leg_e->AddEntry(h_energy_pure_noise, Form("Pure Noise (%.0f)", h_energy_pure_noise->GetEntries()), "l");
      leg_e->AddEntry(h_energy_hybrid, Form("Marley+Noise (%.0f)", h_energy_hybrid->GetEntries()), "l");
      leg_e->AddEntry(h_energy_background, Form("Pure Background (%.0f)", h_energy_background->GetEntries()), "l");
      leg_e->AddEntry(h_energy_mixed_signal_bkg, Form("Marley+Background (%.0f)", h_energy_mixed_signal_bkg->GetEntries()), "l");
      leg_e->Draw();

      gPad->SetLogy();
      // gPad->SetGridx();
      gPad->SetGridy();

      addPageNumber(c_energy, pageNum, totalPages);
      c_energy->SaveAs(pdf.c_str());
      delete leg_e;
      delete c_energy;
    }

  // Close PDF
  {
    TCanvas* cend = new TCanvas("c_ac_end","End",10,10);
    cend->SaveAs((pdf+")").c_str());
    delete cend;
  }
  
  // Cleanup histograms after PDF is generated
  for (auto& kv : n_tps_plane_h) delete kv.second;
  for (auto& kv : total_charge_plane_h) delete kv.second;
  for (auto& kv : total_energy_plane_h) delete kv.second;
  for (auto& kv : max_tp_charge_plane_h) delete kv.second;
  for (auto& kv : total_length_plane_h) delete kv.second;
  for (auto& kv : ntps_vs_total_charge_plane_h) delete kv.second;
  delete h_true_particle_energy;
  delete h_true_neutrino_energy;
  delete h_min_distance;
  delete h2_particle_vs_cluster_energy;
  delete h2_neutrino_vs_cluster_energy;
  delete h2_total_particle_energy_vs_total_charge;
  delete h_adc_pure_marley;
  delete h_adc_pure_noise;
  delete h_adc_hybrid;
  delete h_adc_background;
  delete h_adc_mixed_signal_bkg;
  delete h_energy_pure_marley;
  delete h_energy_pure_noise;
  delete h_energy_hybrid;
  delete h_energy_background;
  delete h_energy_mixed_signal_bkg;

  // Record produced file
  produced.push_back(std::filesystem::absolute(pdf).string());

  if (!produced.empty()){
    LogInfo << "\nSummary of produced files (" << produced.size() << "):" << std::endl;
    for (const auto& p : produced) LogInfo << " - " << p << std::endl;
  }
  return 0;
}
