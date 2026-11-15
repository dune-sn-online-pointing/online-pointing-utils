//////////////////////////////////////////////////////////
// Implementation for the mcparticles class.
// Declarations live in mcparticles.h.
//////////////////////////////////////////////////////////

#include "mcparticles.h"

mcparticles::mcparticles(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root");
      }
      TDirectory * dir = (TDirectory*)f->Get("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root:/triggerAna");
      dir->GetObject("mcparticles",tree);

   }
   Init(tree);
}

mcparticles::mcparticles(const mcparticles& other)
   : fChain(nullptr), fCurrent(-1), generator_name(nullptr), process(nullptr)
{
   event = other.event;
   run = other.run;
   subrun = other.subrun;
   pdg = other.pdg;
   status_code = other.status_code;
   g4_track_id = other.g4_track_id;
   mother = other.mother;
   truth_block_id = other.truth_block_id;
   x = other.x;
   y = other.y;
   z = other.z;
   t = other.t;
   end_x = other.end_x;
   end_y = other.end_y;
   end_z = other.end_z;
   end_t = other.end_t;
   px = other.px;
   py = other.py;
   pz = other.pz;
   energy = other.energy;
   kinetic_energy = other.kinetic_energy;
   edep = other.edep;
   numelectrons = other.numelectrons;
   shower_edep = other.shower_edep;
   shower_numelectrons = other.shower_numelectrons;

   if (other.generator_name) {
      owned_generator_name_.reset(new std::string(*other.generator_name));
      generator_name = owned_generator_name_.get();
   }
   if (other.process) {
      owned_process_.reset(new std::string(*other.process));
      process = owned_process_.get();
   }

   b_event = nullptr;
   b_run = nullptr;
   b_subrun = nullptr;
   b_pdg = nullptr;
   b_generator_name = nullptr;
   b_status_code = nullptr;
   b_g4_track_id = nullptr;
   b_mother = nullptr;
   b_truth_block_id = nullptr;
   b_x = nullptr;
   b_y = nullptr;
   b_z = nullptr;
   b_t = nullptr;
   b_end_x = nullptr;
   b_end_y = nullptr;
   b_end_z = nullptr;
   b_end_t = nullptr;
   b_px = nullptr;
   b_py = nullptr;
   b_pz = nullptr;
   b_energy = nullptr;
   b_kinetic_energy = nullptr;
   b_edep = nullptr;
   b_numelectrons = nullptr;
   b_shower_edep = nullptr;
   b_shower_numelectrons = nullptr;
   b_process = nullptr;
}

mcparticles::~mcparticles()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t mcparticles::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t mcparticles::LoadTree(Long64_t entry)
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

void mcparticles::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set object pointer
   generator_name = 0;
   process = 0;
   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("event", &event, &b_event);
   fChain->SetBranchAddress("run", &run, &b_run);
   fChain->SetBranchAddress("subrun", &subrun, &b_subrun);
   fChain->SetBranchAddress("pdg", &pdg, &b_pdg);
   fChain->SetBranchAddress("generator_name", &generator_name, &b_generator_name);
   fChain->SetBranchAddress("status_code", &status_code, &b_status_code);
   fChain->SetBranchAddress("g4_track_id", &g4_track_id, &b_g4_track_id);
   fChain->SetBranchAddress("mother", &mother, &b_mother);
   fChain->SetBranchAddress("truth_block_id", &truth_block_id, &b_truth_block_id);
   fChain->SetBranchAddress("x", &x, &b_x);
   fChain->SetBranchAddress("y", &y, &b_y);
   fChain->SetBranchAddress("z", &z, &b_z);
   fChain->SetBranchAddress("t", &t, &b_t);
   fChain->SetBranchAddress("end_x", &end_x, &b_end_x);
   fChain->SetBranchAddress("end_y", &end_y, &b_end_y);
   fChain->SetBranchAddress("end_z", &end_z, &b_end_z);
   fChain->SetBranchAddress("end_t", &end_t, &b_end_t);
   fChain->SetBranchAddress("px", &px, &b_px);
   fChain->SetBranchAddress("py", &py, &b_py);
   fChain->SetBranchAddress("pz", &pz, &b_pz);
   fChain->SetBranchAddress("energy", &energy, &b_energy);
   fChain->SetBranchAddress("kinetic_energy", &kinetic_energy, &b_kinetic_energy);
   fChain->SetBranchAddress("edep", &edep, &b_edep);
   fChain->SetBranchAddress("numelectrons", &numelectrons, &b_numelectrons);
   fChain->SetBranchAddress("shower_edep", &shower_edep, &b_shower_edep);
   fChain->SetBranchAddress("shower_numelectrons", &shower_numelectrons, &b_shower_numelectrons);
   fChain->SetBranchAddress("process", &process, &b_process);
}

void mcparticles::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t mcparticles::Cut(Long64_t entry)
{
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
