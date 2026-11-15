//////////////////////////////////////////////////////////
// Implementation for the simides class.
// Declarations live in simides.h.
//////////////////////////////////////////////////////////

#include "simides.h"

simides::simides(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root");
      }
      TDirectory * dir = (TDirectory*)f->Get("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root:/triggerAna");
      dir->GetObject("simides",tree);

   }
   Init(tree);
}

simides::simides(const simides& other) : fChain(nullptr), fCurrent(-1)
{
   event = other.event;
   run = other.run;
   subrun = other.subrun;
   channel = other.channel;
   timestamp = other.timestamp;
   numelectrons = other.numelectrons;
   energy = other.energy;
   x = other.x;
   y = other.y;
   z = other.z;
   trackID = other.trackID;
   origTrackID = other.origTrackID;
   readout_plane_id = other.readout_plane_id;
   readout_view = other.readout_view;
   detector_element = other.detector_element;

   b_event = nullptr;
   b_run = nullptr;
   b_subrun = nullptr;
   b_channel = nullptr;
   b_timestamp = nullptr;
   b_numelectrons = nullptr;
   b_energy = nullptr;
   b_x = nullptr;
   b_y = nullptr;
   b_z = nullptr;
   b_trackID = nullptr;
   b_origTrackID = nullptr;
   b_readout_plane_id = nullptr;
   b_readout_view = nullptr;
   b_detector_element = nullptr;
}

simides::~simides()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t simides::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t simides::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (fChain->GetTreeNumber() != fCurrent) {
      fCurrent = fChain->GetTreeNumber();
   }
   return centry;
}

void simides::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("event", &event, &b_event);
   fChain->SetBranchAddress("run", &run, &b_run);
   fChain->SetBranchAddress("subrun", &subrun, &b_subrun);
   fChain->SetBranchAddress("channel", &channel, &b_channel);
   fChain->SetBranchAddress("timestamp", &timestamp, &b_timestamp);
   fChain->SetBranchAddress("numelectrons", &numelectrons, &b_numelectrons);
   fChain->SetBranchAddress("energy", &energy, &b_energy);
   fChain->SetBranchAddress("x", &x, &b_x);
   fChain->SetBranchAddress("y", &y, &b_y);
   fChain->SetBranchAddress("z", &z, &b_z);
   fChain->SetBranchAddress("trackID", &trackID, &b_trackID);
   fChain->SetBranchAddress("origTrackID", &origTrackID, &b_origTrackID);
   fChain->SetBranchAddress("readout_plane_id", &readout_plane_id, &b_readout_plane_id);
   fChain->SetBranchAddress("readout_view", &readout_view, &b_readout_view);
   fChain->SetBranchAddress("detector_element", &detector_element, &b_detector_element);
}

void simides::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t simides::Cut(Long64_t entry)
{
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
