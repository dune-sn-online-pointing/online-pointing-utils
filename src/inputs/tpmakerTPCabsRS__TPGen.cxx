//////////////////////////////////////////////////////////
// Implementation for the tpmakerTPCabsRS__TPGen class.
// Declarations live in tpmakerTPCabsRS__TPGen.h.
//////////////////////////////////////////////////////////

#include "tpmakerTPCabsRS__TPGen.h"

tpmakerTPCabsRS__TPGen::tpmakerTPCabsRS__TPGen(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_run_1843_1500-2999_tpstream.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_run_1843_1500-2999_tpstream.root");
      }
      TDirectory * dir = (TDirectory*)f->Get("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_run_1843_1500-2999_tpstream.root:/triggerAna/TriggerPrimitives");
      dir->GetObject("tpmakerTPCabsRS__TPGen",tree);

   }
   Init(tree);
}

tpmakerTPCabsRS__TPGen::tpmakerTPCabsRS__TPGen(const tpmakerTPCabsRS__TPGen& other)
   : fChain(nullptr), fCurrent(-1), bt_generator_name(nullptr)
{
   event = other.event;
   run = other.run;
   subrun = other.subrun;
   version = other.version;
   flag = other.flag;
   detid = other.detid;
   channel = other.channel;
   samples_over_threshold = other.samples_over_threshold;
   time_start = other.time_start;
   samples_to_peak = other.samples_to_peak;
   adc_integral = other.adc_integral;
   adc_peak = other.adc_peak;
   readout_plane_id = other.readout_plane_id;
   readout_view = other.readout_view;
   TPCSetID = other.TPCSetID;
   bt_primary_track_id = other.bt_primary_track_id;
   bt_primary_track_numelectron_frac = other.bt_primary_track_numelectron_frac;
   bt_primary_track_energy_frac = other.bt_primary_track_energy_frac;
   bt_edep = other.bt_edep;
   bt_numelectrons = other.bt_numelectrons;
   bt_x = other.bt_x;
   bt_y = other.bt_y;
   bt_z = other.bt_z;
   bt_primary_x = other.bt_primary_x;
   bt_primary_y = other.bt_primary_y;
   bt_primary_z = other.bt_primary_z;
   bt_truth_block_id = other.bt_truth_block_id;

   if (other.bt_generator_name) {
      owned_bt_generator_name_.reset(new std::string(*other.bt_generator_name));
      bt_generator_name = owned_bt_generator_name_.get();
   }

   b_event = nullptr;
   b_run = nullptr;
   b_subrun = nullptr;
   b_version = nullptr;
   b_flag = nullptr;
   b_detid = nullptr;
   b_channel = nullptr;
   b_samples_over_threshold = nullptr;
   b_time_start = nullptr;
   b_samples_to_peak = nullptr;
   b_adc_integral = nullptr;
   b_adc_peak = nullptr;
   b_readout_plane_id = nullptr;
   b_readout_view = nullptr;
   b_TPCSetID = nullptr;
   b_bt_primary_track_id = nullptr;
   b_bt_primary_track_numelectron_frac = nullptr;
   b_bt_primary_track_energy_frac = nullptr;
   b_bt_edep = nullptr;
   b_bt_numelectrons = nullptr;
   b_bt_x = nullptr;
   b_bt_y = nullptr;
   b_bt_z = nullptr;
   b_bt_primary_x = nullptr;
   b_bt_primary_y = nullptr;
   b_bt_primary_z = nullptr;
   b_bt_truth_block_id = nullptr;
   b_bt_generator_name = nullptr;
}

tpmakerTPCabsRS__TPGen::~tpmakerTPCabsRS__TPGen()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t tpmakerTPCabsRS__TPGen::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t tpmakerTPCabsRS__TPGen::LoadTree(Long64_t entry)
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

void tpmakerTPCabsRS__TPGen::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.

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
}

void tpmakerTPCabsRS__TPGen::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t tpmakerTPCabsRS__TPGen::Cut(Long64_t entry)
{
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
