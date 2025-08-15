void countGen() {
  TFile *f = TFile::Open(" /exp/dune/app/users/emvilla/online-pointing-utils/data/clusters_tick10_ch5_min2_U.root");
  TTree *tree = (TTree*)f->Get("clusters");

  std::string *true_label = nullptr;
  tree->SetBranchAddress("true_label", &true_label);

  std::map<std::string, int> counts;

  Long64_t nentries = tree->GetEntries();
  for (Long64_t i = 0; i < nentries; ++i) {
    tree->GetEntry(i);
    counts[*true_label]++;
  }

  for (const auto &entry : counts) {
    std::cout << entry.first << " : " << entry.second << std::endl;
  }

  f->Close();
}
