//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Sun Nov 15 19:35:17 2026 by ROOT version 6.40.04
// from TTree tps/Trigger Primitives with embedded truth
// found on file: prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream_tps.root
//////////////////////////////////////////////////////////

// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-16 at 03:39:35 UTC
#ifndef newtps_h
#define newtps_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.
#include <string>
#include <memory>

class newtps {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   UInt_t          event;
   UInt_t          run;
   UShort_t        version;
   UInt_t          detid;
   UInt_t          channel;
   ULong64_t       samples_over_threshold;
   ULong64_t       time_start;
   ULong64_t       samples_to_peak;
   UInt_t          adc_integral;
   UShort_t        adc_peak;
   UShort_t        detector;
   Int_t           detector_channel;
   std::string    *view;
   Double_t        simide_energy;
   std::string    *generator_name;
   Int_t           particle_pdg;
   std::string    *particle_process;
   Double_t        particle_energy;
   Double_t        particle_x;
   Double_t        particle_y;
   Double_t        particle_z;
   Double_t        particle_px;
   Double_t        particle_py;
   Double_t        particle_pz;
   std::string    *neutrino_interaction;
   Double_t        neutrino_x;
   Double_t        neutrino_y;
   Double_t        neutrino_z;
   Double_t        neutrino_px;
   Double_t        neutrino_py;
   Double_t        neutrino_pz;
   Double_t        neutrino_energy;

   // List of branches
   TBranch        *b_event;   //!
   TBranch        *b_run;   //!
   TBranch        *b_version;   //!
   TBranch        *b_detid;   //!
   TBranch        *b_channel;   //!
   TBranch        *b_samples_over_threshold;   //!
   TBranch        *b_time_start;   //!
   TBranch        *b_samples_to_peak;   //!
   TBranch        *b_adc_integral;   //!
   TBranch        *b_adc_peak;   //!
   TBranch        *b_detector;   //!
   TBranch        *b_detector_channel;   //!
   TBranch        *b_view;   //!
   TBranch        *b_simide_energy;   //!
   TBranch        *b_generator_name;   //!
   TBranch        *b_particle_pdg;   //!
   TBranch        *b_particle_process;   //!
   TBranch        *b_particle_energy;   //!
   TBranch        *b_particle_x;   //!
   TBranch        *b_particle_y;   //!
   TBranch        *b_particle_z;   //!
   TBranch        *b_particle_px;   //!
   TBranch        *b_particle_py;   //!
   TBranch        *b_particle_pz;   //!
   TBranch        *b_neutrino_interaction;   //!
   TBranch        *b_neutrino_x;   //!
   TBranch        *b_neutrino_y;   //!
   TBranch        *b_neutrino_z;   //!
   TBranch        *b_neutrino_px;   //!
   TBranch        *b_neutrino_py;   //!
   TBranch        *b_neutrino_pz;   //!
   TBranch        *b_neutrino_energy;   //!

   newtps(TTree *tree=0);
   newtps(const newtps& other);
   newtps& operator=(const newtps&) = delete;
   virtual ~newtps();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Show(Long64_t entry = -1);
   void             Print() const;   // dump current field values (debugging)

private:
   std::unique_ptr<std::string> owned_view_;
   std::unique_ptr<std::string> owned_generator_name_;
   std::unique_ptr<std::string> owned_particle_process_;
   std::unique_ptr<std::string> owned_neutrino_interaction_;
};

#endif // #ifndef newtps_h
