// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 02:42:58 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 19:38:23 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 19:55:40 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 19:58:15 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 21:02:18 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 21:04:49 UTC (reverted Create() path strip; now writes to the full given path)
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 21:31:13 UTC (Create(): bare filename -> current dir, fixes ensureDirectoryExists)
#include "TPSClass.h"
#include "TFile.h"
#include "TTree.h"
#include "Global.h"
#include <cassert>


LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});


// class to contain the pointers to a root tree needed to write tps out in small groups

bool TPSClass::Create(TString outFilename, TString treename="tps"){
    // Ensure output directory exists. A bare filename (no '/') means the
    // current directory, so default folder to "." in that case -- otherwise
    // substr(0, npos) would return the whole filename and we'd try to create a
    // directory named after the file.
    thefilename = outFilename.Data();
    std::size_t slash = thefilename.find_last_of("/");
    std::string folder = (slash == std::string::npos) ? "." : thefilename.substr(0, slash);
    if (!ensureDirectoryExists(folder)) {
        LogError << "Cannot create or access directory for output file: " << folder << std::endl;
        return 0;
    }
    if (debugMode) LogInfo << " about to open output file " <<  outFilename << std::endl;
    outFile = (TFile::Open(outFilename.Data(), "RECREATE"));
    if (!outFile || outFile->IsZombie()) { LogError << "Failed to open file: " << outFilename << std::endl; return 0; }
    if (verboseMode) LogInfo << " the file opened " << outFilename << std::endl;
    // TPs tree at roost level (not inside a folder)
    //outFile->cd();
    tpsTree = new TTree(treename.Data(), "Trigger Primitives with embedded truth");
    tpsTree->SetDirectory(outFile);
    // TP basic branches
    tpsTree->Branch("event", &evt, "event/i");
    //if(debugMode) LogInfo << " made a branch " <<std::endl;
    tpsTree->Branch("run", &run, "run/i");
    tpsTree->Branch("version", &version, "version/s");
    tpsTree->Branch("detid", &detid, "detid/i");
    tpsTree->Branch("channel", &channel, "channel/i");
    tpsTree->Branch("samples_over_threshold", &s_over, "samples_over_threshold/l");
    tpsTree->Branch("time_start", &tstart, "time_start/l");
    tpsTree->Branch("samples_to_peak", &s_to_peak, "samples_to_peak/l");
    tpsTree->Branch("adc_integral", &adc_integral, "adc_integral/i");
    tpsTree->Branch("adc_peak", &adc_peak, "adc_peak/s");
    tpsTree->Branch("detector", &det, "detector/s");
    tpsTree->Branch("detector_channel", &det_channel, "detector_channel/I");
    tpsTree->Branch("view", &view);
    tpsTree->Branch("simide_energy", &simide_energy, "simide_energy/D");
    
    // Truth branches (always: generator_name from MC truth)
    tpsTree->Branch("generator_name", &gen_name);
    
    // MARLEY-specific particle truth branches
    tpsTree->Branch("particle_pdg", &particle_pdg, "particle_pdg/I");
    tpsTree->Branch("particle_process", &particle_process);
    tpsTree->Branch("particle_energy", &particle_energy, "particle_energy/D");
    tpsTree->Branch("particle_x", &particle_x, "particle_x/D");
    tpsTree->Branch("particle_y", &particle_y, "particle_y/D");
    tpsTree->Branch("particle_z", &particle_z, "particle_z/D");
    tpsTree->Branch("particle_px", &particle_px, "particle_px/D");
    tpsTree->Branch("particle_py", &particle_py, "particle_py/D");
    tpsTree->Branch("particle_pz", &particle_pz, "particle_pz/D");
    
    // Neutrino branches
    tpsTree->Branch("neutrino_interaction", &neutrino_interaction);
    tpsTree->Branch("neutrino_x", &neutrino_x, "neutrino_x/D");
    tpsTree->Branch("neutrino_y", &neutrino_y, "neutrino_y/D");
    tpsTree->Branch("neutrino_z", &neutrino_z, "neutrino_z/D");
    tpsTree->Branch("neutrino_px", &neutrino_px, "neutrino_px/D");
    tpsTree->Branch("neutrino_py", &neutrino_py, "neutrino_py/D");
    tpsTree->Branch("neutrino_pz", &neutrino_pz, "neutrino_pz/D");
    tpsTree->Branch("neutrino_energy", &neutrino_energy, "neutrino_energy/D");

    if (verboseMode) LogInfo << "Created output tpsTree " << tpsTree->GetName() << std::endl;
    outFile->ls();
    if (debugMode) LogInfo << " tpstree pointer as created " << tpsTree << std::endl;
    // std::error_code _ec_abs;
    // if (verboseMode) LogInfo << "Opened TPs file: " << (_ec_abs ? outFilename : abs_p.string()) << std::endl;
    return 1;
}

bool TPSClass::Close(){
    LogInfo << "closing output file " << thefilename << std::endl;
    // Backtracking metadata tree
    outFile->cd();
    
    TTree metaTree = TTree("backtracking_metadata", "Backtracking metadata");
    // int n_events = tps_by_event.size();
    // int n_tps_total = 0;
    // for (const auto& v : tps_by_event) n_tps_total += v.size();
    // float bt_error_margin = static_cast<float>(ParametersManager::getInstance().getDouble("timing.backtracker_error_margin"));
    metaTree.Branch("n_events", &n_events, "n_events/I");
    metaTree.Branch("n_tps_total", &n_tps_total, "n_tps_total/I");
    metaTree.Branch("backtracker_error_margin", &bt_error_margin, "backtracker_error_margin/D");
    metaTree.Fill();
    if (verboseMode){ 
        metaTree.Print();
        LogInfo << " Close output file and write metadata" << metaTree.GetName() << std::endl;
    }
    //outFile->cd();
    metaTree.Write();
    tpsTree->Write();
    outFile->ls();
    outFile->Close();
    return true;

}


int TPSClass::WriteSingleTPS(){ 
    
    // Basic TP info
    
    evt = theprimitive->GetEvent();
    run = theprimitive->GetRun();
    assert(run<200000000);
    version = TriggerPrimitive::s_trigger_primitive_version;
    channel = theprimitive->GetChannel();
    s_over = theprimitive->GetSamplesOverThreshold();
    tstart = theprimitive->GetTimeStart();
    s_to_peak = theprimitive->GetSamplesToPeak();
    adc_integral = theprimitive->GetAdcIntegral();
    adc_peak = theprimitive->GetAdcPeak();
    det = theprimitive->GetDetector();
    det_channel = theprimitive->GetDetectorChannel();
    view = theprimitive->GetView();
    simide_energy = 0; // not filled for now theprimitive.GetSimideEnergy();
    
    // Truth info from the particle and the truth
    if (theparticle){   
        gen_name = theparticle->generator_name ? *theparticle->generator_name : std::string();
        particle_pdg = theparticle->pdg;
        //particle_process = theprimitive->GetParticleProcess();
        particle_energy = theparticle->energy;
        particle_x = theparticle->x;
        particle_y = theparticle->y;
        particle_z = theparticle->z;
        particle_px = theparticle->px;
        particle_py = theparticle->py;
        particle_pz = theparticle->pz;
    }
    else{
        gen_name = "";
        particle_pdg = 0;
        particle_process = "";
        particle_energy = 0.0;
        particle_x = 0.0;
        particle_y = 0.0;
        particle_z = 0.0;
        particle_px = 0.0;
        particle_py = 0.0;
        particle_pz = 0.0;
    }
    if (theneutrino){
        //neutrino_interaction = theneutrino->interactionType;
        // neutrino_x = theneutrino.x;
        // neutrino_y = theneutrino.y;
        // neutrino_z = theneutrino.z;
        // neutrino_px = theneutrino.px;
        // neutrino_py = theneutrino.py;
        // neutrino_pz = theneutrino.pz;
        // neutrino_energy = theneutrino.energy;
    }
    if (thetruth){
        neutrino_x = thetruth->x;
        neutrino_y = thetruth->y;
        neutrino_z = thetruth->z;
        neutrino_px = thetruth->px;
        neutrino_py = thetruth->py;
        neutrino_pz = thetruth->pz;
        neutrino_energy = thetruth->energy;
    }
    else{
        neutrino_interaction = "";
        neutrino_x = 0.0;
        neutrino_y = 0.0;
        neutrino_z = 0.0;
        neutrino_px = 0.0;
        neutrino_py = 0.0;
        neutrino_pz = 0.0;
        neutrino_energy = 0.0;
    }
    
    tpsTree->Fill();
    if (debugMode) LogInfo << "Write out tp from " << evt << " " << run << std::endl;
    //n_tps_total ++;

    // if (verboseMode){
    //      LogInfo << n_events <<  " have written total of " << n_tps_total << " tps so far " << std::endl;
        
    // }
  
    return 1;

}

void TPSClass::Print(const TriggerPrimitive &tp){
        LogInfo << "Flat TriggerPrimitive" << std::endl;
        LogInfo << "event : " << theprimitive->GetEvent()<< std::endl;
        LogInfo << "run : " << theprimitive->GetRun()<< std::endl;
        LogInfo << "version : " << TriggerPrimitive::s_trigger_primitive_version<< std::endl;
        LogInfo << "detid = 0"<< std::endl;
        LogInfo << "channel : " << theprimitive->GetChannel()<< std::endl;
        LogInfo << "s_over : " << theprimitive->GetSamplesOverThreshold()<< std::endl;
        LogInfo << "tstart : " << theprimitive->GetTimeStart()<< std::endl;
        LogInfo << "s_to_peak : " << theprimitive->GetSamplesToPeak()<< std::endl;
        LogInfo << "adc_integral : " << theprimitive->GetAdcIntegral()<< std::endl;
        LogInfo << "adc_peak : " << theprimitive->GetAdcPeak()<< std::endl;
        LogInfo << "det : " << theprimitive->GetDetector()<< std::endl;
        LogInfo << "det_channel : " << theprimitive->GetDetectorChannel()<< std::endl;
        LogInfo << "view : " << theprimitive->GetView()<< std::endl;
        LogInfo << "simide_energy : " << theprimitive->GetSimideEnergy()<< std::endl;

        // Truth info (embedded in TP)
        LogInfo << "gen_name : " << theprimitive->GetGeneratorName()<< std::endl;
        LogInfo << "particle_pdg : " << theprimitive->GetParticlePDG()<< std::endl;
        LogInfo << "particle_process : " << theprimitive->GetParticleProcess()<< std::endl;
        LogInfo << "particle_energy : " << theprimitive->GetParticleEnergy()<< std::endl;
        LogInfo << "particle_x : " << theprimitive->GetParticleX()<< std::endl;
        LogInfo << "particle_y : " << theprimitive->GetParticleY()<< std::endl;
        LogInfo << "particle_z : " << theprimitive->GetParticleZ()<< std::endl;
        LogInfo << "particle_px : " << theprimitive->GetParticlePx()<< std::endl;
        LogInfo << "particle_py : " << theprimitive->GetParticlePy()<< std::endl;
        LogInfo << "particle_pz : " << theprimitive->GetParticlePz()<< std::endl;
        LogInfo << "neutrino_interaction : " << theprimitive->GetNeutrinoInteraction()<< std::endl;
        LogInfo << "neutrino_x : " << theprimitive->GetNeutrinoX()<< std::endl;
        LogInfo << "neutrino_y : " << theprimitive->GetNeutrinoY()<< std::endl;
        LogInfo << "neutrino_z : " << theprimitive->GetNeutrinoZ()<< std::endl;
        LogInfo << "neutrino_px : " << theprimitive->GetNeutrinoPx()<< std::endl;
        LogInfo << "neutrino_py : " << theprimitive->GetNeutrinoPy()<< std::endl;
        LogInfo << "neutrino_pz : " << theprimitive->GetNeutrinoPz()<< std::endl;
        LogInfo << "neutrino_energy : " << theprimitive->GetNeutrinoEnergy()<< std::endl;
}

// int TPSClass::WriteEventVector(const std::vector<std::vector<TriggerPrimitive>> tps_vec){
//     for (auto tps_by_event: tps_vec){
//         TPSClass::WriteSingleEvent();
//     }
//     return n_events;
// }


