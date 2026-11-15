//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Thu Aug 20 11:23:03 2026 by ROOT version 6.32.10
// from TTree mctruths/mctruths
// found on file: prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root
//////////////////////////////////////////////////////////

// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 20:59:41 UTC
#ifndef mctruths_h
#define mctruths_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.
#include <string>
#include <memory>

class mctruths {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   UInt_t          event;
   UInt_t          run;
   UInt_t          subrun;
   Int_t           block_id;
   Int_t           truth_track_id;
   Int_t           pdg;
   std::string     *generator_name;
   Int_t           status_code;
   Double_t        x;
   Double_t        y;
   Double_t        z;
   Double_t        px;
   Double_t        py;
   Double_t        pz;
   Double_t        p;
   Double_t        energy;
   Double_t        kinetic_energy;
   std::string     *process;

   // List of branches
   TBranch        *b_event;   //!
   TBranch        *b_run;   //!
   TBranch        *b_subrun;   //!
   TBranch        *b_block_id;   //!
   TBranch        *b_truth_track_id;   //!
   TBranch        *b_pdg;   //!
   TBranch        *b_generator_name;   //!
   TBranch        *b_status_code;   //!
   TBranch        *b_x;   //!
   TBranch        *b_y;   //!
   TBranch        *b_z;   //!
   TBranch        *b_px;   //!
   TBranch        *b_py;   //!
   TBranch        *b_pz;   //!
   TBranch        *b_p;   //!
   TBranch        *b_energy;   //!
   TBranch        *b_kinetic_energy;   //!
   TBranch        *b_process;   //!

   mctruths(TTree *tree=0);
   mctruths(const mctruths& other);
   mctruths& operator=(const mctruths&) = delete;
   virtual ~mctruths();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Show(Long64_t entry = -1);
   void             Print() const;   // dump current field values (debugging)

private:
   std::unique_ptr<std::string> owned_generator_name_;
   std::unique_ptr<std::string> owned_process_;
};

#endif // #ifndef mctruths_h
