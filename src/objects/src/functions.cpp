#include "functions.h"


void getPrimitivesForView(
    std::string view, 
    std::vector<TriggerPrimitive>& tps, 
    std::vector<TriggerPrimitive*>& tps_per_view) 
{
    for (auto & tp : tps)
        if (tp.GetView() == view)
            tps_per_view.push_back(&tp);
};

bool isTimeCompatible(TrueParticle true_particle, TriggerPrimitive tp, int time_window){
    
    if (tp.GetTimeStart() < true_particle.GetTimeEnd() + time_window 
        && tp.GetTimeStart() > true_particle.GetTimeStart() - time_window) {
        return true;
    }
    else 
        return false;   

    // // compute the interval going form time_start to time_start + samples_over_threshold
    // int start_time = tp.GetTimeStart() ;
    // int end_time = tp.GetTimeStart() + tp.GetSamplesOverThreshold()* TPC_sample_length;
    
    // if (tueP_start_time >= (start_time - time_window) && tueP_start_time <= (end_time  + time_window) ) // this is very loose, think  again in case
    //     return true;
    // else
    //     return false;

};


bool isChannelCompatible (TrueParticle true_particle, TriggerPrimitive tp){
    // check if the channel of the tp is in the vector of channels of the true particle
    if (std::find(true_particle.GetChannels().begin(), true_particle.GetChannels().end(), tp.GetDetectorChannel()) != true_particle.GetChannels().end()) {
        return true;
    }
    else {
        return false;
    }
};