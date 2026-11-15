//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Fri Jun 12 10:36:15 2026 by ROOT version 6.38.04
// from TTree tpmakerTPCabsRS__TPGen/tpmakerTPCabsRS__TPGen
// found on file: prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_run_1843_1500-2999_tpstream.root
//////////////////////////////////////////////////////////

#ifndef tpmakerTPCabsRS__TPGen_h
#define tpmakerTPCabsRS__TPGen_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.
#include <string>
#include <memory>

class tpmakerTPCabsRS__TPGen {
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
   std::string     *bt_generator_name;

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

   tpmakerTPCabsRS__TPGen(TTree *tree=0);
   tpmakerTPCabsRS__TPGen(const tpmakerTPCabsRS__TPGen& other);
   tpmakerTPCabsRS__TPGen& operator=(const tpmakerTPCabsRS__TPGen&) = delete;
   virtual ~tpmakerTPCabsRS__TPGen();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Show(Long64_t entry = -1);

private:
   std::unique_ptr<std::string> owned_bt_generator_name_;
};

#endif // #ifndef tpmakerTPCabsRS__TPGen_h
