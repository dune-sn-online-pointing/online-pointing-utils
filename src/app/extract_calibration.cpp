#include "Global.h"

LoggerInit([]{ Logger::getUserHeader() << "[" << FILENAME << "]"; });

int main(int argc, char* argv[]){
  CmdLineParser clp;
  clp.getDescription() << "> extract_calibration - compute per-event MARLEY TP ADC-integral sums and correlate with true energies" << std::endl;
  clp.addDummyOption("Main options");
  clp.addOption("json", {"-j","--json"}, "JSON file containing configuration");
  clp.addOption("inputFile", {"-i", "--input-file"}, "Input file with list OR single ROOT file path (overrides JSON inputs)");
  clp.addOption("outFolder", {"--output-folder"}, "Output folder path (optional)");
  clp.addTriggerOption("verboseMode", {"-v"}, "Verbose");
  clp.addDummyOption();
  LogInfo << clp.getDescription().str() << std::endl;
  LogInfo << "Usage:\n" << clp.getConfigSummary() << std::endl;
  clp.parseCmdLine(argc, argv);
  LogThrowIf(clp.isNoOptionTriggered(), "No option was provided.");

  // Load parameters
  ParametersManager::getInstance().loadParameters();

  std::string jsonPath = clp.getOptionVal<std::string>("json");
  std::ifstream jf(jsonPath); LogThrowIf(!jf.is_open(), "Could not open JSON: " << jsonPath);
  nlohmann::json j; jf >> j;

  auto resolvePath = [](const std::string& p)->std::string{ std::error_code ec; auto abs=std::filesystem::absolute(p, ec); return ec? p : abs.string(); };

  // Use utility function for file finding (assume _tps_bktr files for calibration)
  std::vector<std::string> inputs = find_input_files(j, "tps");
  
  // Override with CLI input if provided
  if (clp.isOptionTriggered("inputFile")) {
      std::string input_file = clp.getOptionVal<std::string>("inputFile");
      inputs.clear();
      if (input_file.find("_tps_bktr") != std::string::npos || input_file.find(".root") != std::string::npos) {
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

  int tot_cut = j.value("tot_cut", 0);

  // Check if we should limit the number of files to process
  int max_files = j.value("max_files", -1);
  if (max_files > 0) {
    LogInfo << "Max files: " << max_files << std::endl;
  } else {
    LogInfo << "Max files: unlimited" << std::endl;
  }

  std::vector<std::string> produced;

  int file_count = 0;
  for (const auto& inFile : inputs){
    file_count++;
    if (max_files > 0 && file_count > max_files) {
      LogInfo << "Reached max_files limit (" << max_files << "), stopping." << std::endl;
      break;
    }
    TFile* f = TFile::Open(inFile.c_str());
    if (!f || f->IsZombie()){ LogError << "Cannot open: " << inFile << std::endl; if(f){f->Close(); delete f;} continue; }

    // Read TPs from root level (no longer in directory)
    TTree* tpTree = dynamic_cast<TTree*>(f->Get("tps"));
    LogThrowIf(!tpTree, "Tree 'tps' not found in: " << inFile);

    int evt=0; ULong64_t sot=0; UInt_t adc_int=0; std::string* gen=nullptr;
    tpTree->SetBranchAddress("event", &evt);
    tpTree->SetBranchAddress("samples_over_threshold", &sot);
    if (tpTree->GetBranch("adc_integral")) tpTree->SetBranchAddress("adc_integral", &adc_int);
    tpTree->SetBranchAddress("generator_name", &gen);

    // true particle energy per event (if available) and neutrino energy - these are optional trees
    std::map<int, double> evt_true_particle_energy;
    if (auto* tTruth = dynamic_cast<TTree*>(f->Get("true_particles"))){
      int tevt=0; float en=0; tTruth->SetBranchAddress("event", &tevt);
      if (tTruth->GetBranch("en")) tTruth->SetBranchAddress("en", &en);
      Long64_t n = tTruth->GetEntries();
      for (Long64_t i=0;i<n;++i){ tTruth->GetEntry(i); evt_true_particle_energy[tevt] = en; }
    }
    std::map<int, double> evt_nu_energy;
    if (auto* nuTree = dynamic_cast<TTree*>(f->Get("neutrinos"))){
      int nevt=0; int nen=0; nuTree->SetBranchAddress("event", &nevt); if (nuTree->GetBranch("en")) nuTree->SetBranchAddress("en", &nen);
      Long64_t n = nuTree->GetEntries(); for (Long64_t i=0;i<n;++i){ nuTree->GetEntry(i); evt_nu_energy[nevt] = (double)nen; }
    }

    // Sum MARLEY ADC integrals per event (after ToT cut)
    std::map<int, double> marley_sum_adcint;
    Long64_t ntp = tpTree->GetEntries();
    for (Long64_t i=0;i<ntp;++i){ tpTree->GetEntry(i); if ((long long)sot <= tot_cut) continue; if (!gen) continue; std::string low=*gen; std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return (char)std::tolower(c); }); if (low.find("marley")==std::string::npos) continue; marley_sum_adcint[evt] += (double)adc_int; }

    // Prepare plots
    std::vector<double> x_true, y_sum; x_true.reserve(marley_sum_adcint.size()); y_sum.reserve(marley_sum_adcint.size());
    for (const auto& kv : marley_sum_adcint){ double x = -1; auto it=evt_true_particle_energy.find(kv.first); if (it!=evt_true_particle_energy.end()) x = it->second; x_true.push_back(x); y_sum.push_back(kv.second); }
    std::vector<double> x_nu; x_nu.reserve(marley_sum_adcint.size());
    for (const auto& kv : marley_sum_adcint){ double x=-1; auto it=evt_nu_energy.find(kv.first); if (it!=evt_nu_energy.end()) x = it->second; x_nu.push_back(x); }

    // Output PDF path
    std::string base = inFile.substr(inFile.find_last_of("/\\")+1); auto dot=base.find_last_of('.'); if (dot!=std::string::npos) base = base.substr(0, dot);
    std::string outDir = outFolder.empty()? inFile.substr(0, inFile.find_last_of("/\\")) : outFolder; if (outDir.empty()) outDir = ".";
    std::string pdf = outDir + "/" + base + "_calib_tot" + std::to_string(tot_cut) + ".pdf";

    // Start PDF
    TCanvas* c = new TCanvas("c_cal_title","Title",800,600); c->cd(); c->SetFillColor(kWhite);
    auto t = new TText(0.5,0.75, "Calibration Summary"); t->SetTextAlign(22); t->SetTextSize(0.06); t->SetNDC(); t->Draw();
    auto finfo = new TText(0.5,0.55, Form("Input: %s", inFile.c_str())); finfo->SetTextAlign(22); finfo->SetTextSize(0.03); finfo->SetNDC(); finfo->Draw();
    auto tott = new TText(0.5,0.48, Form("ToT cut: %d", tot_cut)); tott->SetTextAlign(22); tott->SetTextSize(0.03); tott->SetNDC(); tott->Draw();
    c->SaveAs((pdf+"(").c_str()); delete c;

    // Scatter: sum ADC integral vs true particle energy
    if (!x_true.empty()){
      TCanvas* cs = new TCanvas("c_cal_true","Sum ADC int vs true particle energy",900,650); cs->cd();
      TGraph* gr = new TGraph((int)x_true.size(), x_true.data(), y_sum.data());
      gr->SetTitle("MARLEY TP ADC integral sum per event vs true particle energy;E_{true} [MeV];#Sigma ADC integral (MARLEY TPs)");
      gr->SetMarkerStyle(20); gr->SetMarkerColor(kBlue+1); gr->Draw("AP"); gPad->SetGridx(); gPad->SetGridy(); cs->SaveAs(pdf.c_str()); delete gr; delete cs;
    }

    // Scatter: sum ADC integral vs neutrino energy
    if (!x_nu.empty()){
      TCanvas* cs2 = new TCanvas("c_cal_nu","Sum ADC int vs neutrino energy",900,650); cs2->cd();
      TGraph* gr2 = new TGraph((int)x_nu.size(), x_nu.data(), y_sum.data());
      gr2->SetTitle("MARLEY TP ADC integral sum per event vs neutrino energy;E_{#nu} [MeV];#Sigma ADC integral (MARLEY TPs)");
      gr2->SetMarkerStyle(20); gr2->SetMarkerColor(kRed+1); gr2->Draw("AP"); gPad->SetGridx(); gPad->SetGridy(); cs2->SaveAs(pdf.c_str()); delete gr2; delete cs2;
    }

    // Distribution of sums
    if (!y_sum.empty()){
      double ymax = *std::max_element(y_sum.begin(), y_sum.end());
      int nb = 100; double hi = std::max(1000.0, ymax*1.05);
      TH1F* hsum = new TH1F("h_sum", "Per-event MARLEY #Sigma ADC integral;#Sigma ADC integral;Events", nb, 0, hi);
      for (double v : y_sum) hsum->Fill(v);
      TCanvas* ch = new TCanvas("c_cal_hist","Sum ADC integral distribution",900,650); ch->cd(); gPad->SetLogy(); hsum->Draw("HIST"); ch->SaveAs(pdf.c_str()); delete hsum; delete ch;
    }

    // Close PDF
    { TCanvas* cend = new TCanvas("c_cal_end","End",10,10); cend->SaveAs((pdf+")").c_str()); delete cend; }

    f->Close(); delete f;
    produced.push_back(std::filesystem::absolute(pdf).string());
  }

  if (!produced.empty()){
    LogInfo << "\nSummary of produced files (" << produced.size() << "):" << std::endl;
    for (const auto& p : produced) LogInfo << " - " << p << std::endl;
  }
  return 0;
}
