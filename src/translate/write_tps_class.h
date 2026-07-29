#ifndef WRITE_TPS_CLASS_H
#define WRITE_TPS_CLASS_H

#include <string>
#include <vector>
#include "TFile.h"
#include "TTree.h"
#include "TriggerPrimitive.hpp"
// #include "TrueParticle.h"
// #include "Neutrino.h"


class TpsWriter
{
public:
    //TpsWriter();
    bool Create(std::string filename, std::string treename);
    bool Close();
    // TTree* GetTree();
    // TFile* GetFile();
    int WriteSingleEvent(const std::vector<TriggerPrimitive> &tps_by_event);
    int WriteEventVector(const std::vector<std::vector<TriggerPrimitive>> tps_vec);
    void SetBTSMargin(float margin) { bt_error_margin = margin; };
    void Print(const TriggerPrimitive &tp);

private:
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
    Float_t particle_energy = 0.0f;
    Float_t particle_x = 0.0f, particle_y = 0.0f, particle_z = 0.0f;
    Float_t particle_px = 0.0f, particle_py = 0.0f, particle_pz = 0.0f;

    // Neutrino info (only for MARLEY with neutrino association)
    std::string neutrino_interaction;
    Float_t neutrino_x = 0.0f, neutrino_y = 0.0f, neutrino_z = 0.0f;
    Float_t neutrino_px = 0.0f, neutrino_py = 0.0f, neutrino_pz = 0.0f;
    Float_t neutrino_energy = 0.0f;

    // metadata
    int n_events = 0;
    int n_tps_total = 0;
    float bt_error_margin = 0;
};

#endif // WRITE_TPS_SIMPLE_H