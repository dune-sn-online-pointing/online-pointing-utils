{
// Quick ROOT script to check generator_name values in TPs
    TFile* f = TFile::Open("data/prodmarley_nue_flat_es_dune10kt_1x2x2_20250926T155638Z_gen_000004_supernova_g4_detsim_ana_2025-10-03T_044709Zbktr10_clusters_tick5_ch2_min2_tot1_en0.root");
    if (!f || f->IsZombie()) {
        std::cout << "Error opening file\n";
        return;
    }
    
    TTree* tps = (TTree*)f->Get("tps");
    if (!tps) {
        std::cout << "No TPs tree found\n";
        return;
    }
    
    std::string* generator_name = nullptr;
    tps->SetBranchAddress("generator_name", &generator_name);
    
    std::map<std::string, int> gen_counts;
    
    Long64_t nentries = tps->GetEntries();
    std::cout << "Total TPs: " << nentries << "\n";
    
    for (Long64_t i = 0; i < TMath::Min(nentries, 10000LL); i++) {
        tps->GetEntry(i);
        if (generator_name) {
            gen_counts[*generator_name]++;
        }
    }
    
    std::cout << "\nGenerator name distribution (first 10000 TPs):\n";
    for (auto& p : gen_counts) {
        std::cout << "  " << p.first << ": " << p.second << "\n";
    }
    
    f->Close();
}
