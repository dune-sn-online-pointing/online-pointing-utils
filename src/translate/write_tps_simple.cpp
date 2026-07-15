
#include "write_tps_simple.h"
#include "TFile.h"
#include "TTree.h"

void write_tps(
    const std::string& out_filename,
    const std::vector<std::vector<TriggerPrimitiveSimple>>& tps_by_event,
    const std::vector<std::vector<TrueParticleSimple>>& true_particles_by_event,
    const std::vector<std::vector<NeutrinoSimple>>& neutrinos_by_event)
{
    // Ensure output directory exists
    std::string folder = out_filename.substr(0, out_filename.find_last_of("/"));
    if (!ensureDirectoryExists(folder)) {
        LogError << "Cannot create or access directory for output file: " << folder << std::endl;
        return;
    }

    TFile outFile(out_filename.c_str(), "RECREATE");
    if (outFile.IsZombie()) {
        LogError << "Cannot create output file: " << out_filename << std::endl;
        return;
    }

    // TPs tree at root level (not inside a folder)
    TTree tpsTree("tps", "Trigger Primitives with embedded truth");
    
    // TP basic variables
    int evt = 0;
    UShort_t version=0;
    UInt_t detid=0;
    UInt_t channel=0;
    UInt_t adc_integral=0;
    UShort_t adc_peak=0;
    UShort_t det=0;
    Int_t det_channel=0;
    ULong64_t tstart=0;
    ULong64_t s_over=0;
    ULong64_t s_to_peak=0;
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
    
    // TP basic branches
    tpsTree.Branch("event", &evt, "event/I");
    tpsTree.Branch("version", &version, "version/s");
    tpsTree.Branch("detid", &detid, "detid/i");
    tpsTree.Branch("channel", &channel, "channel/i");
    tpsTree.Branch("samples_over_threshold", &s_over, "samples_over_threshold/l");
    tpsTree.Branch("time_start", &tstart, "time_start/l");
    tpsTree.Branch("samples_to_peak", &s_to_peak, "samples_to_peak/l");
    tpsTree.Branch("adc_integral", &adc_integral, "adc_integral/i");
    tpsTree.Branch("adc_peak", &adc_peak, "adc_peak/s");
    tpsTree.Branch("detector", &det, "detector/s");
    tpsTree.Branch("detector_channel", &det_channel, "detector_channel/I");
    tpsTree.Branch("view", &view);
    tpsTree.Branch("simide_energy", &simide_energy, "simide_energy/D");
    
    // Truth branches (always: generator_name from MC truth)
    tpsTree.Branch("generator_name", &gen_name);
    
    // MARLEY-specific particle truth branches
    tpsTree.Branch("particle_pdg", &particle_pdg, "particle_pdg/I");
    tpsTree.Branch("particle_process", &particle_process);
    tpsTree.Branch("particle_energy", &particle_energy, "particle_energy/F");
    tpsTree.Branch("particle_x", &particle_x, "particle_x/F");
    tpsTree.Branch("particle_y", &particle_y, "particle_y/F");
    tpsTree.Branch("particle_z", &particle_z, "particle_z/F");
    tpsTree.Branch("particle_px", &particle_px, "particle_px/F");
    tpsTree.Branch("particle_py", &particle_py, "particle_py/F");
    tpsTree.Branch("particle_pz", &particle_pz, "particle_pz/F");
    
    // Neutrino branches
    tpsTree.Branch("neutrino_interaction", &neutrino_interaction);
    tpsTree.Branch("neutrino_x", &neutrino_x, "neutrino_x/F");
    tpsTree.Branch("neutrino_y", &neutrino_y, "neutrino_y/F");
    tpsTree.Branch("neutrino_z", &neutrino_z, "neutrino_z/F");
    tpsTree.Branch("neutrino_px", &neutrino_px, "neutrino_px/F");
    tpsTree.Branch("neutrino_py", &neutrino_py, "neutrino_py/F");
    tpsTree.Branch("neutrino_pz", &neutrino_pz, "neutrino_pz/F");
    tpsTree.Branch("neutrino_energy", &neutrino_energy, "neutrino_energy/F");

    // Backtracking metadata tree
    TTree metaTree("backtracking_metadata", "Backtracking metadata");
    int n_events = tps_by_event.size();
    int n_tps_total = 0;
    for (const auto& v : tps_by_event) n_tps_total += v.size();
    float bt_error_margin = static_cast<float>(ParametersManager::getInstance().getDouble("timing.backtracker_error_margin"));
    metaTree.Branch("n_events", &n_events, "n_events/I");
    metaTree.Branch("n_tps_total", &n_tps_total, "n_tps_total/I");
    metaTree.Branch("backtracker_error_margin", &bt_error_margin, "backtracker_error_margin/F");
    metaTree.Fill();

    // Fill TPs with embedded truth
    for (size_t ev = 0;ev < tps_by_event.size();++ev) {
        const auto& v = tps_by_event[ev];
        for (const auto& tp : v) {
            // Basic TP info
            evt = tp.GetEvent();
            version = TriggerPrimitiveSimple::s_trigger_primitive_version;
            detid = 0;
            channel = tp.GetChannel();
            s_over = tp.GetSamplesOverThreshold();
            tstart = tp.GetTimeStart();
            s_to_peak = tp.GetSamplesToPeak();
            adc_integral = tp.GetAdcIntegral();
            adc_peak = tp.GetAdcPeak();
            det = tp.GetDetector();
            det_channel = tp.GetDetectorChannel();
            view = tp.GetView();
            simide_energy = tp.GetSimideEnergy();
            
            // Truth info (embedded in TP)
            gen_name = tp.GetGeneratorName();
            particle_pdg = tp.GetParticlePDG();
            particle_process = tp.GetParticleProcess();
            particle_energy = tp.GetParticleEnergy();
            particle_x = tp.GetParticleX();
            particle_y = tp.GetParticleY();
            particle_z = tp.GetParticleZ();
            particle_px = tp.GetParticlePx();
            particle_py = tp.GetParticlePy();
            particle_pz = tp.GetParticlePz();
            neutrino_interaction = tp.GetNeutrinoInteraction();
            neutrino_x = tp.GetNeutrinoX();
            neutrino_y = tp.GetNeutrinoY();
            neutrino_z = tp.GetNeutrinoZ();
            neutrino_px = tp.GetNeutrinoPx();
            neutrino_py = tp.GetNeutrinoPy();
            neutrino_pz = tp.GetNeutrinoPz();
            neutrino_energy = tp.GetNeutrinoEnergy();
            tpsTree.Fill();
        }
    }

    // Write both trees at root level
    outFile.cd();
    tpsTree.Write();
    metaTree.Write();
    outFile.Close();
    // Report absolute output path for consistency
    std::error_code _ec_abs;
    auto abs_p = std::filesystem::absolute(std::filesystem::path(out_filename), _ec_abs);
    if (verboseMode) LogInfo << "Wrote TPs file: " << (_ec_abs ? out_filename : abs_p.string()) << std::endl;
}