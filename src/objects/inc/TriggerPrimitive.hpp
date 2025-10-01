#ifndef TRIGGERPRIMITIVE_HPP
#define TRIGGERPRIMITIVE_HPP

#include <vector>
#include <string>
#include <Rtypes.h> // For ROOT types like UInt_t

#include <utils.h>
#include <Logger.h>
#include <TrueParticle.h>


class TriggerPrimitive {

    public:
        static constexpr uint8_t s_trigger_primitive_version = 2;

        // Setters
        void SetTrueParticle(const TrueParticle* true_particle)       { this->true_particle_ = true_particle; }
        void SetTimeStart(uint64_t time_start)                        { this->time_start_ = time_start; }
        void SetSamplesOverThreshold(uint64_t samples_over_threshold) { this->samples_over_threshold_ = samples_over_threshold; }
        void SetSamplesToPeak(uint64_t samples_to_peak)               { this->samples_to_peak_ = samples_to_peak; }
        void SetAdcIntegral(uint64_t adc_integral)                    { this->adc_integral_ = adc_integral; }
        void SetAdcPeak(uint64_t adc_peak)                            { this->adc_peak_ = adc_peak; }
        void SetView(const std::string& view)                         { this->view_ = view; }
        void SetDetector(int detector)                                { this->detector_ = detector; }
        void SetDetectorChannel(int detector_channel)                 { this->detector_channel_ = detector_channel; }
        void SetEvent(int event)                                      { this->event_ = event; }
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
        const TrueParticle* GetTrueParticle() const {
            if (true_particle_ == nullptr) {
                // LogWarning << "No true particle associated to this TriggerPrimitive" << std::endl;
            }
            return true_particle_;
        }
        const std::string GetGeneratorName() const {
            if (true_particle_ == nullptr) {
                // LogWarning << "No true particle associated to this TriggerPrimitive" << std::endl;
                return "UNKNOWN";
            }
            else
                return true_particle_->GetGeneratorName();
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

        const TrueParticle* true_particle_ = nullptr;

}; // class TriggerPrimitive

#endif // TRIGGERPRIMITIVE_HPP
