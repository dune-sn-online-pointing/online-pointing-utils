#ifndef TRIGGERPRIMITIVE_HPP
#define TRIGGERPRIMITIVE_HPP

#include <vector>
#include "Logger.h"

LoggerInit([]{ Logger::getUserHeader() << "[" << FILENAME << "]"; });
    
// this is an adapted copy from https://github.com/DUNE-DAQ/trgdataformats/blob/develop/include/trgdataformats/TriggerPrimitive.hpp

// this could be a struct, but we don't really have memory contraints here, so alignment is not strictly necessary
// not makin`
class TriggerPrimitive {

    public:
        static constexpr uint8_t s_trigger_primitive_version = 2; // still providing conversion fom version 1 to be done outside the class

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
        std::string view = ""; // easier to handle
        std::string interaction_type = "";
        int event = -1;

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

            // associate the view, for easier handling. Could remove later
            // plane U is 0-799, V is 800-1599, and Z is 1600-2559
            if (channel < 800)          { view = "U"; } 
            else if (channel < 1600)    { view = "V"; } 
            else if (channel < 2559)    { view = "X"; }
            else { LogError("Channel out of range! Critical, stopping execution."); return;}
        };

        // TODO? may switch to getters and setters and private members

}; // class TriggerPrimitive


#endif // TRIGGERPRIMITIVE_HPP