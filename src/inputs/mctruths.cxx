//////////////////////////////////////////////////////////
// Implementation for the mctruths class.
// Declarations live in mctruths.h.
//////////////////////////////////////////////////////////

// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 20:59:41 UTC
#include "mctruths.h"
#include <iostream>

mctruths::mctruths(TTree *tree) : fChain(0)
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root");
      }
      TDirectory * dir = (TDirectory*)f->Get("prodmarley_nue_flat_es_dune10kt_1x2x2__trg_mc_2025a_tpg__tpg_dune10kt_1x2x2__triggerAna__v10_12_01d01__triggerana_dune10kt_1x2x2__v10_12_01d01_merged_FLAT-ES_prod_v2-pass2_20251124T190722_f0_tpstream.root:/triggerAna");
      dir->GetObject("mctruths",tree);

   }
   Init(tree);
}

mctruths::mctruths(const mctruths& other)
   : fChain(nullptr), fCurrent(-1), generator_name(nullptr), process(nullptr)
{
   event = other.event;
   run = other.run;
   subrun = other.subrun;
   block_id = other.block_id;
   truth_track_id = other.truth_track_id;
   pdg = other.pdg;
   status_code = other.status_code;
   x = other.x;
   y = other.y;
   z = other.z;
   px = other.px;
   py = other.py;
   pz = other.pz;
   p = other.p;
   energy = other.energy;
   kinetic_energy = other.kinetic_energy;

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
   b_block_id = nullptr;
   b_truth_track_id = nullptr;
   b_pdg = nullptr;
   b_generator_name = nullptr;
   b_status_code = nullptr;
   b_x = nullptr;
   b_y = nullptr;
   b_z = nullptr;
   b_px = nullptr;
   b_py = nullptr;
   b_pz = nullptr;
   b_p = nullptr;
   b_energy = nullptr;
   b_kinetic_energy = nullptr;
   b_process = nullptr;
}

mctruths::~mctruths()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t mctruths::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t mctruths::LoadTree(Long64_t entry)
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

void mctruths::Init(TTree *tree)
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
   fChain->SetBranchAddress("block_id", &block_id, &b_block_id);
   fChain->SetBranchAddress("truth_track_id", &truth_track_id, &b_truth_track_id);
   fChain->SetBranchAddress("pdg", &pdg, &b_pdg);
   fChain->SetBranchAddress("generator_name", &generator_name, &b_generator_name);
   fChain->SetBranchAddress("status_code", &status_code, &b_status_code);
   fChain->SetBranchAddress("x", &x, &b_x);
   fChain->SetBranchAddress("y", &y, &b_y);
   fChain->SetBranchAddress("z", &z, &b_z);
   fChain->SetBranchAddress("px", &px, &b_px);
   fChain->SetBranchAddress("py", &py, &b_py);
   fChain->SetBranchAddress("pz", &pz, &b_pz);
   fChain->SetBranchAddress("p", &p, &b_p);
   fChain->SetBranchAddress("energy", &energy, &b_energy);
   fChain->SetBranchAddress("kinetic_energy", &kinetic_energy, &b_kinetic_energy);
   fChain->SetBranchAddress("process", &process, &b_process);
}

void mctruths::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t mctruths::Cut(Long64_t entry)
{
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}

void mctruths::Print() const
{
   std::cout << "mctruths entry:\n";
   std::cout << "  event: " << event << "\n";
   std::cout << "  run: " << run << "\n";
   std::cout << "  subrun: " << subrun << "\n";
   std::cout << "  block_id: " << block_id << "\n";
   std::cout << "  truth_track_id: " << truth_track_id << "\n";
   std::cout << "  pdg: " << pdg << "\n";
   std::cout << "  generator_name: " << (generator_name ? *generator_name : std::string("(null)")) << "\n";
   std::cout << "  status_code: " << status_code << "\n";
   std::cout << "  x: " << x << "\n";
   std::cout << "  y: " << y << "\n";
   std::cout << "  z: " << z << "\n";
   std::cout << "  px: " << px << "\n";
   std::cout << "  py: " << py << "\n";
   std::cout << "  pz: " << pz << "\n";
   std::cout << "  p: " << p << "\n";
   std::cout << "  energy: " << energy << "\n";
   std::cout << "  kinetic_energy: " << kinetic_energy << "\n";
   std::cout << "  process: " << (process ? *process : std::string("(null)")) << "\n";
   std::cout << std::flush;
}
