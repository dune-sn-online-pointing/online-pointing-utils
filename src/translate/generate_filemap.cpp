#include "generate_filemap.h"





int generate_filemap(TTree* TPTree, 
    std::map<const ULong_t, UInt_t> & tp_map_lo, 
    std::map<const ULong_t, UInt_t> & tp_map_hi,
    const UInt_t COUNT){
    bool DEBUG = false;
   
    if (!TPTree) {
        std::printf("Error: Tree not found!");
        return -1;
    }
    
    ULong_t newevent = -1;
    ULong_t count = 0;
    ULong_t runevent = -1;
    UInt_t event = -1;
    UInt_t run = -1;
    TPTree->SetBranchAddress("event", &event);
    TPTree->SetBranchAddress("run", &run);

    for (UInt_t index = 0; index < TPTree->GetEntries(); ++index){
        TPTree->GetEntry(index);    
        runevent = run*10000000+event;
        
        if (DEBUG) { 
            std::printf("event:  count: %ld, runevent: %ld, index: %d\n", count, runevent, index);
        }
        tp_map_hi[runevent] = index;
        if (runevent != newevent){
            count += 1;
            if (DEBUG){
                std::printf("new event: count: %ld, runevent: %ld, index: %d\n", count, runevent, index);
            }
            newevent = runevent;   
            
            if (count > COUNT && COUNT > 0){
                std::printf("Debug: Reached %ld events, stopping early.\n", count);        
                break;
            }
            
            tp_map_lo[runevent] = index;
        }
    }
    return count;
}   

int find_run_event_range(const UInt_t run, const UInt_t event, 
    std::map<const ULong_t, UInt_t> & tp_map_lo, 
    std::map<const ULong_t, UInt_t> & tp_map_hi,
    UInt_t & low_index, UInt_t & high_index){
    if (!tp_map_lo.contains(run*10000000+event) || !tp_map_hi.contains(run*10000000+event)){
        low_index = -1;
        high_index = -1;
        return 0;
    }
    low_index = tp_map_lo[run*10000000+event];
    high_index = tp_map_hi[run*10000000+event];
    return 1;
}
