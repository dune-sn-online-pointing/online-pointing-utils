{
  auto f1 = TFile::Open("data/prod_es/bkgs/es_valid_bg_tick3_ch2_min2_tot1_ecutI0_ecutC0_clusters_0.root");
  auto f2 = TFile::Open("data/prod_es/bkgs/es_valid_bg_tick3_ch2_min2_tot1_ecutI0_ecutC0_clusters_1.root");
  
  auto dir1 = (TDirectory*)f1->Get("clusters");
  auto dir2 = (TDirectory*)f2->Get("clusters");
  
  auto t1x = (TTree*)dir1->Get("clusters_tree_X");
  auto t2x = (TTree*)dir2->Get("clusters_tree_X");
  
  std::cout << "File 0 has " << t1x->GetEntries() << " clusters in X plane" << std::endl;
  std::cout << "File 1 has " << t2x->GetEntries() << " clusters in X plane" << std::endl;
  std::cout << "Total expected: " << (t1x->GetEntries() + t2x->GetEntries()) << std::endl;
}
