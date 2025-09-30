// File: plot_avg_times.C
void plot_avg_times() {
    TFile *f = TFile::Open("data/cleanES60_tpstream.root");
    if (!f || f->IsZombie()) {
        std::cout << "Cannot open file!" << std::endl;
        return;
    }

    // Try to find TTrees robustly
    TTree *simIDE = nullptr;
    TTree *TPs = nullptr;
    // Try top-level first
    simIDE = (TTree*)f->Get("simIDE");
    TPs = (TTree*)f->Get("TPs");
    // If not found, try inside subdirectories
    if (!simIDE) {
        TDirectory *dir = f->GetDirectory("simIDE");
        if (dir) simIDE = (TTree*)dir->Get("simIDE");
    }
    if (!TPs) {
        TDirectory *dir = f->GetDirectory("TPs");
        if (dir) TPs = (TTree*)dir->Get("TPs");
    }

    if (!simIDE || !TPs) {
        std::cout << "TTrees not found!" << std::endl;
        f->ls(); // Print file structure for debugging
        return;
    }

    // Print branch info for debugging
    // simIDE->Print();
    // TPs->Print();

    // Use correct types for branches
    double simIDE_timestamp = 0;
    ULong64_t TP_time_start = 0;
    // If types mismatch, try float/int
    if (simIDE->GetBranch("timestamp")) {
        simIDE->SetBranchAddress("timestamp", &simIDE_timestamp);
    } else {
        std::cout << "Branch 'timestamp' not found in simIDE!" << std::endl;
        simIDE->Print();
        return;
    }
    if (TPs->GetBranch("time_start")) {
        TPs->SetBranchAddress("time_start", &TP_time_start);
    } else {
        std::cout << "Branch 'time_start' not found in TPs!" << std::endl;
        TPs->Print();
        return;
    }

    // Calculate average simIDE timestamp
    double sum_simIDE = 0;
    Long64_t n_simIDE = simIDE->GetEntries();
    for (Long64_t i = 0; i < n_simIDE; ++i) {
        simIDE->GetEntry(i);
        sum_simIDE += simIDE_timestamp;
    }
    double avg_simIDE = (n_simIDE > 0) ? sum_simIDE / n_simIDE : 0;

    // Calculate average TP time_start
    double sum_TP = 0;
    Long64_t n_TP = TPs->GetEntries();
    for (Long64_t i = 0; i < n_TP; ++i) {
        TPs->GetEntry(i);
        sum_TP += TP_time_start;
    }
    double avg_TP = (n_TP > 0) ? sum_TP / n_TP : 0;

    // Plot on canvas
    TCanvas *c = new TCanvas("c", "Average Times", 800, 600);
    TH1F *h = new TH1F("h", "Average Times;Type;Time", 2, 0, 2);
    h->SetBinContent(1, avg_simIDE);
    h->GetXaxis()->SetBinLabel(1, "simIDE avg timestamp");
    h->SetBinContent(2, avg_TP);
    h->GetXaxis()->SetBinLabel(2, "TPs avg time_start");
    h->SetBarWidth(0.5);
    h->SetBarOffset(0.25);
    h->Draw("b");

    c->SaveAs("avg_times.png");
}