// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 19:38:23 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 21:12:34 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-18 at 20:58:12 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-18 at 21:34:17 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 19:39:40 UTC (debug bt_* branches)
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 19:55:39 UTC (SetSimideEnergy)
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 21:05:00 UTC (neutrino_interaction filename tag)
#ifndef WRITE_TPS_CLASS_H
#define WRITE_TPS_CLASS_H

#include <string>
#include <vector>
#include "TFile.h"
#include "TTree.h"
#include "TriggerPrimitive.hpp"
#include "TString.h"
#include "mcparticles.h"
#include "mctruths.h"
#include "mcneutrinos.h"
#include "tpstream.h"
// #include "TrueParticle.h"
// #include "Neutrino.h"


class TPSClass
{
public:
    //TPSClass();
    bool Create(TString filename, TString treename);
    bool Close();
    // TTree* GetTree();
    // TFile* GetFile();
    // The reader classes (mcparticles/mctruths/mcneutrinos) have a deleted
    // operator=, so we store non-owning pointers to the caller's objects
    // rather than copies. They must outlive the next WriteSingleEvent() call.
    void SetParticle(const mcparticles& particle)       { theparticle = &particle; }
    void SetIncomingNeutrino(const mctruths& neutrino)   { theincoming_neutrino = &neutrino; }
    void SetOutgoingLepton(const mctruths& lepton)       { theoutgoing_lepton = &lepton; }
    void SetNeutrino(const mcneutrinos& neutrino)        { theneutrino = &neutrino; }
    void SetTruth(const mctruths& truth)                 { thetruth = &truth; }
    void SetPrimitive(const TriggerPrimitive& primitive) { theprimitive = &primitive; }
    // DEBUG: stash the raw backtracking fractions straight from the tpstream so
    // they get written to the _tps tree (for comparing match quality). Harmless
    // extra branches; remove when done.
    void SetBtDebug(Double_t efrac, Double_t nfrac, Int_t ptid) {
        bt_energy_frac_dbg = efrac; bt_numelectron_frac_dbg = nfrac; bt_primary_track_id_dbg = ptid;
    }
    // Summed SimIDE energy attributed to this TP (filled by the caller from
    // simides::SumEnergy on the per-event SimIDE list).
    void SetSimideEnergy(Double_t e) { simide_energy = e; }
    // Per-file ES/CC interaction tag (derived by the caller from the input
    // filename, mirroring the old code); written to matched TPs only.
    void SetInteractionLabel(const std::string& s) { interaction_label_ = s; }
    // Clear the per-TP associations before building a new output object. Only
    // the non-owning pointers are nulled; the underlying objects live in the
    // caller's containers and are left intact, so the same particle/truth/
    // neutrino can be associated with another TP later in the same event.
    void Reset() {
        theparticle  = nullptr;
        theincoming_neutrino  = nullptr;
        theoutgoing_lepton  = nullptr;
        theneutrino  = nullptr;
        thetruth     = nullptr;
        theprimitive = nullptr;
        simide_energy = 0.0;   // scalar truth value, not a pointer; clear per TP
    }
    int WriteSingleTPS();
    //int WriteSingleEvent(const std::vector<TriggerPrimitive> &tps_by_event);
    //int WriteEventVector(const std::vector<std::vector<TriggerPrimitive>> tps_vec);
    void SetBTSMargin(float margin) { bt_error_margin = margin; };
    void Print(const TriggerPrimitive &tp);
    // Compact one-line dump of the current branch values (space-separated
    // field=value), handy for debugging / eyeballing against the comparison
    // macro output. Call after WriteSingleTPS() has filled the members.
    void PrintOneLine() const;

private:
    // Non-owning pointers to the current objects set via the Set* methods.
    const mcparticles*      theparticle  = nullptr;
    const mcneutrinos*      theneutrino  = nullptr;
    const mctruths*         thetruth     = nullptr;
    const mctruths*         theincoming_neutrino = nullptr;
    const mctruths*         theoutgoing_lepton   = nullptr;
    const TriggerPrimitive* theprimitive = nullptr;

    std::string thefilename;
    TTree *tpsTree;
    TFile *outFile;
    UInt_t evt = 0;
    UInt_t run = 0;
    UShort_t version = 0;
    UInt_t detid = 0;
    UInt_t channel = 0;
    UInt_t adc_integral = 0;
    UShort_t adc_peak = 0;
    UShort_t det = 0;
    Int_t det_channel = 0;
    ULong64_t tstart = 0;
    ULong64_t s_over = 0;
    ULong64_t s_to_peak = 0;
    std::string view;
    Double_t simide_energy = 0.0;

    // Truth variables (always stored: generator_name from MC truth)
    std::string gen_name;

    // MARLEY-specific particle truth (only meaningful when gen_name contains "marley")
    Int_t particle_pdg = 0;
    std::string particle_process;
    Double_t particle_energy = 0.0f;
    Double_t particle_x = 0.0f, particle_y = 0.0f, particle_z = 0.0f;
    Double_t particle_px = 0.0f, particle_py = 0.0f, particle_pz = 0.0f;

    // Neutrino info (only for MARLEY with neutrino association)
    std::string neutrino_interaction;
    Double_t neutrino_x = 0.0f, neutrino_y = 0.0f, neutrino_z = 0.0f;
    Double_t neutrino_px = 0.0f, neutrino_py = 0.0f, neutrino_pz = 0.0f;
    Double_t neutrino_energy = 0.0f;

    // DEBUG: raw backtracking fractions copied from the tpstream (temporary)
    Double_t bt_energy_frac_dbg = 0.0;
    Double_t bt_numelectron_frac_dbg = 0.0;
    Int_t    bt_primary_track_id_dbg = 0;

    // Per-file ES/CC tag (filename-derived; set once via SetInteractionLabel).
    std::string interaction_label_ = "UNKNOWN";

    // metadata
    int n_events = 0;
    int n_tps_total = 0;
    float bt_error_margin = 0;
};

#endif // WRITE_TPS_SIMPLE_H