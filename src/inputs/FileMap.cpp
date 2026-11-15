// modified using Claude Code with gpt-6-luna by H. Schellman on 2026-11-14 at 23:16:35 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 03:34:05 UTC
#include "FileMap.h"

#include <set>

namespace {
bool superdebugMode = false;

// Contiguity guard toggle. When true, generate() checks that all entries for a
// given event are contiguous in the tree. The file map records only one
// [lo, hi] entry range per event (and Digger reads only that range), so if a
// tree interleaves events the range would miss some entries. Set this to false
// to disable the check entirely.
bool checkEventContiguity = true;
}

int FileMap::generate(TTree* tree, const UInt_t fullcount) {
    if (!tree) {
        std::printf("Error: Tree not found!\n");
        return -1;
    }

    tp_map_lo_.clear();
    tp_map_hi_.clear();

    LogInfo << "Generate filemap for " << tree->GetName() << " with limit "
            << fullcount << " unique events" << std::endl;

    std::set<UInt_t> runs;
    ULong_t unique_event_count = 0;
    ULong_t previous_event_key = 0;
    ULong_t first_event_key = 0;
    bool has_previous_event = false;
    UInt_t noncontiguous_count = 0;

    UInt_t event = 0;
    UInt_t run = 0;
    tree->SetBranchAddress("event", &event);
    tree->SetBranchAddress("run", &run);

    LogInfo << "Mapping " << tree->GetName() << " which has " << tree->GetEntries()
            << " TP entries" << std::endl;

    for (UInt_t index = 0; index < tree->GetEntries(); ++index) {
        tree->GetEntry(index);
        runs.insert(run);

        const ULong_t event_key = FileMap::make_event_key(event, run);
        if (superdebugMode) {
            std::printf("event: count=%lu, runevent=%llu, event=%u, run=%u, index=%u\n",
                        static_cast<unsigned long>(unique_event_count),
                        static_cast<unsigned long long>(event_key), event, run, index);
        }

        if (!has_previous_event || event_key != previous_event_key) {
            // Contiguity guard: if we are starting a "new" event block but this
            // event key was already recorded earlier, its entries are split by
            // other events, so the [lo, hi] range will not cover all of them.
            if (checkEventContiguity && tp_map_lo_.find(event_key) != tp_map_lo_.end()) {
                ++noncontiguous_count;
                LogError << "Non-contiguous event in tree " << tree->GetName()
                         << ": run " << FileMap::run_from_event_key(event_key)
                         << ", event " << FileMap::event_from_event_key(event_key)
                         << " (key " << event_key << ") reappears at entry index "
                         << index << " after other events; its file-map range will "
                         << "be incomplete." << std::endl;
            }
            if (superdebugMode) {
                std::printf("new event: count=%lu, runevent=%llu, event=%u, run=%u, index=%u\n",
                            static_cast<unsigned long>(unique_event_count),
                            static_cast<unsigned long long>(event_key), event, run, index);
            }

            if (fullcount > 0 && unique_event_count >= fullcount) {
                std::printf("Debug: Reached %lu events, stopping early.\n",
                            static_cast<unsigned long>(unique_event_count));
                break;
            }

            if (!has_previous_event) {
                first_event_key = event_key;
            }
            tp_map_lo_[event_key] = index;
            previous_event_key = event_key;
            has_previous_event = true;
            ++unique_event_count;
        }
        tp_map_hi_[event_key] = index;
    }

    for (const auto run_id : runs) {
        LogInfo << "run: " << run_id << std::endl;
    }

    if (unique_event_count > 0) {
        if (debugMode) {
            LogInfo << "pointers " << previous_event_key << " " << tree->GetName() << " "
                    << tp_map_lo_[first_event_key] << " " << tp_map_hi_[first_event_key] << std::endl;
        }
    } else {
        std::cout << "No such event in tree" << std::endl;
    }

    if (checkEventContiguity && noncontiguous_count > 0) {
        LogError << "Contiguity guard: " << noncontiguous_count << " event(s) in tree "
                 << tree->GetName() << " were non-contiguous; file-map ranges for "
                 << "those events are incomplete. (Set checkEventContiguity = false "
                 << "at the top of FileMap.cpp to silence this check.)" << std::endl;
    }

    LogInfo << "Made a map with " << unique_event_count << " event entries" << std::endl;
    return static_cast<int>(unique_event_count);
}

ULong_t FileMap::make_event_key(UInt_t event, UInt_t run) {
    return static_cast<ULong_t>(run) * kRunOffset + static_cast<ULong_t>(event);
}

UInt_t FileMap::event_from_event_key(ULong_t event_key) {
    return static_cast<UInt_t>(event_key % kRunOffset);
}

UInt_t FileMap::run_from_event_key(ULong_t event_key) {
    return static_cast<UInt_t>(event_key / kRunOffset);
}

bool FileMap::single_event_checker(const std::vector<TriggerPrimitive>& tps_by_event) {
    if (tps_by_event.empty()) {
        return true;
    }

    std::set<int> event_ids;
    for (std::size_t i = 0; i < tps_by_event.size(); ++i) {
        const auto& tp = tps_by_event.at(i);
        event_ids.insert(tp.GetEvent());

        if (event_ids.size() > 1) {
            LogError << "Mixed events in a TP set at index " << i << ": ";
            for (const auto event_id : event_ids) {
                LogError << event_id << ", ";
            }
            LogError << std::endl;
            return false;
        }
    }

    return event_ids.size() == 1;
}