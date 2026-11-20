//////////////////////////////////////////////////////////
// Implementation for the newtps class.
// Declarations live in newtps.h.
//////////////////////////////////////////////////////////

// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-16 at 03:39:35 UTC
#include "newtps.h"
#include <iostream>

newtps::newtps(TTree *tree) : fChain(0)
{

   Init(tree);
}

newtps::newtps(const newtps& other)
   : fChain(nullptr), fCurrent(-1),
     view(nullptr), generator_name(nullptr),
     particle_process(nullptr), neutrino_interaction(nullptr)
{
   event = other.event;
   run = other.run;
   version = other.version;
   detid = other.detid;
   channel = other.channel;
   samples_over_threshold = other.samples_over_threshold;
   time_start = other.time_start;
   samples_to_peak = other.samples_to_peak;
   adc_integral = other.adc_integral;
   adc_peak = other.adc_peak;
   detector = other.detector;
   detector_channel = other.detector_channel;
   simide_energy = other.simide_energy;
   particle_pdg = other.particle_pdg;
   particle_energy = other.particle_energy;
   particle_x = other.particle_x;
   particle_y = other.particle_y;
   particle_z = other.particle_z;
   particle_px = other.particle_px;
   particle_py = other.particle_py;
   particle_pz = other.particle_pz;
   neutrino_x = other.neutrino_x;
   neutrino_y = other.neutrino_y;
   neutrino_z = other.neutrino_z;
   neutrino_px = other.neutrino_px;
   neutrino_py = other.neutrino_py;
   neutrino_pz = other.neutrino_pz;
   neutrino_energy = other.neutrino_energy;

   if (other.view) {
      owned_view_.reset(new std::string(*other.view));
      view = owned_view_.get();
   }
   if (other.generator_name) {
      owned_generator_name_.reset(new std::string(*other.generator_name));
      generator_name = owned_generator_name_.get();
   }
   if (other.particle_process) {
      owned_particle_process_.reset(new std::string(*other.particle_process));
      particle_process = owned_particle_process_.get();
   }
   if (other.neutrino_interaction) {
      owned_neutrino_interaction_.reset(new std::string(*other.neutrino_interaction));
      neutrino_interaction = owned_neutrino_interaction_.get();
   }

   b_event = nullptr;
   b_run = nullptr;
   b_version = nullptr;
   b_detid = nullptr;
   b_channel = nullptr;
   b_samples_over_threshold = nullptr;
   b_time_start = nullptr;
   b_samples_to_peak = nullptr;
   b_adc_integral = nullptr;
   b_adc_peak = nullptr;
   b_detector = nullptr;
   b_detector_channel = nullptr;
   b_view = nullptr;
   b_simide_energy = nullptr;
   b_generator_name = nullptr;
   b_particle_pdg = nullptr;
   b_particle_process = nullptr;
   b_particle_energy = nullptr;
   b_particle_x = nullptr;
   b_particle_y = nullptr;
   b_particle_z = nullptr;
   b_particle_px = nullptr;
   b_particle_py = nullptr;
   b_particle_pz = nullptr;
   b_neutrino_interaction = nullptr;
   b_neutrino_x = nullptr;
   b_neutrino_y = nullptr;
   b_neutrino_z = nullptr;
   b_neutrino_px = nullptr;
   b_neutrino_py = nullptr;
   b_neutrino_pz = nullptr;
   b_neutrino_energy = nullptr;
}

newtps::~newtps()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t newtps::GetEntry(Long64_t entry)
{
// Read contents of entry: fills all the leaf-type members above.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t newtps::LoadTree(Long64_t entry)
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

void newtps::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.

   // Set object pointer
   view = 0;
   generator_name = 0;
   particle_process = 0;
   neutrino_interaction = 0;
   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("event", &event, &b_event);
   fChain->SetBranchAddress("run", &run, &b_run);
   fChain->SetBranchAddress("version", &version, &b_version);
   fChain->SetBranchAddress("detid", &detid, &b_detid);
   fChain->SetBranchAddress("channel", &channel, &b_channel);
   fChain->SetBranchAddress("samples_over_threshold", &samples_over_threshold, &b_samples_over_threshold);
   fChain->SetBranchAddress("time_start", &time_start, &b_time_start);
   fChain->SetBranchAddress("samples_to_peak", &samples_to_peak, &b_samples_to_peak);
   fChain->SetBranchAddress("adc_integral", &adc_integral, &b_adc_integral);
   fChain->SetBranchAddress("adc_peak", &adc_peak, &b_adc_peak);
   fChain->SetBranchAddress("detector", &detector, &b_detector);
   fChain->SetBranchAddress("detector_channel", &detector_channel, &b_detector_channel);
   fChain->SetBranchAddress("view", &view, &b_view);
   fChain->SetBranchAddress("simide_energy", &simide_energy, &b_simide_energy);
   fChain->SetBranchAddress("generator_name", &generator_name, &b_generator_name);
   fChain->SetBranchAddress("particle_pdg", &particle_pdg, &b_particle_pdg);
   fChain->SetBranchAddress("particle_process", &particle_process, &b_particle_process);
   fChain->SetBranchAddress("particle_energy", &particle_energy, &b_particle_energy);
   fChain->SetBranchAddress("particle_x", &particle_x, &b_particle_x);
   fChain->SetBranchAddress("particle_y", &particle_y, &b_particle_y);
   fChain->SetBranchAddress("particle_z", &particle_z, &b_particle_z);
   fChain->SetBranchAddress("particle_px", &particle_px, &b_particle_px);
   fChain->SetBranchAddress("particle_py", &particle_py, &b_particle_py);
   fChain->SetBranchAddress("particle_pz", &particle_pz, &b_particle_pz);
   fChain->SetBranchAddress("neutrino_interaction", &neutrino_interaction, &b_neutrino_interaction);
   fChain->SetBranchAddress("neutrino_x", &neutrino_x, &b_neutrino_x);
   fChain->SetBranchAddress("neutrino_y", &neutrino_y, &b_neutrino_y);
   fChain->SetBranchAddress("neutrino_z", &neutrino_z, &b_neutrino_z);
   fChain->SetBranchAddress("neutrino_px", &neutrino_px, &b_neutrino_px);
   fChain->SetBranchAddress("neutrino_py", &neutrino_py, &b_neutrino_py);
   fChain->SetBranchAddress("neutrino_pz", &neutrino_pz, &b_neutrino_pz);
   fChain->SetBranchAddress("neutrino_energy", &neutrino_energy, &b_neutrino_energy);
}

void newtps::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t newtps::Cut(Long64_t entry)
{
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}

void newtps::Print() const
{
   std::cout << "newtps entry:\n";
   std::cout << "  event: " << event << "\n";
   std::cout << "  run: " << run << "\n";
   std::cout << "  version: " << version << "\n";
   std::cout << "  detid: " << detid << "\n";
   std::cout << "  channel: " << channel << "\n";
   std::cout << "  samples_over_threshold: " << samples_over_threshold << "\n";
   std::cout << "  time_start: " << time_start << "\n";
   std::cout << "  samples_to_peak: " << samples_to_peak << "\n";
   std::cout << "  adc_integral: " << adc_integral << "\n";
   std::cout << "  adc_peak: " << adc_peak << "\n";
   std::cout << "  detector: " << detector << "\n";
   std::cout << "  detector_channel: " << detector_channel << "\n";
   std::cout << "  view: " << (view ? *view : std::string("(null)")) << "\n";
   std::cout << "  simide_energy: " << simide_energy << "\n";
   std::cout << "  generator_name: " << (generator_name ? *generator_name : std::string("(null)")) << "\n";
   std::cout << "  particle_pdg: " << particle_pdg << "\n";
   std::cout << "  particle_process: " << (particle_process ? *particle_process : std::string("(null)")) << "\n";
   std::cout << "  particle_energy: " << particle_energy << "\n";
   std::cout << "  particle_x: " << particle_x << "\n";
   std::cout << "  particle_y: " << particle_y << "\n";
   std::cout << "  particle_z: " << particle_z << "\n";
   std::cout << "  particle_px: " << particle_px << "\n";
   std::cout << "  particle_py: " << particle_py << "\n";
   std::cout << "  particle_pz: " << particle_pz << "\n";
   std::cout << "  neutrino_interaction: " << (neutrino_interaction ? *neutrino_interaction : std::string("(null)")) << "\n";
   std::cout << "  neutrino_x: " << neutrino_x << "\n";
   std::cout << "  neutrino_y: " << neutrino_y << "\n";
   std::cout << "  neutrino_z: " << neutrino_z << "\n";
   std::cout << "  neutrino_px: " << neutrino_px << "\n";
   std::cout << "  neutrino_py: " << neutrino_py << "\n";
   std::cout << "  neutrino_pz: " << neutrino_pz << "\n";
   std::cout << "  neutrino_energy: " << neutrino_energy << "\n";
   std::cout << std::flush;
}
