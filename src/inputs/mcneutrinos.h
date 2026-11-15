//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Thu Aug 20 11:23:23 2026 by ROOT version 6.32.10
// from TTree mcneutrinos/mcneutrinos
// found on file: prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root
//////////////////////////////////////////////////////////

#ifndef mcneutrinos_h
#define mcneutrinos_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.
#include <string>
#include <memory>

class mcneutrinos {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   UInt_t          event;
   UInt_t          run;
   UInt_t          subrun;
   Int_t           block_id;
   std::string    *generator_name;
   Int_t           nupdg;
   Int_t           leptonpdg;
   Int_t           ccnc;
   Int_t           mode;
   Int_t           interactionType;
   Int_t           target;
   Int_t           hitnuc;
   Int_t           hitquark;
   Double_t        w;
   Double_t        x;
   Double_t        y;
   Double_t        qsqr;
   Double_t        pt;
   Double_t        theta;

   // List of branches
   TBranch        *b_event;   //!
   TBranch        *b_run;   //!
   TBranch        *b_subrun;   //!
   TBranch        *b_block_id;   //!
   TBranch        *b_generator_name;   //!
   TBranch        *b_nupdg;   //!
   TBranch        *b_leptonpdg;   //!
   TBranch        *b_ccnc;   //!
   TBranch        *b_mode;   //!
   TBranch        *b_interactionType;   //!
   TBranch        *b_target;   //!
   TBranch        *b_hitnuc;   //!
   TBranch        *b_hitquark;   //!
   TBranch        *b_w;   //!
   TBranch        *b_x;   //!
   TBranch        *b_y;   //!
   TBranch        *b_qsqr;   //!
   TBranch        *b_pt;   //!
   TBranch        *b_theta;   //!

   mcneutrinos(TTree *tree=0);
   mcneutrinos(const mcneutrinos& other);
   mcneutrinos& operator=(const mcneutrinos&) = delete;
   virtual ~mcneutrinos();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Show(Long64_t entry = -1);

private:
   std::unique_ptr<std::string> owned_generator_name_;
};

#endif // #ifndef mcneutrinos_h
