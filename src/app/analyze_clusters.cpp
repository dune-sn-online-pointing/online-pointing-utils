#include "Clustering.h"

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

// Helper function to add page number to canvas
void addPageNumber(TCanvas* canvas, int pageNum, int totalPages) {
  canvas->cd();
  TText* pageText = new TText(0.95, 0.02, Form("Page %d/%d", pageNum, totalPages));
  pageText->SetTextAlign(31); // right-aligned
  pageText->SetTextSize(0.02);
  pageText->SetNDC();
  pageText->Draw();
}

int main(int argc, char* argv[]){
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

  // Avoid ROOT auto-ownership of histograms to prevent double deletes on TFile close
  TH1::AddDirectory(kFALSE);

  gStyle->SetOptStat(111111); // no stats box on plots

  std::string json = clp.getOptionVal<std::string>("json");
  std::ifstream jf(json);
  LogThrowIf(!jf.is_open(), "Could not open JSON: " << json);
  nlohmann::json j;
  jf >> j;

  // Use utility function for file finding (clusters files)
  std::vector<std::string> inputs = find_input_files(j, "_clusters.root");

  // Override with CLI input if provided
  if (clp.isOptionTriggered("inputFile")) {
    std::string input_file = clp.getOptionVal<std::string>("inputFile");
    inputs.clear();
    if (input_file.find("_clusters.root") != std::string::npos || input_file.find(".root") != std::string::npos) {
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

  // Check if we should limit the number of files to process
  int max_files = j.value("max_files", -1);
  if (max_files > 0) {
    LogInfo << "Max files: " << max_files << std::endl;
  } else {
    LogInfo << "Max files: unlimited" << std::endl;
  }

  std::string outFolder;
  if (clp.isOptionTriggered("outFolder"))
    outFolder = clp.getOptionVal<std::string>("outFolder");
  else if (j.contains("output_folder"))
    outFolder = j.value("output_folder", std::string(""));

  std::vector<std::string> produced;

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
    LogThrowIf(planes.empty(), "No clusters trees found in file: " << clusters_file);

    // Output PDF path
    std::string base = clusters_file.substr(clusters_file.find_last_of("/\\")+1);
    auto dot = base.find_last_of('.');
    if (dot!=std::string::npos) base = base.substr(0, dot);
    std::string outDir = outFolder.empty()? clusters_file.substr(0, clusters_file.find_last_of("/\\")) : outFolder;
    if (outDir.empty()) outDir = ".";
    std::string pdf = outDir + "/" + base + "_report.pdf";

    // Accumulators across planes
    std::map<std::string, long long> label_counts_all;
    std::map<std::string, std::vector<long long>> label_counts_by_plane; // plane -> counts by label not needed, we keep total per label and per plane n_tps
    std::map<std::string, TH1F*> n_tps_plane_h;
    std::map<std::string, TH1F*> total_charge_plane_h;
    std::map<std::string, TH1F*> total_energy_plane_h;
    std::map<std::string, TH1F*> max_tp_charge_plane_h;
    std::map<std::string, TH1F*> total_length_plane_h;
    std::map<std::string, TH2F*> ntps_vs_total_charge_plane_h;
    
    // Additional histograms for interesting quantities
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
    TH2F* h2_total_particle_energy_vs_total_charge = new TH2F("h2_tot_part_e_vs_charge", "Total visible energy vs total cluster charge per event (Collection Plane);Total visible particle energy [MeV];Total cluster charge [ADC]", 100, 0, 70, 100, 0, 6000);
    h2_total_particle_energy_vs_total_charge->SetDirectory(nullptr);
    
    std::vector<double> marley_enu;
    std::vector<double> marley_ncl; // per event MARLEY Cluster count vs Eν
    std::map<int, double> marley_total_energy_per_event; // sum of total_energy for MARLEY clusters per event
    std::map<int, double> event_neutrino_energy; // neutrino energy per event
    std::map<int,int> sn_clusters_per_event; // sum across planes
    std::map<int, double> event_total_particle_energy; // sum of unique particle energies per event (excluding neutrinos)
    std::map<int, double> event_total_cluster_charge; // sum of all cluster charges per event
    // Track unique particles per event using a set of (energy, position) pairs
    std::map<int, std::set<std::tuple<float, float, float, float>>> event_unique_particles; // event -> set of (energy, x, y, z)
    // Vectors for the calibration graph
    std::vector<double> vec_total_particle_energy;
    std::vector<double> vec_total_cluster_charge;

    // Marley TP fraction categorization counters (across all planes)
    int only_marley_clusters = 0;     // generator_tp_fraction = 1.0
    int partial_marley_clusters = 0;  // 0 < generator_tp_fraction < 1.0
    int no_marley_clusters = 0;       // generator_tp_fraction = 0.0

    // Histograms for total ADC integral by cluster family
    TH1F* h_adc_pure_marley = new TH1F("h_adc_pure_marley", "Total ADC Integral: Pure Marley;Total ADC Integral;Clusters", 50, 0, 50000);
    TH1F* h_adc_pure_noise = new TH1F("h_adc_pure_noise", "Total ADC Integral: Pure Noise;Total ADC Integral;Clusters", 50, 0, 50000);
    TH1F* h_adc_hybrid = new TH1F("h_adc_hybrid", "Total ADC Integral: Hybrid;Total ADC Integral;Clusters", 50, 0, 50000);
    TH1F* h_adc_background = new TH1F("h_adc_background", "Total ADC Integral: Background;Total ADC Integral;Clusters", 50, 0, 50000);
    h_adc_pure_marley->SetDirectory(nullptr);
    h_adc_pure_noise->SetDirectory(nullptr);
    h_adc_hybrid->SetDirectory(nullptr);
    h_adc_background->SetDirectory(nullptr);

    auto ensureHist = [](std::map<std::string, TH1F*>& m, const std::string& key, const char* title, int nbins=100, double xmin=0, double xmax=100){
      if (m.count(key)==0){
        m[key] = new TH1F((key+"_h").c_str(), title, nbins, xmin, xmax);
        m[key]->SetStats(0);
        m[key]->SetDirectory(nullptr);
      }
      return m[key];
    };
    auto ensureHist2D = [](std::map<std::string, TH2F*>& m, const std::string& key, const char* title){
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
      float supernova_tp_fraction=0.f, generator_tp_fraction=0.f;
      // vector branches for derived quantities
      std::vector<int>* v_chan=nullptr;
      std::vector<int>* v_tstart=nullptr;
      std::vector<int>* v_sot=nullptr;
      std::vector<int>* v_adcint=nullptr;
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
      if (pd.tree->GetBranch("true_pos_x")) pd.tree->SetBranchAddress("true_pos_x", &true_pos_x);
      if (pd.tree->GetBranch("true_pos_y")) pd.tree->SetBranchAddress("true_pos_y", &true_pos_y);
      if (pd.tree->GetBranch("true_pos_z")) pd.tree->SetBranchAddress("true_pos_z", &true_pos_z);
      // vectors
      if (pd.tree->GetBranch("tp_detector_channel")) pd.tree->SetBranchAddress("tp_detector_channel", &v_chan);
      if (pd.tree->GetBranch("tp_time_start")) pd.tree->SetBranchAddress("tp_time_start", &v_tstart);
      if (pd.tree->GetBranch("tp_samples_over_threshold")) pd.tree->SetBranchAddress("tp_samples_over_threshold", &v_sot);
      if (pd.tree->GetBranch("tp_adc_integral")) pd.tree->SetBranchAddress("tp_adc_integral", &v_adcint);

      // Histos per plane
      TH1F* h_ntps = ensureHist(n_tps_plane_h, std::string("n_tps_")+pd.name, (std::string("Cluster size (n_tps) - ")+pd.name+";n_{TPs};Clusters").c_str(), 100, 0, 100);
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
        if (true_label_ptr){
          label_counts_all[*true_label_ptr]++;
          if (toLower(*true_label_ptr).find("marley")!=std::string::npos){
            marley_count_evt[event]++;
            marley_total_energy_per_event[event] += total_charge;
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
        
        // Track unique particles and accumulate their energies (excluding neutrinos)
        // Use particle energy and position to identify unique particles
        if (true_pe > 0) {
          // Create a unique identifier for this particle using energy and position
          auto particle_id = std::make_tuple(true_pe, true_pos_x, true_pos_y, true_pos_z);
          // Insert returns pair<iterator, bool> where bool is true if insertion happened
          if (event_unique_particles[event].insert(particle_id).second) {
            // This is a new unique particle, add its energy
            event_total_particle_energy[event] += true_pe;
          }
        }
        
        // Accumulate per-event total cluster charge (only from collection plane X)
        if (pd.name == "X") {
          event_total_cluster_charge[event] += total_charge;
        }

        // Categorize clusters by Marley TP content
        if (generator_tp_fraction == 1.0f) {
          only_marley_clusters++;
        } else if (generator_tp_fraction > 0.0f && generator_tp_fraction < 1.0f) {
          partial_marley_clusters++;
        } else if (generator_tp_fraction == 0.0f) {
          no_marley_clusters++;
        }

        // --- Fill ADC integral histograms by cluster family ---
        // We need to look at the TP-level generator information to classify properly
        // Use the v_adcint vector to compute total ADC integral
        if (v_adcint && !v_adcint->empty()) {
          double adc_sum = 0;
          for (auto val : *v_adcint) adc_sum += val;
          
          // Classify based on true_label and generator_tp_fraction
          bool has_marley = false, has_noise = false, has_other = false;
          
          // Check the true_label
          if (true_label_ptr) {
            std::string label = *true_label_ptr;
            if (label == "marley") has_marley = true;
            else if (label == "UNKNOWN") has_noise = true;
            else if (!label.empty()) has_other = true;
          }
          
          // Also use generator_tp_fraction for more precise classification
          if (generator_tp_fraction == 1.0f) {
            // Pure marley
            h_adc_pure_marley->Fill(adc_sum);
          } else if (generator_tp_fraction == 0.0f) {
            // Pure noise (no generator info)
            h_adc_pure_noise->Fill(adc_sum);
          } else if (generator_tp_fraction > 0.0f && generator_tp_fraction < 1.0f) {
            // Hybrid (mix of marley and noise)
            h_adc_hybrid->Fill(adc_sum);
          }
          
          // Check if it's a background (not marley, not unknown)
          if (true_label_ptr && *true_label_ptr != "marley" && *true_label_ptr != "UNKNOWN" && !true_label_ptr->empty()) {
            h_adc_background->Fill(adc_sum);
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
    
    // After processing all planes, prepare data for the calibration graph
    // Note: We only use collection plane (X) data, no need to divide
    for (const auto& kv : event_total_particle_energy) {
      int evt = kv.first;
      double tot_part_e = kv.second;
      auto it = event_total_cluster_charge.find(evt);
      if (it != event_total_cluster_charge.end()) {
        double tot_charge = it->second;
        vec_total_particle_energy.push_back(tot_part_e);
        vec_total_cluster_charge.push_back(tot_charge);
        // Also fill the histogram for backup
        h2_total_particle_energy_vs_total_charge->Fill(tot_part_e, tot_charge);
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

    // Start PDF with title page
    int pageNum = 1;
    int totalPages = 15; // Updated to include new page
    
    TCanvas* c = new TCanvas("c_ac_title","Title",800,600); c->cd();
    c->SetFillColor(kWhite);
    auto t = new TText(0.5,0.85, "Cluster Analysis Report");
    t->SetTextAlign(22); t->SetTextSize(0.06); t->SetNDC(); t->Draw();
    
    // Split path and filename on separate lines
    std::string path_part = clusters_file.substr(0, clusters_file.find_last_of("/\\")+1);
    std::string file_part = clusters_file.substr(clusters_file.find_last_of("/\\")+1);
    auto ptxt = new TText(0.5,0.65, Form("Path: %s", path_part.c_str()));
    ptxt->SetTextAlign(22); ptxt->SetTextSize(0.025); ptxt->SetNDC(); ptxt->Draw();
    auto ftxt = new TText(0.5,0.60, Form("File: %s", file_part.c_str()));
    ftxt->SetTextAlign(22); ftxt->SetTextSize(0.025); ftxt->SetNDC(); ftxt->Draw();
    
    // Extract clustering parameters from filename (e.g., tick5_ch2_min2_tot1)
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
          auto txt = new TText(0.5, ypos, Form("Min total charge threshold: %s", tot_val.c_str()));
          txt->SetTextAlign(22); txt->SetTextSize(0.025); txt->SetNDC(); txt->Draw();
        }
      }
    }
    
    // Add timestamp
    TDatime now;
    auto date_txt = new TText(0.5, 0.15, Form("Generated on: %s", now.AsString()));
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

    // Page: Total particle energy vs total cluster charge per event
    if (!vec_total_particle_energy.empty()) {
      pageNum++;
      TCanvas* c_tot_corr = new TCanvas("c_ac_tot_corr","Total energy vs charge per event",900,700);
      
      // Create TGraph from vectors
      TGraph* gr_calib = new TGraph((int)vec_total_particle_energy.size(), 
                                     vec_total_particle_energy.data(), 
                                     vec_total_cluster_charge.data());
      gr_calib->SetTitle("Total visible energy vs total cluster charge per event;Total visible particle energy [MeV];Total cluster charge [ADC]");
      gr_calib->SetMarkerStyle(20);
      gr_calib->SetMarkerSize(0.8);
      gr_calib->SetMarkerColor(kBlue+1);
      gr_calib->Draw("AP");
      
      // Perform linear fit
      TF1* fit_tot = new TF1("fit_tot_energy_charge", "pol1", 0, 70);
      fit_tot->SetLineColor(kRed);
      fit_tot->SetLineWidth(2);
      gr_calib->Fit(fit_tot, "Q"); // Q for quiet mode
      fit_tot->Draw("SAME");
      
      // Get fit parameters
      double slope = fit_tot->GetParameter(1);
      double intercept = fit_tot->GetParameter(0);
      double slope_err = fit_tot->GetParError(1);
      double intercept_err = fit_tot->GetParError(0);
      double chi2 = fit_tot->GetChisquare();
      int ndf = fit_tot->GetNDF();
      
      // Display fit results on canvas
      TLatex latex;
      latex.SetNDC();
      latex.SetTextSize(0.03);
      latex.SetTextColor(kRed);
      latex.DrawLatex(0.15, 0.85, Form("Fit: Charge = %.3f #times E_{visible} + %.3f", slope, intercept));
      latex.DrawLatex(0.15, 0.81, Form("Slope: %.3f #pm %.3f ADC/MeV", slope, slope_err));
      latex.DrawLatex(0.15, 0.77, Form("Intercept: %.3f #pm %.3f ADC", intercept, intercept_err));
      latex.DrawLatex(0.15, 0.73, Form("#chi^{2}/ndf = %.2f/%d = %.2f", chi2, ndf, (ndf > 0 ? chi2/ndf : 0)));
      
      gPad->SetGridx();
      gPad->SetGridy();
      addPageNumber(c_tot_corr, pageNum, totalPages);
      c_tot_corr->SaveAs(pdf.c_str());
      delete fit_tot;
      delete gr_calib;
      delete c_tot_corr;
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

      // Add text annotations with counts
      auto text1 = new TText(0.8, only_marley_clusters + 0.05 * hmarley->GetMaximum(), Form("%d", only_marley_clusters));
      text1->SetTextAlign(22); text1->SetTextSize(0.04); text1->Draw();
      auto text2 = new TText(1.8, partial_marley_clusters + 0.05 * hmarley->GetMaximum(), Form("%d", partial_marley_clusters));
      text2->SetTextAlign(22); text2->SetTextSize(0.04); text2->Draw();
      auto text3 = new TText(2.8, no_marley_clusters + 0.05 * hmarley->GetMaximum(), Form("%d", no_marley_clusters));
      text3->SetTextAlign(22); text3->SetTextSize(0.04); text3->Draw();

      addPageNumber(cmarley, pageNum, totalPages);
      cmarley->SaveAs(pdf.c_str());
      delete hmarley;
      delete cmarley;
    }

    // --- New Page: Total ADC Integral by Cluster Family ---
    if (h_adc_pure_marley->GetEntries() > 0 || h_adc_pure_noise->GetEntries() > 0 || 
        h_adc_hybrid->GetEntries() > 0 || h_adc_background->GetEntries() > 0) {
      pageNum++;
      TCanvas* c_adc = new TCanvas("c_adc_family", "Total ADC Integral by Cluster Family", 900, 700);
      
      h_adc_pure_marley->SetLineColor(kBlue);
      h_adc_pure_noise->SetLineColor(kGray+2);
      h_adc_hybrid->SetLineColor(kGreen+2);
      h_adc_background->SetLineColor(kRed);
      h_adc_pure_marley->SetLineWidth(2);
      h_adc_pure_noise->SetLineWidth(2);
      h_adc_hybrid->SetLineWidth(2);
      h_adc_background->SetLineWidth(2);

      // Find max to set proper axis range
      double max_val = 0;
      max_val = std::max(max_val, h_adc_pure_marley->GetMaximum());
      max_val = std::max(max_val, h_adc_pure_noise->GetMaximum());
      max_val = std::max(max_val, h_adc_hybrid->GetMaximum());
      max_val = std::max(max_val, h_adc_background->GetMaximum());
      
      h_adc_pure_marley->SetMaximum(max_val * 1.2);
      h_adc_pure_marley->SetTitle("Total ADC Integral by Cluster Family;Total ADC Integral;Clusters");
      h_adc_pure_marley->Draw("HIST");
      h_adc_pure_noise->Draw("HIST SAME");
      h_adc_hybrid->Draw("HIST SAME");
      h_adc_background->Draw("HIST SAME");

      TLegend* leg = new TLegend(0.65,0.65,0.88,0.88);
      leg->AddEntry(h_adc_pure_marley, Form("Pure Marley (%.0f)", h_adc_pure_marley->GetEntries()), "l");
      leg->AddEntry(h_adc_pure_noise, Form("Pure Noise (%.0f)", h_adc_pure_noise->GetEntries()), "l");
      leg->AddEntry(h_adc_hybrid, Form("Hybrid (%.0f)", h_adc_hybrid->GetEntries()), "l");
      leg->AddEntry(h_adc_background, Form("Background (%.0f)", h_adc_background->GetEntries()), "l");
      leg->Draw();

      gPad->SetLogy();
      gPad->SetGridx();
      gPad->SetGridy();

      addPageNumber(c_adc, pageNum, totalPages);
      c_adc->SaveAs(pdf.c_str());
      delete leg;
      delete c_adc;
    }

    // Close PDF and file
    {
      TCanvas* cend = new TCanvas("c_ac_end","End",10,10);
      cend->SaveAs((pdf+")").c_str());
      delete cend;
    }
    f->Close();
    delete f;
    // cleanup hists
    for (auto& kv : n_tps_plane_h) delete kv.second; n_tps_plane_h.clear();
    for (auto& kv : total_charge_plane_h) delete kv.second; total_charge_plane_h.clear();
    for (auto& kv : total_energy_plane_h) delete kv.second; total_energy_plane_h.clear();
    for (auto& kv : max_tp_charge_plane_h) delete kv.second; max_tp_charge_plane_h.clear();
    for (auto& kv : total_length_plane_h) delete kv.second; total_length_plane_h.clear();
    for (auto& kv : ntps_vs_total_charge_plane_h) delete kv.second; ntps_vs_total_charge_plane_h.clear();
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

    // record produced
    produced.push_back(std::filesystem::absolute(pdf).string());
  }

  if (!produced.empty()){
    LogInfo << "\nSummary of produced files (" << produced.size() << "):" << std::endl;
    for (const auto& p : produced) LogInfo << " - " << p << std::endl;
  }
  return 0;
}
