#ifndef TRIGGERPRIMITIVE_HPP
#define TRIGGERPRIMITIVE_HPP

#include <vector>
#include <string>

#include <utils.h>
#include <Logger.h>
#include <TrueParticle.h>

LoggerInit([]{ Logger::getUserHeader() << "[" << FILENAME << "]"; });
    
class TriggerPrimitive {

    public:
        static constexpr uint8_t s_trigger_primitive_version = 2;

        // Metadata.
        uint64_t version : 8;
        uint64_t flag : 8;
        uint64_t detid : 8;
        
        // Physics data.
        uint64_t channel : 24;
        uint64_t samples_over_threshold : 16;
        uint64_t time_start : 64;
        uint64_t samples_to_peak : 16;
        uint64_t adc_integral : 32;
        uint64_t adc_peak : 16;

        // Additional variables
        int detector = -1;
        int detector_channel = -1;
        std::string view = "";
        int event = -1;

        TrueParticle * true_particle = nullptr;

        // Setters
        void SetTrueParticle(TrueParticle *true_particle) { this->true_particle = true_particle; }
        void SetTimeStart(uint64_t time_start) { this->time_start = time_start; }
        void SetSamplesOverThreshold(uint64_t samples_over_threshold) { this->samples_over_threshold = samples_over_threshold; }
        void SetSamplesToPeak(uint64_t samples_to_peak) { this->samples_to_peak = samples_to_peak; }
        void SetAdcIntegral(uint64_t adc_integral) { this->adc_integral = adc_integral; }
        void SetAdcPeak(uint64_t adc_peak) { this->adc_peak = adc_peak; }
        void SetView(const std::string& view) { this->view = view; }
        void SetDetector(int detector) { this->detector = detector; }
        void SetDetectorChannel(int detector_channel) { this->detector_channel = detector_channel; }
        void SetEvent(int event) { this->event = event; }

        // Getters
        double TimeStart() const { return time_start; }
        double TimeEnd() const { return time_start + samples_over_threshold * TPC_sample_length; }
        std::string GetView() const { return view; }
        int GetDetector() const { return detector; }
        int GetDetectorChannel() const { return detector_channel; }
        int GetEvent() const { return event; }
        int GetSamplesOverThreshold() const { return samples_over_threshold; }
        int GetSamplesToPeak() const { return samples_to_peak; }
        int GetAdcIntegral() const { return adc_integral; }
        int GetAdcPeak() const { return adc_peak; }

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
        ) : version(version),
            flag(flag),
            detid(detid),
            channel(channel),
            samples_over_threshold(samples_over_threshold),
            time_start(time_start),
            samples_to_peak(samples_to_peak),
            adc_integral(adc_integral),
            adc_peak(adc_peak)
        {
            if (version != s_trigger_primitive_version) {
                static std::once_flag tp_version_warning;
                std::call_once(tp_version_warning, []() {
                    LogWarning("TriggerPrimitive version is not 2, be sure to have converted time_peak to samples_to_peak");
                });
            }
            
            detector_channel = channel % APA::total_channels;
            detector = channel / APA::total_channels;

            if (detector_channel < APA::induction_channels)                                      { view = "U"; } 
            else if (detector_channel < APA::induction_channels * 2)                             { view = "V"; }
            else if (detector_channel < APA::induction_channels * 2 + APA::collection_channels)  { view = "X"; }
            else { 
                LogError << "Channel out of range: " << detector_channel << "! Critical, stopping execution.\n"; 
                throw std::runtime_error("Channel out of range: ");
            }
        };

}; // class TriggerPrimitive

#endif // TRIGGERPRIMITIVE_HPP
