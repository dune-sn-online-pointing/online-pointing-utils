//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Thu Aug 20 11:24:47 2026 by ROOT version 6.32.10
// from TTree simides/simides
// found on file: prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root
//////////////////////////////////////////////////////////

#ifndef simides_h
#define simides_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

class simides {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   UInt_t          event;
   UInt_t          run;
   UInt_t          subrun;
   UInt_t          channel;
   UShort_t        timestamp;
   Float_t         numelectrons;
   Float_t         energy;
   Float_t         x;
   Float_t         y;
   Float_t         z;
   Int_t           trackID;
   Int_t           origTrackID;
   Int_t           readout_plane_id;
   Int_t           readout_view;
   Int_t           detector_element;

   // List of branches
   TBranch        *b_event;   //!
   TBranch        *b_run;   //!
   TBranch        *b_subrun;   //!
   TBranch        *b_channel;   //!
   TBranch        *b_timestamp;   //!
   TBranch        *b_numelectrons;   //!
   TBranch        *b_energy;   //!
   TBranch        *b_x;   //!
   TBranch        *b_y;   //!
   TBranch        *b_z;   //!
   TBranch        *b_trackID;   //!
   TBranch        *b_origTrackID;   //!
   TBranch        *b_readout_plane_id;   //!
   TBranch        *b_readout_view;   //!
   TBranch        *b_detector_element;   //!

   simides(TTree *tree=0);
   simides(const simides& other);
   simides& operator=(const simides&) = delete;
   virtual ~simides();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Show(Long64_t entry = -1);
};

#endif // #ifndef simides_h
