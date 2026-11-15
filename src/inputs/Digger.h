// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 03:26:01 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 03:30:04 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 21:15:21 UTC
#ifndef DIGGER_H
#define DIGGER_H

#include <chrono>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <TBranch.h>
#include <TObjArray.h>

#include "FileMap.h"
#include "mcneutrinos.h"
#include "mcparticles.h"
#include "mctruths.h"
#include "simides.h"
#include "tpstream.h"
//#include "tpstream.h"
//#include "tpmakerTPCsimpleThr__TPGen.h"

namespace digger_detail {
class BranchAddressGuard {
public:
    explicit BranchAddressGuard(TTree* tree)
        : tree_(tree), make_class_(tree->GetMakeClass()) {
        save_branches(tree->GetListOfBranches());
    }

    ~BranchAddressGuard() {
        for (auto it = addresses_.rbegin(); it != addresses_.rend(); ++it) {
            it->first->SetAddress(it->second);
        }
        tree_->SetMakeClass(make_class_);
    }

private:
    void save_branches(TObjArray* branches) {
        if (!branches) {
            return;
        }
        for (Int_t i = 0; i < branches->GetEntriesFast(); ++i) {
            auto* branch = static_cast<TBranch*>(branches->At(i));
            addresses_.emplace_back(branch, branch->GetAddress());
            save_branches(branch->GetListOfBranches());
        }
    }

    TTree* tree_;
    Int_t make_class_;
    std::vector<std::pair<TBranch*, char*>> addresses_;
};
}

class Digger {
public:
    template <typename Reader>
    std::vector<Reader> find_objects(TTree* tree, ULong_t event_key,
                                     const FileMap& file_map) const {
        if (!tree) {
            throw std::invalid_argument("Digger::find_objects requires a non-null TTree");
        }

        std::vector<Reader> objects;

        // Use the FileMap index to read only the contiguous entry range that
        // belongs to this event, instead of scanning the whole tree. The map
        // stores the first (lo) and last (hi) entry index for each event key.
        const auto& lo_map = file_map.get_tp_map_lo();
        const auto& hi_map = file_map.get_tp_map_hi();
        const auto lo_it = lo_map.find(event_key);
        if (lo_it == lo_map.end()) {
            // This event is not present in this tree's file map; nothing to read.
            if (debugMode) {
                LogInfo << "[timing] Digger::find_objects on " << tree->GetName()
                        << " (event " << event_key << "): event not in file map, skipped\n";
            }
            return objects;
        }
        const Long64_t lo = static_cast<Long64_t>(lo_it->second);
        const Long64_t hi = static_cast<Long64_t>(hi_map.at(event_key));
        // The [lo, hi] range is the exact upper bound on how many objects this
        // event can yield, so reserve up front to avoid reallocations (and the
        // string-allocating re-copies they trigger) during emplace_back.
        objects.reserve(static_cast<std::size_t>(hi - lo + 1));

        digger_detail::BranchAddressGuard branch_addresses(tree);
        const auto delete_reader = [](Reader* reader) {
            reader->fChain = nullptr;
            delete reader;
        };
        std::unique_ptr<Reader, decltype(delete_reader)> reader(
            new Reader(tree), delete_reader);
        if (!reader->b_event || !reader->b_run) {
            throw std::runtime_error("Digger::find_objects requires event and run branches");
        }

        const auto timing_start = std::chrono::steady_clock::now();
        for (Long64_t entry = lo; entry <= hi; ++entry) {
            if (reader->GetEntry(entry) <= 0) {
                throw std::runtime_error("Digger::find_objects failed to read a TTree entry");
            }
            if (FileMap::make_event_key(reader->event, reader->run) == event_key) {
                objects.emplace_back(*reader);
            }
        }
        if (debugMode) {
            const auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - timing_start).count();
            LogInfo << "[timing] Digger::find_objects on " << tree->GetName()
                    << " (event " << event_key << "): scanned " << (hi - lo + 1)
                    << " entries [" << lo << ", " << hi << "], matched "
                    << objects.size() << ", " << elapsed_us << " us\n";
        }
        return objects;
    }

    std::vector<mcparticles> find_mcparticles(TTree* tree, ULong_t event_key,
                                              const FileMap& file_map) const {
        return find_objects<mcparticles>(tree, event_key, file_map);
    }
    std::vector<mcneutrinos> find_mcneutrinos(TTree* tree, ULong_t event_key,
                                              const FileMap& file_map) const {
        return find_objects<mcneutrinos>(tree, event_key, file_map);
    }
    std::vector<mctruths> find_mctruths(TTree* tree, ULong_t event_key,
                                        const FileMap& file_map) const {
        return find_objects<mctruths>(tree, event_key, file_map);
    }
    std::vector<simides> find_simides(TTree* tree, ULong_t event_key,
                                      const FileMap& file_map) const {
        return find_objects<simides>(tree, event_key, file_map);
    }
    std::vector<tpstream> find_tpstream(
        TTree* tree, ULong_t event_key, const FileMap& file_map) const {
        return find_objects<tpstream>(tree, event_key, file_map);
    }
    // std::vector<tpmakerTPCsimpleThr__TPGen> find_tpmakerTPCsimpleThr__TPGen(
    //     TTree* tree, ULong_t event_key, const FileMap& file_map) const {
    //     return find_objects<tpmakerTPCsimpleThr__TPGen>(tree, event_key, file_map);
    // }
};

#endif
