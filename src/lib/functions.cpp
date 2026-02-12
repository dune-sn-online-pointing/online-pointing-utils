#include "functions.h"
#include <map>
#include <string>
#include <TError.h>

const std::map<std::string, ELogLevel> rootVerbLevelMap = {
    {"kInfo", static_cast<ELogLevel>(kInfo)},
    {"kWarning", static_cast<ELogLevel>(kWarning)},
    {"kError", static_cast<ELogLevel>(kError)},
    {"kBreak", static_cast<ELogLevel>(kBreak)},
    {"kSysError", static_cast<ELogLevel>(kSysError)},
    {"kFatal", static_cast<ELogLevel>(kFatal)}
};

void getPrimitivesForView(
    std::string view, 
    std::vector<TriggerPrimitive>& tps, 
    std::vector<TriggerPrimitive*>& tps_per_view) 
{
    for (auto & tp : tps)
        if (tp.GetView() == view)
            tps_per_view.push_back(&tp);
};

bool isTimeCompatible(TrueParticle* true_particle, TriggerPrimitive* tp, int time_window){

    // LogInfo << "True particle time start: " << true_particle->GetTimeStart() << ", end: " << true_particle->GetTimeEnd() << std::endl;
    // LogInfo << "TP time start: " << tp->GetTimeStart() << ", end: " << tp->GetTimeEnd() << std::endl;
    
    bool verbose = false;

    if (tp->GetTimeStart() < true_particle->GetTimeEnd() + time_window 
        && tp->GetTimeStart() > true_particle->GetTimeStart() - time_window) {
        if (verbose) LogInfo << "TP is time compatible with true particle, tp time start: " << tp->GetTimeStart() << ", true particle time start: " << true_particle->GetTimeStart() << ", end: " << true_particle->GetTimeEnd() << std::endl;
        return true;
    }
    else {
        if (verbose) LogInfo << "TP is NOT time compatible with true particle, tp time start: " << tp->GetTimeStart() << ", true particle time start: " << true_particle->GetTimeStart() << ", end: " << true_particle->GetTimeEnd() << std::endl;

    // // compute the interval going form time_start to time_start + samples_over_threshold
        return false;
    };

    // int start_time = tp->GetTimeStart() ;
    // int end_time = tp->GetTimeStart() + tp->GetSamplesOverThreshold()* TPC_sample_length;
    
    // if (tueP_start_time >= (start_time - time_window) && tueP_start_time <= (end_time  + time_window) ) // this is very loose, think  again in case
    //     return true;
    // else
    //     return false;

};


bool isChannelCompatible (TrueParticle* true_particle, TriggerPrimitive* tp){
    // check if the channel of the tp is in the vector of channels of the true particle
    // note that here we need the Channel of the true particle, not the detector channel
    // if (std::find(true_particle->GetChannels().begin(), true_particle->GetChannels().end(), tp->GetChannel()) != true_particle->GetChannels().end()) {
    //     // LogInfo << "TP is detector " << tp->GetDetector() << ", channel " << tp->GetDetectorChannel() << ", true particle has channels list ";
    //     // for (auto& channel : true_particle->GetChannels()) {
    //     //     LogInfo << channel << " ";
    //     // }
    //     // LogInfo << std::endl;
    //     return true;
    // }

    bool verbose = false;

    if (true_particle->GetChannels().count(tp->GetChannel())) {
        if (verbose) {
            LogInfo << "TP is channel " << tp->GetChannel() << ", true particle has channels list ";
            for (auto& channel : true_particle->GetChannels()) {
                LogInfo << channel << " ";
            }
            LogInfo << std::endl;
        }
       return true; 
    }
    if (verbose) {
        LogInfo << "TP is NOT channel compatible, it is " << tp->GetChannel() << ", true particle has channels list ";
        for (auto& channel : true_particle->GetChannels()) {
            LogInfo << channel << " ";
        }
        LogInfo << std::endl;
    }
    return false;
};


ELogLevel stringToRootLevel(const std::string& level) {
    auto it = rootVerbLevelMap.find(level);
    return (it != rootVerbLevelMap.end()) ? it->second : static_cast<ELogLevel>(kWarning);
}

// Helper function to extract clustering parameters from filename
std::map<std::string, int> extractClusteringParams(const std::string& filename) {
  std::map<std::string, int> params;
  params["tick"] = -1; params["ch"] = -1; params["min"] = -1; params["tot"] = -1;
  std::regex tick_regex("_tick(\\d+)"), ch_regex("_ch(\\d+)"), min_regex("_min(\\d+)"), tot_regex("_tot(\\d+)");
  std::smatch match;
  if (std::regex_search(filename, match, tick_regex) && match.size() > 1) params["tick"] = std::stoi(match[1].str());
  if (std::regex_search(filename, match, ch_regex) && match.size() > 1) params["ch"] = std::stoi(match[1].str());
  if (std::regex_search(filename, match, min_regex) && match.size() > 1) params["min"] = std::stoi(match[1].str());
  if (std::regex_search(filename, match, tot_regex) && match.size() > 1) params["tot"] = std::stoi(match[1].str());
  return params;
}

std::string formatClusteringConditions(const std::map<std::string, int>& params) {
  std::stringstream ss;
  bool first = true;
  if (params.at("ch") >= 0) { ss << "ch:" << params.at("ch"); first = false; }
  if (params.at("tick") >= 0) { if (!first) ss << ", "; ss << "tick:" << params.at("tick"); first = false; }
  if (params.at("tot") >= 0) { if (!first) ss << ", "; ss << "tot:" << params.at("tot"); first = false; }
  if (params.at("min") >= 0) { if (!first) ss << ", "; ss << "min:" << params.at("min"); }
  return ss.str();
}
