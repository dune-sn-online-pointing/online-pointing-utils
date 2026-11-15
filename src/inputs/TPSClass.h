// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 19:38:23 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 21:12:34 UTC
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
    void SetNeutrino(const mcneutrinos& neutrino)        { theneutrino = &neutrino; }
    void SetTruth(const mctruths& truth)                 { thetruth = &truth; }
    void SetPrimitive(const TriggerPrimitive& primitive) { theprimitive = &primitive; }
    // Clear the per-TP associations before building a new output object. Only
    // the non-owning pointers are nulled; the underlying objects live in the
    // caller's containers and are left intact, so the same particle/truth/
    // neutrino can be associated with another TP later in the same event.
    void Reset() {
        theparticle  = nullptr;
        theneutrino  = nullptr;
        thetruth     = nullptr;
        theprimitive = nullptr;
    }
    int WriteSingleTPS();
    //int WriteSingleEvent(const std::vector<TriggerPrimitive> &tps_by_event);
    //int WriteEventVector(const std::vector<std::vector<TriggerPrimitive>> tps_vec);
    void SetBTSMargin(float margin) { bt_error_margin = margin; };
    void Print(const TriggerPrimitive &tp);

private:
    // Non-owning pointers to the current objects set via the Set* methods.
    const mcparticles*      theparticle  = nullptr;
    const mcneutrinos*      theneutrino  = nullptr;
    const mctruths*         thetruth     = nullptr;
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

    // metadata
    int n_events = 0;
    int n_tps_total = 0;
    float bt_error_margin = 0;
};

#endif // WRITE_TPS_SIMPLE_H