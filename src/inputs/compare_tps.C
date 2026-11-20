// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-16 at 03:48:28 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 19:19:50 UTC
//
// Short ROOT macro: load the new and old flat-TP trees via the newtps/oldtps
// reader classes and compare them field-by-field.
//
//   export NEWTPS=/path/to/..._tpstream_tps.root   (new read_trigger_trees output)
//   export OLDTPS=/path/to/..._s0_l1_tps.root       (old internal-backtracking output)
//   cd src/inputs
//   root -l -b -q 'compare_tps.C+'
//
// The two files may have different numbers of events, so TPs are matched on
// (channel, time_start) rather than by entry number. Integer fields are
// compared exactly; floating fields with a tolerance (old is Float_t, new is
// Double_t, so exact equality is not expected).

#include "oldtps.cxx"
#include "newtps.cxx"

#include "TFile.h"
#include "TTree.h"
#include "TSystem.h"

#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <utility>

static bool dclose(double a, double b, double rtol = 1e-4, double atol = 1e-6) {
   return std::fabs(a - b) <= atol + rtol * std::max(std::fabs(a), std::fabs(b));
}

void compare_tps() {
   const char* newpath = gSystem->Getenv("NEWTPS");
   const char* oldpath = gSystem->Getenv("OLDTPS");
   if (!newpath || !oldpath) {
      std::cerr << "Set $NEWTPS and $OLDTPS to the two _tps.root files.\n";
      return;
   }

   TFile* fnew = TFile::Open(newpath, "READ");
   TFile* fold = TFile::Open(oldpath, "READ");
   if (!fnew || fnew->IsZombie()) { std::cerr << "Cannot open NEWTPS: " << newpath << "\n"; return; }
   if (!fold || fold->IsZombie()) { std::cerr << "Cannot open OLDTPS: " << oldpath << "\n"; return; }

   TTree* tnew = nullptr; fnew->GetObject("tps", tnew);
   TTree* told = nullptr; fold->GetObject("tps", told);
   if (!tnew) { std::cerr << "No 'tps' tree in NEWTPS\n"; return; }
   if (!told) { std::cerr << "No 'tps' tree in OLDTPS\n"; return; }

   newtps rnew(tnew);
   oldtps rold(told);

   std::cout << "NEWTPS " << newpath << "\n  entries: " << tnew->GetEntries() << "\n";
   std::cout << "OLDTPS " << oldpath << "\n  entries: " << told->GetEntries() << "\n\n";

   // Index the NEW tree by (channel, time_start).
   std::map<std::pair<UInt_t, ULong64_t>, Long64_t> idx;
   for (Long64_t i = 0; i < tnew->GetEntries(); ++i) {
      rnew.GetEntry(i);
      idx[{rnew.channel, rnew.time_start}] = i;
   }

   long matched = 0, unmatched = 0, shown = 0;
   std::map<std::string, long> mism;   // field -> number of matched TPs that differ

   for (Long64_t j = 0; j < told->GetEntries(); ++j) {
      rold.GetEntry(j);
      auto it = idx.find({rold.channel, rold.time_start});
      if (it == idx.end()) { ++unmatched; continue; }
      rnew.GetEntry(it->second);
      ++matched;

      // Three little field-comparison helpers used for the per-field calls
      // below. Each takes a field name `n` plus the old and new value, and if
      // they disagree it (a) bumps that field's running mismatch tally in the
      // map `mism[n]`, and (b) sets `any` so we know this TP differed in at
      // least one field. `[&]` lets the lambdas see/modify `mism`, `any`, etc.
      // in the enclosing scope.
      bool any = false;                           // did THIS matched TP differ anywhere?
      // ci = compare ints: exact inequality.
      auto ci = [&](const char* n, long long a, long long b) { if (a != b) { mism[n]++; any = true; } };
      // cf = compare floats: "differ" means NOT within tolerance (dclose),
      // since old is Float_t and new is Double_t so exact equality is wrong.
      auto cf = [&](const char* n, double a, double b) { if (!dclose(a, b)) { mism[n]++; any = true; } };
      // cs = compare strings held by pointer (the reader classes store
      // std::string*): treat a null pointer as "", then compare the text.
      auto cs = [&](const char* n, const std::string* a, const std::string* b) {
         std::string sa = a ? *a : "", sb = b ? *b : "";
         if (sa != sb) { mism[n]++; any = true; }
      };

      ci("version", rold.version, rnew.version);
      ci("detid", rold.detid, rnew.detid);
      ci("detector", rold.detector, rnew.detector);
      ci("detector_channel", rold.detector_channel, rnew.detector_channel);
      ci("samples_over_threshold", rold.samples_over_threshold, rnew.samples_over_threshold);
      ci("samples_to_peak", rold.samples_to_peak, rnew.samples_to_peak);
      ci("adc_integral", rold.adc_integral, rnew.adc_integral);
      ci("adc_peak", rold.adc_peak, rnew.adc_peak);
      ci("particle_pdg", rold.particle_pdg, rnew.particle_pdg);
      cs("view", rold.view, rnew.view);
      cs("generator_name", rold.generator_name, rnew.generator_name);
      cs("particle_process", rold.particle_process, rnew.particle_process);
      cs("neutrino_interaction", rold.neutrino_interaction, rnew.neutrino_interaction);
      cf("simide_energy", rold.simide_energy, rnew.simide_energy);
      cf("particle_energy", rold.particle_energy, rnew.particle_energy);
      cf("particle_x", rold.particle_x, rnew.particle_x);
      cf("particle_y", rold.particle_y, rnew.particle_y);
      cf("particle_z", rold.particle_z, rnew.particle_z);
      cf("particle_px", rold.particle_px, rnew.particle_px);
      cf("particle_py", rold.particle_py, rnew.particle_py);
      cf("particle_pz", rold.particle_pz, rnew.particle_pz);
      cf("neutrino_x", rold.neutrino_x, rnew.neutrino_x);
      cf("neutrino_y", rold.neutrino_y, rnew.neutrino_y);
      cf("neutrino_z", rold.neutrino_z, rnew.neutrino_z);
      cf("neutrino_px", rold.neutrino_px, rnew.neutrino_px);
      cf("neutrino_py", rold.neutrino_py, rnew.neutrino_py);
      cf("neutrino_pz", rold.neutrino_pz, rnew.neutrino_pz);
      cf("neutrino_energy", rold.neutrino_energy, rnew.neutrino_energy);

      if (any && shown < 10) {
         std::cout << "MISMATCH  channel=" << rold.channel << " time_start=" << rold.time_start
                   << "  (old event=" << rold.event << ", new event=" << rnew.event
                   << " run=" << rnew.run << ")\n";
         ++shown;
      }
   }

   std::cout << "\n=== summary ===\n";
   std::cout << "matched TPs (by channel,time_start): " << matched << "\n";
   std::cout << "old TPs with no new match:           " << unmatched << "\n";
   if (mism.empty()) {
      std::cout << "no field mismatches among matched TPs\n";
   } else {
      std::cout << "field mismatches (count over matched TPs):\n";
      for (const auto& kv : mism) std::cout << "  " << kv.first << ": " << kv.second << "\n";
   }

   // Detach so the reader destructors don't delete the still-referenced TFiles.
   rnew.fChain = nullptr;
   rold.fChain = nullptr;
}
