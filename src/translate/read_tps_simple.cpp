#include "read_tps_simple.h"

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

void read_tps_simple(TTree* tpTree, UInt_t theevent, UInt_t therun, UInt_t lo, UInt_t hi,
    std::vector<TriggerPrimitive>& tps_by_event){
    if (verboseMode) LogInfo << "Reading TPs from: " << tpTree->GetName() << std::endl;
    
    // TFile inFile(in_filename.c_str(), "READ"); 
    // if (inFile.IsZombie()) { LogError << "Cannot open: " << in_filename << std::endl; return; }

    // // Read TPs tree from root level (no longer in "tps" directory)
    // if (auto* tpTree = dynamic_cast<TTree*>(inFile.Get("tps"))) {
        
    // TP basic variables

    if(verboseMode){
        LogInfo << " read_tps_simple " << theevent << " " << therun << " " << lo << " " << hi << std::endl;
    }
    UInt_t event=0; 
    UInt_t run=0;
    UShort_t version=0; 
    UInt_t detid=0, channel=0; 
    ULong64_t s_over=0, tstart=0, s_to_peak=0; 
    UInt_t adc_integral=0; 
    UShort_t adc_peak=0, det=0; 
    Int_t det_channel=0; 
    std::string* view=nullptr;
    Double_t simide_energy=0.0;
    
    // Truth variables
    std::string* gen_name=nullptr;
    Int_t particle_pdg=0;
    std::string* particle_process=nullptr;
    Float_t particle_energy=0.0f;
    Float_t particle_x=0.0f, particle_y=0.0f, particle_z=0.0f;
    Float_t particle_px=0.0f, particle_py=0.0f, particle_pz=0.0f;
    std::string* neutrino_interaction=nullptr;
    Float_t neutrino_x=0.0f, neutrino_y=0.0f, neutrino_z=0.0f;
    Float_t neutrino_px=0.0f, neutrino_py=0.0f, neutrino_pz=0.0f;
    Float_t neutrino_energy=0.0f;
    
    // Set branch addresses for TP basics
    
    tpTree->SetBranchAddress("event", &event); 
    tpTree->SetBranchAddress("run", &run); 
    tpTree->SetBranchAddress("version", &version); 
    tpTree->SetBranchAddress("detid", &detid); 
    tpTree->SetBranchAddress("channel", &channel);
    tpTree->SetBranchAddress("samples_over_threshold", &s_over);
    tpTree->SetBranchAddress("time_start", &tstart);
    tpTree->SetBranchAddress("samples_to_peak", &s_to_peak);
    tpTree->SetBranchAddress("adc_integral", &adc_integral);
    tpTree->SetBranchAddress("adc_peak", &adc_peak);
    tpTree->SetBranchAddress("detector", &det);
    tpTree->SetBranchAddress("detector_channel", &det_channel);
    tpTree->SetBranchAddress("view", &view);
    if (tpTree->GetBranch("simide_energy")) {
        tpTree->SetBranchAddress("simide_energy", &simide_energy);
    }
    
    // Set branch addresses for truth
    tpTree->SetBranchAddress("generator_name", &gen_name);
    if (tpTree->GetBranch("particle_pdg")) tpTree->SetBranchAddress("particle_pdg", &particle_pdg);
    if (tpTree->GetBranch("particle_process")) tpTree->SetBranchAddress("particle_process", &particle_process);
    if (tpTree->GetBranch("particle_energy")) tpTree->SetBranchAddress("particle_energy", &particle_energy);
    if (tpTree->GetBranch("particle_x")) tpTree->SetBranchAddress("particle_x", &particle_x);
    if (tpTree->GetBranch("particle_y")) tpTree->SetBranchAddress("particle_y", &particle_y);
    if (tpTree->GetBranch("particle_z")) tpTree->SetBranchAddress("particle_z", &particle_z);
    if (tpTree->GetBranch("particle_px")) tpTree->SetBranchAddress("particle_px", &particle_px);
    if (tpTree->GetBranch("particle_py")) tpTree->SetBranchAddress("particle_py", &particle_py);
    if (tpTree->GetBranch("particle_pz")) tpTree->SetBranchAddress("particle_pz", &particle_pz);
    if (tpTree->GetBranch("neutrino_interaction")) tpTree->SetBranchAddress("neutrino_interaction", &neutrino_interaction);
    if (tpTree->GetBranch("neutrino_x")) tpTree->SetBranchAddress("neutrino_x", &neutrino_x);
    if (tpTree->GetBranch("neutrino_y")) tpTree->SetBranchAddress("neutrino_y", &neutrino_y);
    if (tpTree->GetBranch("neutrino_z")) tpTree->SetBranchAddress("neutrino_z", &neutrino_z);
    if (tpTree->GetBranch("neutrino_px")) tpTree->SetBranchAddress("neutrino_px", &neutrino_px);
    if (tpTree->GetBranch("neutrino_py")) tpTree->SetBranchAddress("neutrino_py", &neutrino_py);
    if (tpTree->GetBranch("neutrino_pz")) tpTree->SetBranchAddress("neutrino_pz", &neutrino_pz);
    if (tpTree->GetBranch("neutrino_energy")) tpTree->SetBranchAddress("neutrino_energy", &neutrino_energy);

    for (Long64_t i=lo; i<=hi; ++i){ 
        tpTree->GetEntry(i); 
        if (event != theevent || run != therun){
            LogInfo << "event did not match tree: " << tpTree->GetName() << " event: " << event << " theevent: " << theevent << " run : " << run << " therun: " << therun << std::endl;
            assert(0);
        }

        TriggerPrimitive tp(version, 0, detid, channel, s_over, tstart, s_to_peak, adc_integral, adc_peak); 
        tp.SetEvent(event);
        tp.SetRun(run); 
        tp.SetDetector(det);
        tp.SetDetectorChannel(det_channel);
        tp.SetSimideEnergy(simide_energy);
        
        // Set embedded truth
        if (gen_name) tp.SetGeneratorName(*gen_name);
        tp.SetParticlePDG(particle_pdg);
        if (particle_process) tp.SetParticleProcess(*particle_process);
        tp.SetParticleEnergy(particle_energy);
        tp.SetParticlePosition(particle_x, particle_y, particle_z);
        tp.SetParticleMomentum(particle_px, particle_py, particle_pz);
        if (neutrino_interaction) {
            tp.SetNeutrinoInfo(*neutrino_interaction, neutrino_x, neutrino_y, neutrino_z,
                                neutrino_px, neutrino_py, neutrino_pz, neutrino_energy);
        }
        
        tps_by_event.push_back(tp);
    }
}

    // Note: true_particles_by_event and neutrinos_by_event are no longer populated
    // Truth information is now embedded directly in TPs
    // These maps are kept as function parameters for backward compatibility but will be empty


