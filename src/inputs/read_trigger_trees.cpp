// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-14 at 23:25:42 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 03:26:01 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 03:30:04 UTC
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>

#include <TFile.h>
#include <TTree.h>

#include "Digger.h"
#include "FileMap.h"
#include "mcneutrinos.h"
#include "mcparticles.h"
#include "mctruths.h"
#include "simides.h"
#include "tpmakerTPCabsRS__TPGen.h"
#include "tpmakerTPCsimpleThr__TPGen.h"

#ifdef INPUTS_STANDALONE
bool debugMode = false;
#endif

#define COUNTER 10
template <typename Reader>
std::shared_ptr<void> make_input(TTree* tree) {
    std::shared_ptr<Reader> reader(new Reader(tree), [](Reader* input) {
        if (input->fChain) {
            input->fChain->ResetBranchAddresses();
            input->fChain = nullptr;
        }
        delete input;
    });
    reader->Init(tree);
    return reader;
}

// RAII scope timer: on destruction it reports the wall-clock time the enclosing
// scope took, but only when debugMode is on. Used to time each loop below.
struct ScopeTimer {
    std::string label_;
    std::chrono::steady_clock::time_point start_;
    explicit ScopeTimer(std::string label)
        : label_(std::move(label)), start_(std::chrono::steady_clock::now()) {}
    ~ScopeTimer() {
        if (debugMode) {
            const auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start_).count();
            LogInfo << "[timing] " << label_ << ": " << elapsed_us << " us\n";
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <root-file>\n";
        return 1;
    }

    std::unique_ptr<TFile> file(TFile::Open(argv[1], "READ"));
    if (!file || file->IsZombie()) {
        std::cerr << "Error: could not open ROOT file: " << argv[1] << '\n';
        return 1;
    }
    Digger digger;
    const std::map<std::string, std::string> tree_paths = {
        {"mcparticles", "triggerAna/mcparticles"},
        {"mctruths", "triggerAna/mctruths"},
        {"mcneutrinos", "triggerAna/mcneutrinos"},
        {"simides", "triggerAna/simides"},
        {"tpmakerTPCabsRS__TPGen", "triggerAna/TriggerPrimitives/tpmakerTPCabsRS__TPGen"},
        {"tpmakerTPCsimpleThr__TPGen", "triggerAna/TriggerPrimitives/tpmakerTPCsimpleThr__TPGen"}
    };
    std::map<std::string, FileMap> file_maps;
    std::map<std::string, std::shared_ptr<void>> inputs;
    std::map<std::string, TTree*> trees;
    int fullcount = 0;
    if (COUNTER > 0) {
        LogInfo << "Debug mode is ON. Processing only the first " << COUNTER << " events.\n";
        fullcount = COUNTER; 
        debugMode = true;      
    }
    {
    ScopeTimer _timer_setup("tree setup loop (read trees + build file maps)");
    for (const auto& tree_path : tree_paths) {
        TTree* tree = nullptr;
        file->GetObject(tree_path.second.c_str(), tree);
        if (!tree) {
            std::cerr << "Error: missing TTree: " << tree_path.second << '\n';
            return 1;
        }

        if (tree_path.first == "mcparticles") {
            inputs[tree_path.first] = make_input<mcparticles>(tree);
        } else if (tree_path.first == "mctruths") {
            inputs[tree_path.first] = make_input<mctruths>(tree);
        } else if (tree_path.first == "mcneutrinos") {
            inputs[tree_path.first] = make_input<mcneutrinos>(tree);
        } else if (tree_path.first == "simides") {
            inputs[tree_path.first] = make_input<simides>(tree);
        } else if (tree_path.first == "tpmakerTPCabsRS__TPGen") {
            inputs[tree_path.first] = make_input<tpmakerTPCabsRS__TPGen>(tree);
        } else if (tree_path.first == "tpmakerTPCsimpleThr__TPGen") {
            inputs[tree_path.first] = make_input<tpmakerTPCsimpleThr__TPGen>(tree);
        } else {
            std::cerr << "Error: no reader configured for TTree: " << tree_path.first << '\n';
            return 1;
        }
        trees[tree_path.first] = tree;
        file_maps[tree_path.first] = FileMap();
        file_maps[tree_path.first].generate(tree, fullcount);
        LogInfo << tree_path.first << ": " << tree->GetEntries() << " entries\n";
        LogInfo << tree_path.first << ": " << file_maps[tree_path.first].get_tp_map_lo().size() << " unique events\n";
    }
    }
    std::set<ULong_t> event_keys;
    {
    ScopeTimer _timer_keys("event-key collection loop");
    for (const auto& tree_map : file_maps) {
        for (const auto& event_entry : tree_map.second.get_tp_map_lo()) {
            event_keys.insert(event_entry.first);
        }
    }
    }

    int counter = 0;
    {
    ScopeTimer _timer_events("main event processing loop");
    for (const auto event_key : event_keys) {
        if (debugMode) {
            LogInfo << "Processing event: " << event_key << "\n";
        }
        const int tp_count = file_maps["tpmakerTPCabsRS__TPGen"].get_tp_map_hi().at(event_key) -
            file_maps["tpmakerTPCabsRS__TPGen"].get_tp_map_lo().at(event_key) + 1;
        
        if (debugMode) {
            LogInfo << "Event: " << event_key << ", has tp " << tp_count << "\n";
        }
        ++counter;
        if (debugMode && counter > COUNTER) {
            LogInfo << "Debug mode limit reached, stopping after " << COUNTER << " events.\n";
            break;
        }
        if (tp_count < 1) {
            LogInfo << "Event: " << event_key << " has no trigger primitives.\n";
        }

        std::map<UInt_t, Int_t> g4id_to_pindex;
        UInt_t index = 0;
        const auto particles =
            digger.find_mcparticles(trees.at("mcparticles"), event_key, file_maps.at("mcparticles"));
        {
        ScopeTimer _timer_particles("particle loop (event " + std::to_string(event_key) + ")");
        for (const auto& particle : particles) {
            g4id_to_pindex[particle.g4_track_id] = index++;
            LogInfo << "Event: " << event_key << ", Index: " << index - 1 << ", Particle PDG: " << particle.pdg
                    << ", g4id: " << particle.g4_track_id
                    << ", Energy: " << particle.energy << ", Truth Block ID: " << particle.truth_block_id << "\n";
        }
        }

        int tpindex = 0;
        const auto primitives =
            digger.find_tpmakerTPCsimpleThr__TPGen(trees.at("tpmakerTPCsimpleThr__TPGen"), event_key,
                                                   file_maps.at("tpmakerTPCsimpleThr__TPGen"));
        {
        ScopeTimer _timer_primitives("primitive loop (event " + std::to_string(event_key) + ")");
        for (const auto& primitive : primitives) {
            if (primitive.samples_over_threshold < 2) {
                continue;
            }
            tpindex++;
            LogInfo << "Event: " << event_key << ", TP Index: " << tpindex << ", Primitive Channel: " << primitive.channel 
                    << ", g4id: " << primitive.bt_primary_track_id << ", Time Start: " << primitive.time_start << "\n";
            if (g4id_to_pindex.find(primitive.bt_primary_track_id) != g4id_to_pindex.end()) {
                Int_t pindex = g4id_to_pindex[primitive.bt_primary_track_id];
                LogInfo << "Event: " << event_key << ", Primitive Channel: " << primitive.channel 
                        << ", g4id: " << primitive.bt_primary_track_id << ", Time Start: " << primitive.time_start 
                        << ", Particle Index: " << pindex << "\n";
            } else {
                LogInfo << "Event: " << event_key << ", Primitive Channel: " << primitive.channel 
                        << ", g4id: " << primitive.bt_primary_track_id << ", Time Start: " << primitive.time_start 
                        << ", Particle Index: NOT FOUND\n";
            }
            LogInfo << "Event: " << event_key << ", Primitive Channel: " << primitive.channel
                    << ", g4id: " << primitive.bt_primary_track_id << ", Time Start: " << primitive.time_start << "\n";
        }
        }


        const auto truths =
            digger.find_mctruths(trees.at("mctruths"), event_key, file_maps.at("mctruths"));
        {
        ScopeTimer _timer_truths("truth loop (event " + std::to_string(event_key) + ")");
        for (const auto& truth : truths) {
            LogInfo << "Event: " << event_key << ", Truth PDG: " << truth.pdg
                    << ", Energy: " << truth.energy << "\n";
        }
        }
    }
    }
    return 0;
}