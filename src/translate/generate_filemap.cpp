#include "generate_filemap.h"



bool superdebugMode=false;
ULong_t RUNOFFSET = 10000000;
int generate_filemap(TTree* TPTree, 
    std::map<const ULong_t, UInt_t> & tp_map_lo, 
    std::map<const ULong_t, UInt_t> & tp_map_hi,
    const UInt_t fullcount){
    
    LogInfo << " Generate filemap for " << TPTree->GetName() << "  with limit " << fullcount << " unique events " <<  std::endl;
    std::set<UInt_t > runs;
    if (!TPTree) {
        std::printf("Error: Tree not found!");
        return -1;
    }
    LogInfo << "mapping " << TPTree->GetName() << " which has " << TPTree->GetEntries() <<  " tps entries " << std::endl;
    ULong_t newevent = 0;
    ULong_t count = 0;
    ULong_t runevent = 0;

    UInt_t event = 0;
    UInt_t run = 0;
    TPTree->SetBranchAddress("event", &event);
    TPTree->SetBranchAddress("run", &run);
    ULong_t firstevent = 0;

    for (UInt_t index = 0; index < TPTree->GetEntries(); ++index){
        TPTree->GetEntry(index); 
        runs.emplace(run); 
        runevent = make_event_key(event, run); //ULong_t(run)*10000000+ULong_t(event);
        if (superdebugMode) { 
            std::printf("event:  count: %ld, runevent: %ld, event: %d, run %d, index: %d\n", count, runevent, event, run, index);
        }
        if (count == 0) firstevent = runevent;
        tp_map_hi[runevent] = index;
        if (runevent != newevent){
            tp_map_lo[runevent] = index;
            count += 1;
            if (debugMode){
                std::printf("new event: count: %ld, runevent: %ld, event: %d, run %d,index: %d\n", count, runevent, event, run, index);
            }
            newevent = runevent;   
            
            if (count > fullcount && fullcount > 0){
                std::printf("Debug: Reached %ld events, stopping early.\n", count);        
                break;
            }
            
            
            
        }
    }
    for (auto run:runs){
        LogInfo << "run: " << run << std::endl;
    }
    if (count > 0){
      if(debugMode) LogInfo << "pointers " << runevent << TPTree->GetName() << tp_map_lo[firstevent] << " " << tp_map_hi[firstevent] << std::endl;
    }
    else{
        std::cout << " no such event in tree " << std::endl;
    }
    LogInfo << " made a map with " << count << " event entries" << std::endl;
    return count;
}   

ULong_t make_event_key(UInt_t event, UInt_t run){
    return (ULong_t)run*RUNOFFSET + (ULong_t)(event);
};

UInt_t event_from_event_key(ULong_t event_key){
    return (UInt_t)(event_key%RUNOFFSET);
};

UInt_t run_from_event_key(ULong_t event_key){
    return (UInt_t)(event_key/RUNOFFSET);
};

// int find_run_event_range(const UInt_t run, const UInt_t event, 
//     std::map<const ULong_t, UInt_t> & tp_map_lo, 
//     std::map<const ULong_t, UInt_t> & tp_map_hi,
//     UInt_t & low_index, UInt_t & high_index){
//     if (!tp_map_lo.contains(run*10000000+event) || !tp_map_hi.contains(run*10000000+event)){
//         low_index = -1;
//         high_index = -1;
//         return 0;
//     }
//     low_index = tp_map_lo[run*10000000+event];
//     high_index = tp_map_hi[run*10000000+event];
//     return 1;
// }
bool SingleEventChecker(const std::vector<TriggerPrimitive> &tps_by_event){
    if (tps_by_event.size() <=0 ) return true;
    std::set<ULong_t> keys;
    int count = 0;
    keys.clear();
   
    for (auto tps:tps_by_event){
        ULong_t event_key = make_event_key(tps.GetEvent(), tps.GetRun());
        keys.emplace(event_key);
        if (keys.size()>1){
            LogError << "mixed events in a tps ";
            LogError << count << ", ";
            for (auto key:keys){
                LogError <<  key << ", ";
            } 
            LogError << std::endl;
        }
        count++;
    }
    return (keys.size() == 1);
}