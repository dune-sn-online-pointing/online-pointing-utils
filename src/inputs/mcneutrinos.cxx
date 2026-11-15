//////////////////////////////////////////////////////////
// Implementation for the mcneutrinos class.
// Declarations live in mcneutrinos.h.
//////////////////////////////////////////////////////////

// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 20:59:41 UTC
#include "mcneutrinos.h"
#include <iostream>

mcneutrinos::mcneutrinos(TTree *tree) : fChain(0)
{

   Init(tree);
}

mcneutrinos::mcneutrinos(const mcneutrinos& other)
   : fChain(nullptr), fCurrent(-1), generator_name(nullptr)
{
   event = other.event;
   run = other.run;
   subrun = other.subrun;
   block_id = other.block_id;
   nupdg = other.nupdg;
   leptonpdg = other.leptonpdg;
   ccnc = other.ccnc;
   mode = other.mode;
   interactionType = other.interactionType;
   target = other.target;
   hitnuc = other.hitnuc;
   hitquark = other.hitquark;
   w = other.w;
   x = other.x;
   y = other.y;
   qsqr = other.qsqr;
   pt = other.pt;
   theta = other.theta;

   if (other.generator_name) {
      owned_generator_name_.reset(new std::string(*other.generator_name));
      generator_name = owned_generator_name_.get();
   }

   b_event = nullptr;
   b_run = nullptr;
   b_subrun = nullptr;
   b_block_id = nullptr;
   b_generator_name = nullptr;
   b_nupdg = nullptr;
   b_leptonpdg = nullptr;
   b_ccnc = nullptr;
   b_mode = nullptr;
   b_interactionType = nullptr;
   b_target = nullptr;
   b_hitnuc = nullptr;
   b_hitquark = nullptr;
   b_w = nullptr;
   b_x = nullptr;
   b_y = nullptr;
   b_qsqr = nullptr;
   b_pt = nullptr;
   b_theta = nullptr;
}

mcneutrinos::~mcneutrinos()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t mcneutrinos::GetEntry(Long64_t entry)
{
// Read contents of entry: fills all the leaf-type members above.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t mcneutrinos::LoadTree(Long64_t entry)
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

void mcneutrinos::Init(TTree *tree)
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
   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("event", &event, &b_event);
   fChain->SetBranchAddress("run", &run, &b_run);
   fChain->SetBranchAddress("subrun", &subrun, &b_subrun);
   fChain->SetBranchAddress("block_id", &block_id, &b_block_id);
   fChain->SetBranchAddress("generator_name", &generator_name, &b_generator_name);
   fChain->SetBranchAddress("nupdg", &nupdg, &b_nupdg);
   fChain->SetBranchAddress("leptonpdg", &leptonpdg, &b_leptonpdg);
   fChain->SetBranchAddress("ccnc", &ccnc, &b_ccnc);
   fChain->SetBranchAddress("mode", &mode, &b_mode);
   fChain->SetBranchAddress("interactionType", &interactionType, &b_interactionType);
   fChain->SetBranchAddress("target", &target, &b_target);
   fChain->SetBranchAddress("hitnuc", &hitnuc, &b_hitnuc);
   fChain->SetBranchAddress("hitquark", &hitquark, &b_hitquark);
   fChain->SetBranchAddress("w", &w, &b_w);
   fChain->SetBranchAddress("x", &x, &b_x);
   fChain->SetBranchAddress("y", &y, &b_y);
   fChain->SetBranchAddress("qsqr", &qsqr, &b_qsqr);
   fChain->SetBranchAddress("pt", &pt, &b_pt);
   fChain->SetBranchAddress("theta", &theta, &b_theta);
}

void mcneutrinos::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t mcneutrinos::Cut(Long64_t entry)
{
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}

void mcneutrinos::Print() const
{
   std::cout << "mcneutrinos entry:\n";
   std::cout << "  event: " << event << "\n";
   std::cout << "  run: " << run << "\n";
   std::cout << "  subrun: " << subrun << "\n";
   std::cout << "  block_id: " << block_id << "\n";
   std::cout << "  generator_name: " << (generator_name ? *generator_name : std::string("(null)")) << "\n";
   std::cout << "  nupdg: " << nupdg << "\n";
   std::cout << "  leptonpdg: " << leptonpdg << "\n";
   std::cout << "  ccnc: " << ccnc << "\n";
   std::cout << "  mode: " << mode << "\n";
   std::cout << "  interactionType: " << interactionType << "\n";
   std::cout << "  target: " << target << "\n";
   std::cout << "  hitnuc: " << hitnuc << "\n";
   std::cout << "  hitquark: " << hitquark << "\n";
   std::cout << "  w: " << w << "\n";
   std::cout << "  x: " << x << "\n";
   std::cout << "  y: " << y << "\n";
   std::cout << "  qsqr: " << qsqr << "\n";
   std::cout << "  pt: " << pt << "\n";
   std::cout << "  theta: " << theta << "\n";
   std::cout << std::flush;
}
