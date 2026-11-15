//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Thu Aug 20 11:26:44 2026 by ROOT version 6.32.10
// from TTree tpmakerTPCsimpleThr__TPGen/tpmakerTPCsimpleThr__TPGen
// found on file: prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root
//////////////////////////////////////////////////////////

#ifndef TP_h
#define TP_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.
#include "string"

class TP {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   UInt_t          event;
   UInt_t          run;
   UInt_t          subrun;
   UChar_t         version;
   UChar_t         flag;
   UChar_t         detid;
   UInt_t          channel;
   UShort_t        samples_over_threshold;
   ULong_t         time_start;
   UShort_t        samples_to_peak;
   UInt_t          adc_integral;
   UShort_t        adc_peak;
   UInt_t          readout_plane_id;
   Int_t           readout_view;
   UInt_t          TPCSetID;
   Int_t           bt_primary_track_id;
   Double_t        bt_primary_track_numelectron_frac;
   Double_t        bt_primary_track_energy_frac;
   Double_t        bt_edep;
   Double_t        bt_numelectrons;
   Double_t        bt_x;
   Double_t        bt_y;
   Double_t        bt_z;
   Double_t        bt_primary_x;
   Double_t        bt_primary_y;
   Double_t        bt_primary_z;
   Int_t           bt_truth_block_id;
   string          *bt_generator_name;

   // List of branches
   TBranch        *b_event;   //!
   TBranch        *b_run;   //!
   TBranch        *b_subrun;   //!
   TBranch        *b_version;   //!
   TBranch        *b_flag;   //!
   TBranch        *b_detid;   //!
   TBranch        *b_channel;   //!
   TBranch        *b_samples_over_threshold;   //!
   TBranch        *b_time_start;   //!
   TBranch        *b_samples_to_peak;   //!
   TBranch        *b_adc_integral;   //!
   TBranch        *b_adc_peak;   //!
   TBranch        *b_readout_plane_id;   //!
   TBranch        *b_readout_view;   //!
   TBranch        *b_TPCSetID;   //!
   TBranch        *b_bt_primary_track_id;   //!
   TBranch        *b_bt_primary_track_numelectron_frac;   //!
   TBranch        *b_bt_primary_track_energy_frac;   //!
   TBranch        *b_bt_edep;   //!
   TBranch        *b_bt_numelectrons;   //!
   TBranch        *b_bt_x;   //!
   TBranch        *b_bt_y;   //!
   TBranch        *b_bt_z;   //!
   TBranch        *b_bt_primary_x;   //!
   TBranch        *b_bt_primary_y;   //!
   TBranch        *b_bt_primary_z;   //!
   TBranch        *b_bt_truth_block_id;   //!
   TBranch        *b_bt_generator_name;   //!

   TP(TTree *tree=0);
   virtual ~TP();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual bool     Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef TP_cxx
TP::TP(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root");
      }
      TDirectory * dir = (TDirectory*)f->Get("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root:/triggerAna/TriggerPrimitives");
      dir->GetObject("tpmakerTPCsimpleThr__TPGen",tree);

   }
   Init(tree);
}

TP::~TP()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t TP::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t TP::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (fChain->GetTreeNumber() != fCurrent) {
      fCurrent = fChain->GetTreeNumber();
      Notify();
   }
   return centry;
}

void TP::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set object pointer
   bt_generator_name = 0;
   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("event", &event, &b_event);
   fChain->SetBranchAddress("run", &run, &b_run);
   fChain->SetBranchAddress("subrun", &subrun, &b_subrun);
   fChain->SetBranchAddress("version", &version, &b_version);
   fChain->SetBranchAddress("flag", &flag, &b_flag);
   fChain->SetBranchAddress("detid", &detid, &b_detid);
   fChain->SetBranchAddress("channel", &channel, &b_channel);
   fChain->SetBranchAddress("samples_over_threshold", &samples_over_threshold, &b_samples_over_threshold);
   fChain->SetBranchAddress("time_start", &time_start, &b_time_start);
   fChain->SetBranchAddress("samples_to_peak", &samples_to_peak, &b_samples_to_peak);
   fChain->SetBranchAddress("adc_integral", &adc_integral, &b_adc_integral);
   fChain->SetBranchAddress("adc_peak", &adc_peak, &b_adc_peak);
   fChain->SetBranchAddress("readout_plane_id", &readout_plane_id, &b_readout_plane_id);
   fChain->SetBranchAddress("readout_view", &readout_view, &b_readout_view);
   fChain->SetBranchAddress("TPCSetID", &TPCSetID, &b_TPCSetID);
   fChain->SetBranchAddress("bt_primary_track_id", &bt_primary_track_id, &b_bt_primary_track_id);
   fChain->SetBranchAddress("bt_primary_track_numelectron_frac", &bt_primary_track_numelectron_frac, &b_bt_primary_track_numelectron_frac);
   fChain->SetBranchAddress("bt_primary_track_energy_frac", &bt_primary_track_energy_frac, &b_bt_primary_track_energy_frac);
   fChain->SetBranchAddress("bt_edep", &bt_edep, &b_bt_edep);
   fChain->SetBranchAddress("bt_numelectrons", &bt_numelectrons, &b_bt_numelectrons);
   fChain->SetBranchAddress("bt_x", &bt_x, &b_bt_x);
   fChain->SetBranchAddress("bt_y", &bt_y, &b_bt_y);
   fChain->SetBranchAddress("bt_z", &bt_z, &b_bt_z);
   fChain->SetBranchAddress("bt_primary_x", &bt_primary_x, &b_bt_primary_x);
   fChain->SetBranchAddress("bt_primary_y", &bt_primary_y, &b_bt_primary_y);
   fChain->SetBranchAddress("bt_primary_z", &bt_primary_z, &b_bt_primary_z);
   fChain->SetBranchAddress("bt_truth_block_id", &bt_truth_block_id, &b_bt_truth_block_id);
   fChain->SetBranchAddress("bt_generator_name", &bt_generator_name, &b_bt_generator_name);
   Notify();
}

bool TP::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return true;
}

void TP::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t TP::Cut(Long64_t entry)
{
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef TP_cxx
