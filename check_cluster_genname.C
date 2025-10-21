{
// Quick ROOT script to check generator_name values in clusters
    TFile* f = TFile::Open("output/prod_es_clustering/prodmarley_nue_flat_es_dune10kt_1x2x2_20250926T163503Z_gen_000515_supernova_g4_detsim_ana_2025-10-03T_050410Zbktr10_clusters_tick3_ch1_min1_tot0_en0.root");
    if (!f || f->IsZombie()) {
        std::cout << "Error opening file\n";
        return;
    }
    
    TTree* clusters = (TTree*)f->Get("clusters");
    if (!clusters) {
        std::cout << "No clusters tree found\n";
        return;
    }
    
    clusters->Print();
    
    std::string* generator_name = nullptr;
    if (clusters->GetBranch("generator_name")) {
        clusters->SetBranchAddress("generator_name", &generator_name);
        
        std::map<std::string, int> gen_counts;
        
        Long64_t nentries = clusters->GetEntries();
        std::cout << "\nTotal clusters: " << nentries << "\n";
        
        for (Long64_t i = 0; i < nentries; i++) {
            clusters->GetEntry(i);
            if (generator_name) {
                gen_counts[*generator_name]++;
            }
        }
        
        std::cout << "\nGenerator name distribution:\n";
        for (auto& p : gen_counts) {
            std::cout << "  " << p.first << ": " << p.second << "\n";
        }
    } else {
        std::cout << "No generator_name branch found in clusters tree\n";
    }
    
    f->Close();
}
