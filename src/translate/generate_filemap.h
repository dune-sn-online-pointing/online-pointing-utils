#ifndef GENERATE_FILEMAP_H
#define GENERATE_FILEMAP_H
#include "TFile.h"
#include "TTree.h"
#include <map>
#include <vector>
#include <string>
//#include <format>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <libgen.h>
#include "global.h"
#include "TriggerPrimitive.hpp"

// function to generate maps by run/event number to objects in a



int generate_filemap(TTree* tree,
    std::map<const ULong_t, UInt_t> & tp_map_lo, 
    std::map<const ULong_t, UInt_t> & tp_map_hi,
    const UInt_t COUNT = 0);

    
ULong_t make_event_key(UInt_t event, UInt_t run);

UInt_t event_from_event_key(ULong_t event_key);

UInt_t run_from_event_key(ULong_t event_key);

bool SingleEventChecker(const std::vector<TriggerPrimitive> &tps_by_event);
#endif