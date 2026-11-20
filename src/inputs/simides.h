//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Thu Aug 20 11:24:47 2026 by ROOT version 6.32.10
// from TTree simides/simides
// found on file: prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root
//////////////////////////////////////////////////////////

// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 19:55:39 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 20:37:05 UTC (time-windowed SumEnergy)
#ifndef simides_h
#define simides_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.
#include <vector>

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

   // Sum the deposited energy over all SimIDEs in `ides` that belong to the
   // given primary track and channel (trackID linkage). Used to fill a TP's
   // simide_energy from the per-event SimIDE list.
   static Double_t SumEnergy(const std::vector<simides>& ides, Int_t trackID, UInt_t channel);

   // As above, but also require the SimIDE to fall within +/- time_tol of
   // tp_time (localizes the sum like the old backtracker's time window).
   // WARNING: in this file the SimIDE `timestamp` (UShort_t, ~O(1e3)) and the
   // TP time_start (~O(1e5)) are NOT on the same clock -- a conversion/offset
   // is needed for this window to be physically meaningful. Tune/convert on
   // review; with the raw values a small tolerance will reject everything.
   static Double_t SumEnergy(const std::vector<simides>& ides, Int_t trackID, UInt_t channel,
                             double tp_time, double time_tol);
};

#endif // #ifndef simides_h
