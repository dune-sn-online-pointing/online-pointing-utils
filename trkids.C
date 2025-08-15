void trkids() {
  TFile *f = TFile::Open(" /exp/dune/app/users/emvilla/v10_07_00d00/output/standard/prodmarley_nue_flat_ES_dune10kt_1x2x2_standard-triggerana_tree_1x2x2_simpleThr60_allPlanes-100events_testSample0/triggersim_hist.root");
  TTree *tree = (TTree*)f->Get("triggerAnaDumpTPs/simides");

  int track_id = 0;
  tree->SetBranchAddress("trackID", &track_id);

  std::map<int, int> counts;

  Long64_t nentries = tree->GetEntries();
  for (Long64_t i = 0; i < nentries; ++i) {
    tree->GetEntry(i);
    counts[track_id]++;
  }

  std::cout << "Number of entries: " << nentries << std::endl;  

  for (const auto &entry : counts) {
    if (entry.second > 0) {
      std::cout << entry.first << " : " << entry.second << std::endl;
    }
  }

  // print how many are positive, how many negative
  int positive = 0;
  int negative = 0;
  for (const auto &entry : counts) {
    if (entry.first > 0) {
      positive++;
    } else {
      negative++;
    }
  }
  std::cout << "Positive: " << positive << std::endl;
  std::cout << "Negative: " << negative << std::endl;

  // check if there are entries where the track id is the same but opposite sign
  // for example, if there are 2 entries with track id 1 and 1 entry with track id -1
  int total_opposite = 0;
  for (const auto &entry : counts) {
    if (entry.first > 0 && counts[-entry.first] > 0) {
      std::cout << "Found track id " << entry.first << " and " << -entry.first << std::endl;
      std::cout << "Counts: " << entry.second << " and " << counts[-entry.first] << std::endl;
      if (-entry.first < 0) {
        total_opposite += counts[-entry.first];
      }
    }
  }
  std::cout << "Total opposite: " << total_opposite << std::endl;

  // print total list of track ids
  std::cout << "Total track ids: " << counts.size() << std::endl;
  for (const auto &entry : counts) {
    std::cout << entry.first << " : " << entry.second << std::endl;
  }

  f->Close();
}
