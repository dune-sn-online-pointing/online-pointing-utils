// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 02:42:58 UTC
#include "TPSClass.h"
#include "TFile.h"
#include "TTree.h"
#include "Global.h"
#include <cassert>
#include "generate_filemap.h"

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});


// class to contain the pointers to a root tree needed to write tps out in small groups

bool TPSClass::Create(std::string outFilename, std::string treename="tps"){
    // Ensure output directory exists
    thefilename = outFilename;
    std::string folder = outFilename.substr(0, outFilename.find_last_of("/"));
    if (!ensureDirectoryExists(folder)) {
        LogError << "Cannot create or access directory for output file: " << folder << std::endl;
        return 0;
    }
    if (debugMode) LogInfo << " about to open output file" <<  outFilename << std::endl;
    outFile = (TFile::Open(outFilename.c_str(), "RECREATE"));
    if (!outFile || outFile->IsZombie()) { LogError << "Failed to open file: " << outFilename << std::endl; return 0; }
    if (verboseMode) LogInfo << " the file opened " << outFilename << std::endl;
    // TPs tree at roost level (not inside a folder)
    //outFile->cd();
    tpsTree = new TTree(treename.c_str(), "Trigger Primitives with embedded truth");
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


int TPSClass::WriteSingleEvent(const std::vector<TriggerPrimitive>& tps_by_event){
    int n_tps = 0;
    if (debugMode) LogInfo << "try to write " << tps_by_event.size() << " tps " << std::endl;
    UInt_t first = 0;
    if (tps_by_event.size()>0) first = tps_by_event.at(0).GetEvent();
    bool test = SingleEventChecker(tps_by_event);
    if (!test) LogError << " Inconsistent event" << std::endl;
    for (const auto& tp : tps_by_event) {
        // Basic TP info
        n_tps++;
        evt = tp.GetEvent();
        if (evt != first){
            LogInfo << " different events in vector " << n_tps << " first " << first << " " << evt << std::endl;
        }
        run = tp.GetRun();
        assert(run<200000000);
        version = tp.TriggerPrimitive::s_trigger_primitive_version;
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
        //particle_process = tp.GetParticleProcess();
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
        // if (verboseMode) LogInfo << "Fill " << channel << std::endl;
        //if (verboseMode && s_over > 10) Print(tp);
        // if (debugMode) LogInfo << " before fill " << tpsTree << std::endl;
        tpsTree->Fill();
        // if (debugMode) LogInfo << "Write out tp " << n_tps << " from " << evt << " " << run << std::endl;
        n_tps_total ++;
    }
    n_events ++;
    if (verboseMode){
         LogInfo << n_events <<  " have written total of " << n_tps_total << " tps so far " << std::endl;
        
    }
  
    return tps_by_event.size();

}

void TPSClass::Print(const TriggerPrimitive &tp){
        LogInfo << "Flat TriggerPrimitive" << std::endl;
        LogInfo << "event : " << tp.GetEvent()<< std::endl;
        LogInfo << "run : " << tp.GetRun()<< std::endl;
        LogInfo << "version : " << TriggerPrimitive::s_trigger_primitive_version<< std::endl;
        LogInfo << "detid = 0"<< std::endl;
        LogInfo << "channel : " << tp.GetChannel()<< std::endl;
        LogInfo << "s_over : " << tp.GetSamplesOverThreshold()<< std::endl;
        LogInfo << "tstart : " << tp.GetTimeStart()<< std::endl;
        LogInfo << "s_to_peak : " << tp.GetSamplesToPeak()<< std::endl;
        LogInfo << "adc_integral : " << tp.GetAdcIntegral()<< std::endl;
        LogInfo << "adc_peak : " << tp.GetAdcPeak()<< std::endl;
        LogInfo << "det : " << tp.GetDetector()<< std::endl;
        LogInfo << "det_channel : " << tp.GetDetectorChannel()<< std::endl;
        LogInfo << "view : " << tp.GetView()<< std::endl;
        LogInfo << "simide_energy : " << tp.GetSimideEnergy()<< std::endl;
        
        // Truth info (embedded in TP)
        LogInfo << "gen_name : " << tp.GetGeneratorName()<< std::endl;
        LogInfo << "particle_pdg : " << tp.GetParticlePDG()<< std::endl;
        LogInfo << "particle_process : " << tp.GetParticleProcess()<< std::endl;
        LogInfo << "particle_energy : " << tp.GetParticleEnergy()<< std::endl;
        LogInfo << "particle_x : " << tp.GetParticleX()<< std::endl;
        LogInfo << "particle_y : " << tp.GetParticleY()<< std::endl;
        LogInfo << "particle_z : " << tp.GetParticleZ()<< std::endl;
        LogInfo << "particle_px : " << tp.GetParticlePx()<< std::endl;
        LogInfo << "particle_py : " << tp.GetParticlePy()<< std::endl;
        LogInfo << "particle_pz : " << tp.GetParticlePz()<< std::endl;
        LogInfo << "neutrino_interaction : " << tp.GetNeutrinoInteraction()<< std::endl;
        LogInfo << "neutrino_x : " << tp.GetNeutrinoX()<< std::endl;
        LogInfo << "neutrino_y : " << tp.GetNeutrinoY()<< std::endl;
        LogInfo << "neutrino_z : " << tp.GetNeutrinoZ()<< std::endl;
        LogInfo << "neutrino_px : " << tp.GetNeutrinoPx()<< std::endl;
        LogInfo << "neutrino_py : " << tp.GetNeutrinoPy()<< std::endl;
        LogInfo << "neutrino_pz : " << tp.GetNeutrinoPz()<< std::endl;
        LogInfo << "neutrino_energy : " << tp.GetNeutrinoEnergy()<< std::endl;
}

int TPSClass::WriteEventVector(const std::vector<std::vector<TriggerPrimitive>> tps_vec){
    for (auto tps_by_event: tps_vec){
        TPSClass::WriteSingleEvent(tps_by_event);
    }
    return n_events;
}


