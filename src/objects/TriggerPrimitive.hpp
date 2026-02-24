#ifndef TRIGGERPRIMITIVE_HPP
#define TRIGGERPRIMITIVE_HPP

#include "TrueParticle.h"
#include "Global.h"

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
        void SetDetector(int detector)                                { this->detector_ = detector; }
        void SetDetectorChannel(int detector_channel)                 { this->detector_channel_ = detector_channel; }
        void SetEvent(int event)                                      { this->event_ = event; }
        void SetSimideEnergy(double simide_energy)                    { this->simide_energy_ = simide_energy; }
        void AddSimideEnergy(double simide_energy)                    { this->simide_energy_ += simide_energy; }
        
        // Truth setters (always set generator)
        void SetGeneratorName(const std::string& gen)                 { this->generator_name_ = gen; }
        
        // MARLEY-specific particle truth setters (only set when generator is MARLEY)
        void SetParticlePDG(int pdg)                                 { this->particle_pdg_ = pdg; }
        void SetParticleProcess(const std::string& proc)             { this->particle_process_ = proc; }
        void SetParticleEnergy(float energy)                         { this->particle_energy_ = energy; }
        void SetParticlePosition(float x, float y, float z)          { this->particle_x_ = x; this->particle_y_ = y; this->particle_z_ = z; }
        void SetParticleMomentum(float px, float py, float pz)       { this->particle_px_ = px; this->particle_py_ = py; this->particle_pz_ = pz; }
        void SetNeutrinoInfo(const std::string& interaction, float nu_x, float nu_y, float nu_z, 
                            float nu_px, float nu_py, float nu_pz, float nu_energy) {
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
                    neutrino_energy_ = (float)nu->GetEnergy();
                }
            }
        }
        void SetView(int ch) {
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
        double GetTimeStart() const     { return time_start_; }
        double GetTimeEnd() const       { return time_start_ + samples_over_threshold_ * TPC_sample_length; }
        double GetTimePeak() const      { return time_start_ + samples_to_peak_ * TPC_sample_length; }
        std::string GetView() const     { return view_; }
        int GetDetector()                   const { return detector_; }
        int GetDetectorChannel()            const { return detector_channel_; }
        int GetChannel()                    const { return channel_; } // this is the original channel for larsoft
        int GetEvent()                      const { return event_; }
        uint64_t GetSamplesOverThreshold()  const { return samples_over_threshold_; }
        uint64_t GetSamplesToPeak()         const { return samples_to_peak_; }
        uint64_t GetAdcIntegral()           const { return adc_integral_; }
        uint64_t GetAdcPeak()               const { return adc_peak_; }
        double GetSimideEnergy()            const { return simide_energy_; }
        
        // Truth getters (always available)
        std::string GetGeneratorName() const { return generator_name_; }
        
        // MARLEY-specific particle truth getters (return meaningful values only if generator is MARLEY)
        int GetParticlePDG() const              { return particle_pdg_; }
        std::string GetParticleProcess() const  { return particle_process_; }
        float GetParticleEnergy() const         { return particle_energy_; }
        float GetParticleX() const              { return particle_x_; }
        float GetParticleY() const              { return particle_y_; }
        float GetParticleZ() const              { return particle_z_; }
        float GetParticlePx() const             { return particle_px_; }
        float GetParticlePy() const             { return particle_py_; }
        float GetParticlePz() const             { return particle_pz_; }
        std::string GetNeutrinoInteraction() const { return neutrino_interaction_; }
        float GetNeutrinoX() const              { return neutrino_x_; }
        float GetNeutrinoY() const              { return neutrino_y_; }
        float GetNeutrinoZ() const              { return neutrino_z_; }
        float GetNeutrinoPx() const             { return neutrino_px_; }
        float GetNeutrinoPy() const             { return neutrino_py_; }
        float GetNeutrinoPz() const             { return neutrino_pz_; }
        float GetNeutrinoEnergy() const         { return neutrino_energy_; }
        
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
        }

        // Methods
        void Print() const {
            LogInfo << "TriggerPrimitive: " << std::endl;
            LogInfo << "  event: " << event_ << std::endl;
            LogInfo << "  version: " << version_ << std::endl;
            LogInfo << "  channel: " << channel_ << std::endl;
            LogInfo << "  samples_over_threshold: " << samples_over_threshold_ << std::endl;
            LogInfo << "  time_start: " << time_start_ << std::endl;
            LogInfo << "  samples_to_peak: " << samples_to_peak_ << std::endl;
            LogInfo << "  adc_integral: " << adc_integral_ << std::endl;
            LogInfo << "  adc_peak: " << adc_peak_ << std::endl;
            LogInfo << "  detector: " << detector_ << std::endl;
            LogInfo << "  detector_channel: " << detector_channel_ << std::endl;
            LogInfo << "  view: " << view_ << std::endl;
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
        int detector_ = -1; // this goes from 0 to the number of APAs 
        int detector_channel_ = -1;
        std::string view_ = "";
        int event_ = -1;
        
        // SimIDE energy in MeV (sum of all SimIDEs contributing to this TP)
        double simide_energy_ = 0.0;
        
        // Temporary pointer for backtracking processing (not serialized)
        const TrueParticle* temp_true_particle_ = nullptr;
        
        // ===== Embedded truth information =====
        // Always stored: generator name for all TPs (from MC truth, not Geant4)
        std::string generator_name_ = "UNKNOWN";
        
        // MARLEY-specific particle truth (only filled when generator is MARLEY)
        int particle_pdg_ = 0;
        std::string particle_process_ = "";
        float particle_energy_ = 0.0f;
        float particle_x_ = 0.0f;
        float particle_y_ = 0.0f;
        float particle_z_ = 0.0f;
        float particle_px_ = 0.0f;
        float particle_py_ = 0.0f;
        float particle_pz_ = 0.0f;
        
        // Neutrino information (only filled for MARLEY TPs with neutrino association)
        std::string neutrino_interaction_ = "";
        float neutrino_x_ = 0.0f;
        float neutrino_y_ = 0.0f;
        float neutrino_z_ = 0.0f;
        float neutrino_px_ = 0.0f;
        float neutrino_py_ = 0.0f;
        float neutrino_pz_ = 0.0f;
        float neutrino_energy_ = 0.0f;

}; // class TriggerPrimitive

#endif // TRIGGERPRIMITIVE_HPP
