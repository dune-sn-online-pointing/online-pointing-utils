#ifndef WRITE_TPS_SIMPLE_H
#define WRITE_TPS_SIMPLE_H

#include <string>
#include <vector>
#include "TFile.h"
#include "TTree.h"
#include "TriggerPrimitive.hpp"
//#include "TrueParticle.h"
//#include "Neutrino.h"


TTree* open_tps_simple(
    std::string out_filename,
    TFile * outFile
);

void write_tps_single_event(
    TFile * outFile,
	TTree * tpTree,
	const std::vector<TriggerPrimitive>& tps_by_event
);

void close_tps_simple(
	TFile *  out_file_pointer,
	int n_events,
    int n_tps_total,
    float bt_error_margin);

void write_tps_simple(std::string out_filename, 
    const std::vector<std::vector<TriggerPrimitive>> tps_vec,
    const float bt_error_margin=0);


#endif // WRITE_TPS_SIMPLE_H