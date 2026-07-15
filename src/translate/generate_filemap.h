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
#include "../lib/global.h"

// function to generate maps by run/event number to objects in a



int generate_filemap(TTree* tree,
    std::map<const ULong_t, UInt_t> & tp_map_lo, 
    std::map<const ULong_t, UInt_t> & tp_map_hi,
    const UInt_t COUNT = 0);

int find_run_event_range(const UInt_t run, const UInt_t event, 
    std::map<const ULong_t, UInt_t> & tp_map_lo, 
    std::map<const ULong_t, UInt_t> & tp_map_hi,
    UInt_t & low_index, UInt_t & high_index);
    
#endif