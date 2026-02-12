#include "global.h"

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

int main(int argc, char* argv[]) {
    CmdLineParser clp;
    clp.getDescription() << "> plot_avg_times app - Compare TP time_start with simides Timestamp (scaled by 32)." << std::endl;
    clp.addDummyOption("Main options");
    clp.addOption("input", {"-i", "--input"}, "Input ROOT file to analyze");
    clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");
    clp.addDummyOption();

    LogInfo << clp.getDescription().str() << std::endl;
    LogInfo << "Usage:" << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;

    clp.parseCmdLine(argc, argv);
    LogThrowIf( clp.isNoOptionTriggered(), "No option was provided." );

    std::string inputFile = clp.getOptionVal<std::string>("input");
    LogThrowIf( inputFile.empty(), "Input file not specified." );

    LogInfo << "Opening input file: " << inputFile << std::endl;
    TFile* file = TFile::Open(inputFile.c_str());
    LogThrowIf( (not file) or file->IsZombie(), "Cannot open file " << inputFile );

    // Get the triggerAnaDumpTPs directory
    TDirectoryFile* dir = (TDirectoryFile*)file->Get("triggerAnaDumpTPs");
    LogThrowIf( not dir, "Cannot find directory 'triggerAnaDumpTPs' in file." );
    
    // Get the TriggerPrimitives tree (TPs)
    TTree* tpTree = nullptr;
    TDirectoryFile* tpDir = (TDirectoryFile*)dir->Get("TriggerPrimitives");
    if (tpDir) {
        tpTree = (TTree*)tpDir->Get("tpmakerTPC__TriggerAnaTree1x2x2");
        if (tpTree) {
            LogInfo << "Found TP tree with " << tpTree->GetEntries() << " entries" << std::endl;
        }
    }
    
    // Get the simides tree
    TTree* simTree = (TTree*)dir->Get("simides");
    if (simTree) {
        LogInfo << "Found simides tree with " << simTree->GetEntries() << " entries" << std::endl;
    }

    LogThrowIf( (not tpTree) or (not simTree), "Could not find both TP and simides trees to analyze." );

    // Set style for detailed statistics
    gStyle->SetOptStat(111111);  // Show all statistics (name, entries, mean, RMS, underflow, overflow)
    
    // Create histograms for TP time_start and simides Timestamp comparison
    TH1F* hist_tp_time_start = new TH1F("hist_tp_time_start", "TP Time Start", 50, 0, 200000);
    TH1F* hist_simides_timestamp = new TH1F("hist_simides_timestamp", "SimIDEs Timestamp x32", 50, 0, 200000);
    // Configure visual styles for overlap clarity (diagonal hatch patterns)
    hist_simides_timestamp->SetFillStyle(3004); // one diagonal direction
    hist_simides_timestamp->SetFillColorAlpha(kRed, 0.35);
    hist_tp_time_start->SetFillStyle(3005);     // opposite diagonal direction
    hist_tp_time_start->SetFillColorAlpha(kBlue, 0.35);

    // Set up branches for TP timing data
    ULong_t tp_time_start = 0;
    tpTree->SetBranchAddress("time_start", &tp_time_start);
    
    // Set up branches for simides timing data (need to check the actual branch name)
    // Let's first print the simides tree structure
    LogInfo << "Printing simides tree structure:" << std::endl;
    simTree->Print(); // ROOT prints to std::cout internally
    
    // Set up simides timestamp branch (it's UShort_t based on tree structure)
    UShort_t simides_timestamp = 0;
    if (simTree->GetBranch("Timestamp")) {
        simTree->SetBranchAddress("Timestamp", &simides_timestamp);
        LogInfo << "Connected to simides Timestamp branch" << std::endl;
    } else {
        LogError << "Could not find Timestamp branch in simides tree" << std::endl;
        file->Close();
        return 1; // using return here since we already logged the error
    }

    // Fill histograms from TP tree
    Long64_t tpEntries = tpTree->GetEntries();
    LogInfo << "Processing " << tpEntries << " TP entries..." << std::endl;
    for (Long64_t i = 0; i < tpEntries; i++) {
        tpTree->GetEntry(i);
        hist_tp_time_start->Fill(tp_time_start);
    }
    
    // Fill histograms from simides tree (multiply timestamp by 32)
    Long64_t simEntries = simTree->GetEntries();
    LogInfo << "Processing " << simEntries << " simides entries..." << std::endl;
    for (Long64_t i = 0; i < simEntries; i++) {
        simTree->GetEntry(i);
        hist_simides_timestamp->Fill(simides_timestamp * 32);
    }
    
    // Removed time-difference histogram as requested; overlay provides comparative view.

    // Create a single canvas with both histograms overlaid
    TCanvas* canvas_tp_vs_simides = new TCanvas("canvas_tp_vs_simides", "TP vs SimIDEs Timing Comparison", 1200, 800);

    // Plot simides first (left axis)
    canvas_tp_vs_simides->cd();
    double maxSim = hist_simides_timestamp->GetMaximum();
    double maxTP  = hist_tp_time_start->GetMaximum();
    double leftMax = maxSim * 1.15; // add headroom
    hist_simides_timestamp->SetMaximum(leftMax);
    hist_simides_timestamp->SetLineColor(kRed);
    hist_simides_timestamp->SetLineWidth(2);
    hist_simides_timestamp->SetTitle("TP Time Start vs SimIDEs Timestamp");
    hist_simides_timestamp->GetXaxis()->SetTitle("Time");
    hist_simides_timestamp->GetYaxis()->SetTitle("SimIDEs Entries");
    hist_simides_timestamp->Draw();
    gPad->Update();
    TPaveStats* stats_sim = (TPaveStats*)hist_simides_timestamp->FindObject("stats");
    if (stats_sim) {
        stats_sim->SetX1NDC(0.68);
        stats_sim->SetX2NDC(0.88);
        stats_sim->SetY1NDC(0.7);
        stats_sim->SetY2NDC(0.9);
    }

    // Create scaled clone of TP histogram for overlay
    double rightMax = maxTP * 1.15;
    double scale = leftMax / rightMax; // multiply TP counts by this to map to left axis range
    TH1F* hist_tp_scaled = (TH1F*)hist_tp_time_start->Clone("hist_tp_time_start_scaled");
    hist_tp_scaled->SetDirectory(nullptr);
    hist_tp_scaled->SetLineColor(kBlue);
    hist_tp_scaled->SetLineWidth(2);
    hist_tp_scaled->SetFillStyle(hist_tp_time_start->GetFillStyle());
    hist_tp_scaled->SetFillColorAlpha(kBlue, 0.35);
    hist_tp_scaled->SetStats(true); // enable stats to compare visually (scaled counts)
    for (int b = 1; b <= hist_tp_scaled->GetNbinsX(); ++b) {
        hist_tp_scaled->SetBinContent(b, hist_tp_time_start->GetBinContent(b) * scale);
        hist_tp_scaled->SetBinError(b, hist_tp_time_start->GetBinError(b) * scale);
    }
    hist_tp_scaled->Draw("HIST SAME");
    gPad->Update();
    // Retrieve and stagger stat boxes (right side, non-overlapping)
    TPaveStats* stats_tp = (TPaveStats*)hist_tp_scaled->FindObject("stats");
    if (stats_sim) {
        stats_sim->SetX1NDC(0.68);
        stats_sim->SetX2NDC(0.88);
        stats_sim->SetY1NDC(0.78);
        stats_sim->SetY2NDC(0.93);
    }
    if (stats_tp) {
        stats_tp->SetX1NDC(0.68);
        stats_tp->SetX2NDC(0.88);
        stats_tp->SetY1NDC(0.55);
        stats_tp->SetY2NDC(0.72);
    }

    // Draw right axis with original TP scale
    gPad->Update();
    double xMax = gPad->GetUxmax();
    double xMin = gPad->GetUxmin();
    double yMin = gPad->GetUymin();
    double yMax = gPad->GetUymax();
    TGaxis* rightAxis = new TGaxis(xMax, yMin, xMax, yMax, 0, rightMax, 510, "+L");
    rightAxis->SetLineColor(kBlue);
    rightAxis->SetLabelColor(kBlue);
    rightAxis->SetTitle("TP Entries");
    rightAxis->SetTitleColor(kBlue);
    rightAxis->CenterTitle();
    rightAxis->Draw();

    // Add legend
    TLegend* legend = new TLegend(0.15, 0.75, 0.39, 0.90);
    legend->AddEntry(hist_simides_timestamp, "SimIDEs Timestamp x32", "l");
    legend->AddEntry(hist_tp_time_start, "TP Time Start", "l");
    legend->Draw();

    // Save the canvas
    canvas_tp_vs_simides->SaveAs("tp_vs_simides_comparison.png");

    // No separate distribution canvases anymore.

    // Print some statistics
    LogInfo << "\n=== TIMING COMPARISON STATISTICS ===" << std::endl;
    LogInfo << "TP Time Start - Mean: " << hist_tp_time_start->GetMean() 
        << ", RMS: " << hist_tp_time_start->GetRMS() << std::endl;
    LogInfo << "SimIDEs Timestamp x32 - Mean: " << hist_simides_timestamp->GetMean() 
        << ", RMS: " << hist_simides_timestamp->GetRMS() << std::endl;
    // Time difference stats removed.

    // Clean up
    delete hist_tp_time_start;
    delete hist_simides_timestamp;
    // scaled clone & right axis cleaned up with canvas deletion
    delete canvas_tp_vs_simides;
    file->Close();

    return 0;
}
