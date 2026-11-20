// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-18 at 20:47:56 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-18 at 20:51:01 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-18 at 21:03:11 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-18 at 21:08:01 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-18 at 21:09:12 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-18 at 21:46:34 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 21:26:37 UTC (ignore events not in both files)
//
// Channel-by-channel comparison of the new and old flat-TP trees, via the
// newtps/oldtps reader classes. For every channel that appears in EITHER file
// it reports discrepancies:
//   - TP entries present on only one side (missing from the other)
//   - channels missing entirely from one side
//   - for entries present on both sides, the differing fields with their
//     old/new values
// Channels that agree completely are not printed.
//
// The full per-channel report is written to a text file (so you can grep/dig
// around in it); only the summary and the output path are echoed to the
// terminal.
//
//   export NEWTPS=/path/to/..._tpstream_tps.root
//   export OLDTPS=/path/to/..._s0_l10_tps.root
//   export OLDRUN=1843          # optional, see note below
//   export CMPOUT=mycompare.txt # optional, default compare_bychannel.txt
//   cd src/inputs
//   root -l -b -q 'compare_tps_bychannel.C+'
//
// Matching is by (run, event, time_start) within each channel -- NOT by row
// order (channels are not ordered the same way in the two files) and NOT by
// event alone (the same event number can recur across runs). Integer fields
// are compared exactly; floating fields with a tolerance (old is Float_t, new
// is Double_t).
//
// NOTE: the OLD tps format has no `run` branch, only `event`. So the old side
// is given a single run number: $OLDRUN if set, otherwise the sole run found in
// the NEW file. If the NEW file contains more than one run and $OLDRUN is not
// set, the macro stops and asks you to set it.

#include "oldtps.cxx"
#include "newtps.cxx"

#include "TFile.h"
#include "TTree.h"
#include "TSystem.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>

static bool dclose(double a, double b, double rtol = 1e-4, double atol = 1e-6) {
   return std::fabs(a - b) <= atol + rtol * std::max(std::fabs(a), std::fabs(b));
}

// Factor-of-1000 tolerant: also treats a~=1000*b or b~=1000*a as equal, to
// absorb the known old-code MeV/GeV unit bug on energy/momentum fields.
static bool dclose_k(double a, double b) {
   return dclose(a, b) || dclose(a, 1000.0 * b) || dclose(1000.0 * a, b);
}

// Per-TP key within a channel: (run, event, time_start).
using Key = std::tuple<long long, long long, ULong64_t>;

void compare_tps_bychannel() {
   const char* newpath = gSystem->Getenv("NEWTPS");
   const char* oldpath = gSystem->Getenv("OLDTPS");
   if (!newpath || !oldpath) {
      std::cerr << "Set $NEWTPS and $OLDTPS to the two _tps.root files.\n";
      return;
   }

   // Output text file (overridable via $CMPOUT).
   const char* outenv = gSystem->Getenv("CMPOUT");
   std::string outname = (outenv && outenv[0] != '\0') ? outenv : "compare_bychannel.txt";
   std::ofstream out(outname);
   if (!out) { std::cerr << "Cannot open output file: " << outname << "\n"; return; }

   // $IGNORE_KFACTOR=1 : treat energy/momentum fields that differ by a factor
   // of ~1000 as matching (the known old-code MeV/GeV bug). Positions and other
   // fields are always compared strictly.
   const char* kenv = gSystem->Getenv("IGNORE_KFACTOR");
   const bool ignore_k = (kenv && kenv[0] != '\0' && kenv[0] != '0');

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

   out << "NEWTPS " << newpath << "\n  entries: " << tnew->GetEntries() << "\n";
   out << "OLDTPS " << oldpath << "\n  entries: " << told->GetEntries() << "\n";
   out << "IGNORE_KFACTOR: " << (ignore_k ? "on (energy/momentum x1000 tolerated)" : "off (strict)") << "\n";

   // Group the NEW file by channel -> ((run,event,time_start) -> entry index),
   // and collect the set of runs present.
   std::map<UInt_t, std::map<Key, Long64_t>> newByCh, oldByCh;
   std::set<long long> newRuns;
   std::set<std::pair<long long, long long>> evNew, evOld;  // (run,event) present per file
   for (Long64_t i = 0; i < tnew->GetEntries(); ++i) {
      rnew.GetEntry(i);
      newRuns.insert((long long)rnew.run);
      evNew.insert({(long long)rnew.run, (long long)rnew.event});
      newByCh[rnew.channel][Key((long long)rnew.run, (long long)rnew.event, rnew.time_start)] = i;
   }

   // The OLD format has no run branch; assign it a single run.
   long long oldrun = 0;
   const char* oldrun_env = gSystem->Getenv("OLDRUN");
   if (oldrun_env && oldrun_env[0] != '\0') {
      oldrun = std::atoll(oldrun_env);
      out << "OLD run: " << oldrun << " (from $OLDRUN)\n";
   } else if (newRuns.size() == 1) {
      oldrun = *newRuns.begin();
      out << "OLD run: " << oldrun << " (sole run in NEW; set $OLDRUN to override)\n";
   } else {
      std::cerr << "NEW file has " << newRuns.size()
                << " runs; OLD has no run branch. Set $OLDRUN to the old file's run.\n";
      rnew.fChain = nullptr;
      return;
   }
   out << "\n";

   for (Long64_t j = 0; j < told->GetEntries(); ++j) {
      rold.GetEntry(j);
      evOld.insert({oldrun, (long long)rold.event});
      oldByCh[rold.channel][Key(oldrun, (long long)rold.event, rold.time_start)] = j;
   }

   // Only compare (run,event)s present in BOTH files; events in just one file
   // (e.g. the old file has more events than the new one processed) are ignored.
   std::set<std::pair<long long, long long>> evCommon;
   for (const auto& re : evNew) if (evOld.count(re)) evCommon.insert(re);

   // Union of channels.
   std::set<UInt_t> channels;
   for (const auto& kv : newByCh) channels.insert(kv.first);
   for (const auto& kv : oldByCh) channels.insert(kv.first);

   long ch_reported = 0, ch_only_old = 0, ch_only_new = 0;
   long tp_only_old = 0, tp_only_new = 0, tp_both = 0, tp_diff = 0;
   long tp_skipped_event = 0;  // TPs in a (run,event) not present in both files

   for (UInt_t ch : channels) {
      const bool inOld = oldByCh.count(ch) != 0;
      const bool inNew = newByCh.count(ch) != 0;
      if (inOld && !inNew) ++ch_only_old;
      if (inNew && !inOld) ++ch_only_new;

      // Union of (run,event,time_start) keys on this channel.
      std::set<Key> keys;
      if (inOld) for (const auto& kv : oldByCh[ch]) keys.insert(kv.first);
      if (inNew) for (const auto& kv : newByCh[ch]) keys.insert(kv.first);

      std::ostringstream lines;
      for (const Key& k : keys) {
         // Ignore TPs whose (run,event) isn't in both files entirely.
         if (!evCommon.count({std::get<0>(k), std::get<1>(k)})) { ++tp_skipped_event; continue; }

         const bool o = inOld && oldByCh[ch].count(k);
         const bool n = inNew && newByCh[ch].count(k);

         // Skip entries present on only one side (still counted for the
         // summary, just not printed).
         if (o && !n) { ++tp_only_old; continue; }
         if (n && !o) { ++tp_only_new; continue; }

         ++tp_both;
         rold.GetEntry(oldByCh[ch][k]);
         rnew.GetEntry(newByCh[ch][k]);

         // Build a FULL old-vs-new dump of every field; mark the ones that
         // differ. `any` records whether this TP is discrepant at all.
         bool any = false;
         std::ostringstream full;
         // Aligned columns so same/different is easy to scan down the block.
         auto emit = [&](const char* nm, const std::string& a, const std::string& b, bool diff) {
            full << "      " << std::left << std::setw(24) << nm
                 << std::setw(22) << ("old=" + a)
                 << std::setw(22) << ("new=" + b);
            if (diff) { full << "   <<< DIFF"; any = true; }
            full << "\n";
         };
         auto fi = [&](const char* nm, long long a, long long b) {
            std::ostringstream sa, sb; sa << a; sb << b; emit(nm, sa.str(), sb.str(), a != b);
         };
         auto ff = [&](const char* nm, double a, double b) {
            std::ostringstream sa, sb; sa << a; sb << b; emit(nm, sa.str(), sb.str(), !dclose(a, b));
         };
         // Energy/momentum fields: optionally tolerate the x1000 MeV/GeV bug.
         auto ffk = [&](const char* nm, double a, double b) {
            const bool same = ignore_k ? dclose_k(a, b) : dclose(a, b);
            std::ostringstream sa, sb; sa << a; sb << b; emit(nm, sa.str(), sb.str(), !same);
         };
         auto fs = [&](const char* nm, const std::string* a, const std::string* b) {
            std::string sa = a ? *a : "", sb = b ? *b : "";
            emit(nm, "'" + sa + "'", "'" + sb + "'", sa != sb);
         };

         fi("version", rold.version, rnew.version);
         fi("detid", rold.detid, rnew.detid);
         fi("detector", rold.detector, rnew.detector);
         fi("detector_channel", rold.detector_channel, rnew.detector_channel);
         fi("samples_over_threshold", rold.samples_over_threshold, rnew.samples_over_threshold);
         fi("samples_to_peak", rold.samples_to_peak, rnew.samples_to_peak);
         fi("adc_integral", rold.adc_integral, rnew.adc_integral);
         fi("adc_peak", rold.adc_peak, rnew.adc_peak);
         fi("particle_pdg", rold.particle_pdg, rnew.particle_pdg);
         fs("view", rold.view, rnew.view);
         fs("generator_name", rold.generator_name, rnew.generator_name);
         fs("particle_process", rold.particle_process, rnew.particle_process);
         fs("neutrino_interaction", rold.neutrino_interaction, rnew.neutrino_interaction);
         ffk("simide_energy", rold.simide_energy, rnew.simide_energy);
         ffk("particle_energy", rold.particle_energy, rnew.particle_energy);
         ff("particle_x", rold.particle_x, rnew.particle_x);
         ff("particle_y", rold.particle_y, rnew.particle_y);
         ff("particle_z", rold.particle_z, rnew.particle_z);
         ffk("particle_px", rold.particle_px, rnew.particle_px);
         ffk("particle_py", rold.particle_py, rnew.particle_py);
         ffk("particle_pz", rold.particle_pz, rnew.particle_pz);
         ff("neutrino_x", rold.neutrino_x, rnew.neutrino_x);
         ff("neutrino_y", rold.neutrino_y, rnew.neutrino_y);
         ff("neutrino_z", rold.neutrino_z, rnew.neutrino_z);
         ffk("neutrino_px", rold.neutrino_px, rnew.neutrino_px);
         ffk("neutrino_py", rold.neutrino_py, rnew.neutrino_py);
         ffk("neutrino_pz", rold.neutrino_pz, rnew.neutrino_pz);
         ffk("neutrino_energy", rold.neutrino_energy, rnew.neutrino_energy);

         if (any) {
            ++tp_diff;
            lines << "    run " << std::get<0>(k) << " event " << std::get<1>(k)
                  << " time_start " << std::get<2>(k) << ":\n" << full.str();
         }
      }

      const std::string body = lines.str();
      if (!body.empty()) {
         ++ch_reported;
         out << "channel " << ch << ":\n" << body;
      }
   }

   // Summary -> both the file and the terminal.
   std::ostringstream s;
   s << "\n=== summary ===\n";
   s << "runs in NEW:";
   for (long long r : newRuns) s << " " << r;
   s << "   (OLD treated as run " << oldrun << ")\n";
   s << "(run,event) in NEW: " << evNew.size() << ",  in OLD: " << evOld.size()
     << ",  common (compared): " << evCommon.size() << "\n";
   s << "TPs ignored (event not in both files): " << tp_skipped_event << "\n";
   s << "channels in NEW: " << newByCh.size()
     << ",  in OLD: " << oldByCh.size()
     << ",  union: " << channels.size() << "\n";
   s << "channels only in OLD: " << ch_only_old
     << ",  only in NEW: " << ch_only_new << "\n";
   s << "channels with something to report: " << ch_reported << "\n";
   s << "TP entries OLD-only: " << tp_only_old
     << ",  NEW-only: " << tp_only_new << "\n";
   s << "TP entries on both sides: " << tp_both
     << "  (of which differ in >=1 field: " << tp_diff << ")\n";

   out << s.str();
   out.close();

   std::cout << s.str();
   std::cout << "\nFull per-channel report written to: " << outname << "\n";

   // Detach so the reader destructors don't delete the still-referenced TFiles.
   rnew.fChain = nullptr;
   rold.fChain = nullptr;
}
