#ifndef TRIGGERPRIMITIVE_HPP
#define TRIGGERPRIMITIVE_HPP

#include "TrueParticle.h"
#include "global.h"

class TriggerPrimitive {

    public:
        static constexpr uint8_t s_trigger_primitive_version = 2;

        // Setters
        void SetTimeStart(uint64_t time_start)                        { this->time_start_ = time_start; }
        void SetSamplesOverThreshold(uint64_t samples_over_threshold) { this->samples_over_threshold_ = samples_over_threshold; }
        void SetSamplesToPeak(uint64_t samples_to_peak)               { this->samples_to_peak_ = samples_to_peak; }
        void SetAdcIntegral(uint64_t adc_integral)                    { this->adc_integral_ = adc_integral; }
        void SetAdcPeak(uint64_t adc_peak)                            { this->adc_peak_ = adc_peak; }
        void SetView(const std::string& view)                         { this->view_ = view; }
        void SetDetector(Int_t detector)                                { this->detector_ = detector; }
        void SetDetectorChannel(Int_t detector_channel)                 { this->detector_channel_ = detector_channel; }
        void SetEvent(UInt_t event)                                   { this->event_ = event; }
        void SetRun(UInt_t run)                                       { this->run_ = run; }
        void SetSimideEnergy(Double_t simide_energy)                    { this->simide_energy_ = simide_energy; }
        void AddSimideEnergy(Double_t simide_energy)                    { this->simide_energy_ += simide_energy; }
        
        // Truth setters (always set generator)
        void SetGeneratorName(const std::string& gen)                 { this->generator_name_ = gen; }
        void SetBTGeneratorName(const std::string& gen)                 { this->bt_generator_name_ = gen; }
        
        // MARLEY-specific particle truth setters (only set when generator is MARLEY)
        void SetParticlePDG(Int_t pdg)                                 { this->particle_pdg_ = pdg; }
        void SetParticleProcess(const std::string& proc)             { this->particle_process_ = proc; }
        void SetParticleEnergy(Double_t energy)                         { this->particle_energy_ = Double_t(energy); }
        void SetParticlePosition(Double_t x, Double_t y, Double_t z)          { this->particle_x_ = Double_t(x); this->particle_y_ = Double_t(y); this->particle_z_ = Double_t(z); }
        void SetParticleMomentum(Double_t px, Double_t py, Double_t pz)       { this->particle_px_ = Double_t(px); this->particle_py_ = Double_t(py); this->particle_pz_ = Double_t(pz); }
        void SetNeutrinoInfo(const std::string& interaction, Double_t nu_x, Double_t nu_y, Double_t nu_z, 
        Double_t nu_px, Double_t nu_py, Double_t nu_pz, Double_t nu_energy) {
            this->neutrino_interaction_ = interaction;
            this->neutrino_x_ = nu_x; this->neutrino_y_ = nu_y; this->neutrino_z_ = nu_z;
            this->neutrino_px_ = nu_px; this->neutrino_py_ = nu_py; this->neutrino_pz_ = nu_pz;
            this->neutrino_energy_ = nu_energy;
        }
        
        // Helper to set all MARLEY truth from TrueParticle and Neutrino (for backward compatibility during transition)
        void SetTrueParticle(const TrueParticle* true_particle) {
            temp_true_particle_ = true_particle;  // Store pointer for later generator name update
            if (true_particle == nullptr) {
                generator_name_ = "UNKNOWN";
                return;
            }
            generator_name_ = true_particle->GetGeneratorName();
            
            // Check if MARLEY (case-insensitive)
            std::string gen_lower = generator_name_;
            std::transform(gen_lower.begin(), gen_lower.end(), gen_lower.begin(), ::tolower);
            if (gen_lower.find("marley") != std::string::npos) {
                particle_pdg_ = true_particle->GetPdg();
                particle_process_ = true_particle->GetProcess();
                particle_energy_ = true_particle->GetEnergy();
                particle_x_ = true_particle->GetX();
                particle_y_ = true_particle->GetY();
                particle_z_ = true_particle->GetZ();
                particle_px_ = true_particle->GetPx();
                particle_py_ = true_particle->GetPy();
                particle_pz_ = true_particle->GetPz();
                
                const Neutrino* nu = true_particle->GetNeutrino();
                if (nu != nullptr) {
                    neutrino_interaction_ = nu->GetInteraction();
                    neutrino_x_ = nu->GetX();
                    neutrino_y_ = nu->GetY();
                    neutrino_z_ = nu->GetZ();
                    neutrino_px_ = nu->GetPx();
                    neutrino_py_ = nu->GetPy();
                    neutrino_pz_ = nu->GetPz();
                    neutrino_energy_ = (Double_t)nu->GetEnergy();
                }
            }
        }
        void SetView(Int_t ch) {
            if (ch < APA::induction_channels)                                      { view_ = "U"; } 
            else if (ch < APA::induction_channels * 2)                             { view_ = "V"; }
            else if (ch < APA::induction_channels * 2 + APA::collection_channels)  { view_ = "X"; }
            else { 
                LogError << "Channel out of range: " << ch << "! Critical, stopping execution.\n"; 
                throw std::runtime_error("Channel out of range: ");
            }
            // LogInfo << "Set view to: " << view_ << std::endl;
        }

        // Getters
        Double_t GetTimeStart() const     { return time_start_; }
        Double_t GetTimeEnd() const       { return time_start_ + samples_over_threshold_ * TPC_sample_length; }
        Double_t GetTimePeak() const      { return time_start_ + samples_to_peak_ * TPC_sample_length; }
        std::string GetView() const     { return view_; }
        Int_t GetDetector()                   const { return detector_; }
        Int_t GetDetectorChannel()            const { return detector_channel_; }
        Int_t GetChannel()                    const { return channel_; } // this is the original channel for larsoft
        UInt_t GetEvent()                      const { return event_; }
        UInt_t GetRun()                        const { return run_; }
        uint64_t GetSamplesOverThreshold()  const { return samples_over_threshold_; }
        uint64_t GetSamplesToPeak()         const { return samples_to_peak_; }
        uint64_t GetAdcIntegral()           const { return adc_integral_; }
        uint64_t GetAdcPeak()               const { return adc_peak_; }
        Double_t GetSimideEnergy()            const { return simide_energy_; }
        
        // Truth getters (always available)
        std::string GetGeneratorName() const { return generator_name_; }
        std::string GetBTGeneratorName() const { return bt_generator_name_; }
        
        // MARLEY-specific particle truth getters (return meaningful values only if generator is MARLEY)
        Int_t GetParticlePDG() const              { return particle_pdg_; }
        std::string GetParticleProcess() const  { return particle_process_; }
        Double_t GetParticleEnergy() const         { return particle_energy_; }
        Double_t GetParticleX() const              { return particle_x_; }
        Double_t GetParticleY() const              { return particle_y_; }
        Double_t GetParticleZ() const              { return particle_z_; }
        Double_t GetBTPrimaryX() const              { return bt_primary_x_; }
        Double_t GetBTPrimaryY() const              { return bt_primary_y_; }
        Double_t GetBTPrimaryZ() const              { return bt_primary_z_; }
        Double_t GetParticlePx() const             { return particle_px_; }
        Double_t GetParticlePy() const             { return particle_py_; }
        Double_t GetParticlePz() const             { return particle_pz_; }
        std::string GetNeutrinoInteraction() const { return neutrino_interaction_; }
        Double_t GetNeutrinoX() const              { return neutrino_x_; }
        Double_t GetNeutrinoY() const              { return neutrino_y_; }
        Double_t GetNeutrinoZ() const              { return neutrino_z_; }
        Double_t GetNeutrinoPx() const             { return neutrino_px_; }
        Double_t GetNeutrinoPy() const             { return neutrino_py_; }
        Double_t GetNeutrinoPz() const             { return neutrino_pz_; }
        Double_t GetNeutrinoEnergy() const         { return neutrino_energy_; }
        
        // Helper to check if this is MARLEY (case-insensitive)
        bool IsMarley() const {
            std::string gen_lower = generator_name_;
            std::transform(gen_lower.begin(), gen_lower.end(), gen_lower.begin(), ::tolower);
            return gen_lower.find("marley") != std::string::npos;
        }
        
        // Temporary accessor for backtracking processing (returns temp pointer, not serialized)
        const TrueParticle* GetTrueParticle() const {
            return temp_true_particle_;
        }

        TriggerPrimitive(
            uint64_t version,
            uint64_t flag,
            uint64_t detid,
            uint64_t channel,
            uint64_t samples_over_threshold,
            uint64_t time_start,
            uint64_t samples_to_peak,
            uint64_t adc_integral,
            uint64_t adc_peak
        ) : version_(version),
            flag_(flag),
            detid_(detid),
            channel_(channel),
            samples_over_threshold_(samples_over_threshold),
            time_start_(time_start),
            samples_to_peak_(samples_to_peak),
            adc_integral_(adc_integral),
            adc_peak_(adc_peak)
        {
            if (version_ != s_trigger_primitive_version) {
                static std::once_flag tp_version_warning;
                std::call_once(tp_version_warning, []() {
                    LogWarning("TriggerPrimitive version is not 2, be sure to have converted time_peak to samples_to_peak");
                });
            }

            detector_channel_ = channel_ % APA::total_channels;
            detector_ = channel_ / APA::total_channels;
            SetView(detector_channel_);
            // LogInfo << "TriggerPrimitive created with channel: " << channel_ << ", detector: " << detector_ << ", view: " << view_ << std::endl;
        };

        // Methods


        void SetBtVals(UInt_t readout_plane_id,
            Int_t readout_view,
            UInt_t TPCSetID,
            Double_t bt_edep,	
            const std::string &bt_generator_name,	
            Double_t bt_numelectrons,
            Double_t bt_primary_track_energy_frac,
            Int_t    bt_primary_track_id,
            Double_t bt_primary_track_numelectron_frac,
            Double_t bt_primary_x,
            Double_t bt_primary_y,
            Double_t bt_primary_z,
            Int_t    bt_truth_block_id,
            Double_t bt_x,
            Double_t bt_y,
            Double_t bt_z)
            {
                this->bt_edep_ = bt_edep; 
                this->bt_generator_name_ = bt_generator_name;
                this->bt_numelectrons_ = bt_numelectrons;
                this->bt_primary_track_id_ = bt_primary_track_id;
                this->bt_primary_track_energy_frac_ = bt_primary_track_energy_frac;
                this->bt_primary_x_ = bt_primary_x;
                this->bt_primary_y_ = bt_primary_y;
                this->bt_primary_z_ = bt_primary_z;
                this->bt_truth_block_id_ = bt_truth_block_id;
                this->bt_x_ = bt_x;
                this->bt_y_ = bt_y;
                this->bt_z_ = bt_z;
            } 
        void Print() const {
            LogInfo << "TriggerPrimitive: " << std::endl;
            LogInfo << "  event: " << " " <<event_ << std::endl;
            LogInfo << "  run: " << " " << run_ << std::endl;
            LogInfo << "  version: " << " " << version_ << std::endl;
            LogInfo << "  channel: " << " " << channel_ << std::endl;
            LogInfo << "  samples_over_threshold: " << " " << samples_over_threshold_ << std::endl;
            LogInfo << "  time_start: " << " " << time_start_ << std::endl;
            LogInfo << "  samples_to_peak: " << " " << samples_to_peak_ << std::endl;
            LogInfo << "  adc_integral: " << " " << adc_integral_ << std::endl;
            LogInfo << "  adc_peak: " << " " << adc_peak_ << std::endl;
            LogInfo << "  detector: " << " " << detector_ << std::endl;
            LogInfo << "  detector_channel: " << " " << detector_channel_ << std::endl;
            LogInfo << "  view: " << " " << view_ << std::endl;
            LogInfo << "  stored generator " << " " << generator_name_ << std::endl;
            LogInfo << "  particle_pdg" << " " << particle_pdg_ << std::endl;
            if (generator_name_ == "UNKNOWN" && bt_generator_name_ == "") return;
            LogInfo << "  particle_process" << " " << particle_process_ << std::endl;
            LogInfo << "  particle_energy" << " " << particle_energy_ << std::endl;
            LogInfo << "  particle_x" << " " << particle_x_ << std::endl;
            LogInfo << "  particle_y" << " " << particle_y_ << std::endl;
            LogInfo << "  particle_z" << " " << particle_z_ << std::endl;
            LogInfo << "  particle_px" << " " << particle_px_ << std::endl;
            LogInfo << "  particle_py" << " " << particle_py_ << std::endl;
            LogInfo << "  particle_pz" << " " << particle_pz_ << std::endl;
            LogInfo << "  neutrino_interaction" << " " << neutrino_interaction_ << std::endl;
            LogInfo << "  neutrino_x" << " " << neutrino_x_ << std::endl;
            LogInfo << "  neutrino_y" << " " << neutrino_y_ << std::endl;
            LogInfo << "  neutrino_z" << " " << neutrino_z_ << std::endl;
            LogInfo << "  neutrino_px" << " " << neutrino_px_ << std::endl;
            LogInfo << "  neutrino_py" << " " << neutrino_py_ << std::endl;
            LogInfo << "  neutrino_pz" << " " << neutrino_pz_ << std::endl;
            LogInfo << "  neutrino_energy" << " " << neutrino_energy_ << std::endl;  
            LogInfo << "  simide_energy" << " " << simide_energy_ << std::endl;
            LogInfo << "  readout_plane_id" << " " << readout_plane_id << std::endl;
            LogInfo << "  readout_view" << " " << readout_view_ << std::endl;
            LogInfo << "  TPCSetID" << " " << TPCSetID_ << std::endl;
            LogInfo << "  bt_edep" << " " << bt_edep_ << std::endl;
            LogInfo << "  bt_generator_name" << " " << bt_generator_name_ << std::endl;
            LogInfo << "  bt_numelectrons" << " " << bt_numelectrons_ << std::endl;
            LogInfo << "  bt_primary_track_energy_frac" << " " << bt_primary_track_energy_frac_ << std::endl;
            LogInfo << "  bt_primary_track_id" << " " << bt_primary_track_id_ << std::endl;
            LogInfo << "  bt_primary_track_numelectron_frac" << " " << bt_primary_track_numelectron_frac_ << std::endl;
            LogInfo << "  bt_primary_x" << " " << bt_primary_x_ << std::endl;
            LogInfo << "  bt_primary_y" << " " << bt_primary_y_ << std::endl;
            LogInfo << "  bt_primary_z" << " " << bt_primary_z_ << std::endl;
            LogInfo << "  bt_truth_block_id" << " " << bt_truth_block_id_ << std::endl;
            LogInfo << "  bt_x" << " " << bt_x_ << std::endl;
            LogInfo << "  bt_y" << " " << bt_y_ << std::endl;
            LogInfo << "  bt_z" << " " << bt_z_ << std::endl;
        }

    private:
        // Metadata.
    uint64_t version_ = 0;
    uint64_t flag_ = 0;
    uint64_t detid_ = 0;

    // Physics data.
    uint64_t channel_ = 0;
    uint64_t samples_over_threshold_ = 0;
    uint64_t time_start_ = 0; // in larsoft it's much shorter
    uint64_t samples_to_peak_ = 0;
    uint64_t adc_integral_ = 0;
    uint64_t adc_peak_ = 0;

    // Additional variables
    Int_t detector_ = -1; // this goes from 0 to the number of APAs 
    Int_t detector_channel_ = -1;
    std::string view_ = "";
    UInt_t event_ = -1;
    UInt_t run_ = -1;

        
        
    // SimIDE energy in MeV (sum of all SimIDEs contributing to this TP)
    Double_t simide_energy_ = 0.0;
    
    // Temporary pointer for backtracking processing (not serialized)
    const TrueParticle* temp_true_particle_ = nullptr;
    
    // ===== Embedded truth information =====
    // Always stored: generator name for all TPs (from MC truth, not Geant4)
    std::string generator_name_ = "UNKNOWN";
    
    // MARLEY-specific particle truth (only filled when generator is MARLEY)
    Int_t particle_pdg_ = 0;
    std::string particle_process_ = "";
    Double_t particle_energy_ = 0.0f;
    Double_t particle_x_ = 0.0f;
    Double_t particle_y_ = 0.0f;
    Double_t particle_z_ = 0.0f;
    Double_t particle_px_ = 0.0f;
    Double_t particle_py_ = 0.0f;
    Double_t particle_pz_ = 0.0f;
    
    // Neutrino information (only filled for MARLEY TPs with neutrino association)
    std::string neutrino_interaction_ = "";
    Double_t neutrino_x_ = 0.0f;
    Double_t neutrino_y_ = 0.0f;
    Double_t neutrino_z_ = 0.0f;
    Double_t neutrino_px_ = 0.0f;
    Double_t neutrino_py_ = 0.0f;
    Double_t neutrino_pz_ = 0.0f;
    Double_t neutrino_energy_ = 0.0f;

    // variables from new tpstream format

    UInt_t readout_plane_id = 0;
    Int_t readout_view_ = 0;
    UInt_t TPCSetID_ = 0;
    Double_t bt_edep_ = 0.0f;
    std::string bt_generator_name_ = "";
    Double_t bt_numelectrons_	= 0.0f;
    Double_t bt_primary_track_energy_frac_	= 0.0f;
    Int_t bt_primary_track_id_	= 0;
    Double_t bt_primary_track_numelectron_frac_	= 0.0f;
    Double_t bt_primary_x_	= 0.0f;
    Double_t bt_primary_y_	= 0.0f;
    Double_t bt_primary_z_	= 0.0f;
    Int_t bt_truth_block_id_ = 0;
    Double_t bt_x_	= 0.0f;
    Double_t bt_y_	= 0.0f;
    Double_t bt_z_ = 0.0f;
}; // class TriggerPrimitive

#endif // TRIGGERPRIMITIVE_HPP
