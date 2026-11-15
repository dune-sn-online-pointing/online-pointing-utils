// modified using Claude Code with gpt-6-luna by H. Schellman on 2026-11-14 at 22:57:30 UTC
#ifndef FILEMAP_H
#define FILEMAP_H

#include "TFile.h"
#include "TTree.h"

#include <map>
#include <string>
#include <vector>

#include "Global.h"
#include "TriggerPrimitive.hpp"

class FileMap {
public:
    static constexpr ULong_t kRunOffset = 10000000ULL;

    FileMap() = default;
    ~FileMap() = default;

    int generate(TTree* tree, UInt_t fullcount = 0);

    const std::map<ULong_t, UInt_t>& get_tp_map_lo() const { return tp_map_lo_; }
    const std::map<ULong_t, UInt_t>& get_tp_map_hi() const { return tp_map_hi_; }

    static ULong_t make_event_key(UInt_t event, UInt_t run);
    static UInt_t event_from_event_key(ULong_t event_key);
    static UInt_t run_from_event_key(ULong_t event_key);
    static bool single_event_checker(const std::vector<TriggerPrimitive>& tps_by_event);

private:
    std::map<ULong_t, UInt_t> tp_map_lo_;
    std::map<ULong_t, UInt_t> tp_map_hi_;
};

#endif