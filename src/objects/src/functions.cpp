#include "functions.h"


void getPrimitivesForView(
    std::string view, 
    std::vector<TriggerPrimitive>& tps, 
    std::vector<TriggerPrimitive*>& tps_per_view) 
{
    for (auto & tp : tps)
        if (tp.view == view)
            tps_per_view.push_back(&tp);
};

bool isTimeCompatible(int simide_timestamp, TriggerPrimitive tp, int time_window){
    
    // compute the interval going form time_start to time_start + samples_over_threshold
    int start_time = tp.time_start ;
    int end_time = tp.time_start + tp.samples_over_threshold* TPC_sample_length;
    
    if (simide_timestamp >= (start_time - time_window) && simide_timestamp <= (end_time  + time_window) ) // this is very loose, think  again in case
        return true;
    else
        return false;

};
