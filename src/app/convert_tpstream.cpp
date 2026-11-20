// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-14 at 23:25:42 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 03:26:01 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 03:30:04 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 18:35:36 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 20:43:52 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 20:55:08 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 21:04:49 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 21:15:21 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-18 at 21:41:07 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 19:39:40 UTC (SetBtDebug call)
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 19:44:42 UTC (Reset() per TP - fixes truth over-matching)
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 19:55:39 UTC (simide_energy sum via simides::SumEnergy)
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 20:37:05 UTC (simide time window)
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 20:41:00 UTC (revert to non-windowed simide match)
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 21:05:00 UTC (neutrino_interaction from filename, mirrors old)
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 21:20:00 UTC (neutrino_interaction from neutrino w > 0.001 GeV)
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 22:24:25 UTC (moved from src/inputs/read_trigger_trees.cpp to src/app/convert_tpstream.cpp)
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-20 at 00:49:34 UTC (output filename: _cnvrt_s{skip}_l{count}_tps.root)
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-20 at 00:55:00 UTC (JobConfig: maxcount/skipevent from json, dropped COUNTER)
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-20 at 01:09:38 UTC (-f override: skip if output exists)
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
#include "TPSClass.h"
#include "mcneutrinos.h"
#include "mcparticles.h"
#include "mctruths.h"
#include "simides.h"
#include "tpstream.h"
#include "JobConfig.h"
#include <filesystem>
//#include "tpmakerTPCabsRS__TPGen.h"
//#include "tpmakerTPCsimpleThr__TPGen.h"

#ifdef INPUTS_STANDALONE
bool debugMode = false;
#endif

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
    // Configuration: -j <json>, -v, -d  (JobConfig also sets verboseMode/debugMode).
    JobConfig config;
    config.readArgs(argc, argv);
    if (!config.jsonFile.empty()) config.readJson(config.jsonFile);

    // Input file: a positional argument (the file the wrapper loops over) when
    // given, otherwise the tpstream_input_file from the JSON config.
    std::string inPath = (argc > 1 && argv[1][0] != '-') ? std::string(argv[1])
                                                         : config.tpstreamInputFile();
    LogThrowIf(inPath.empty(),
               "convert_tpstream: no input file (pass it as an argument or set tpstream_input_file in the json)");

    const int maxcount  = config.maxCount();   // events to process (0 = all), from "maxcount"
    const int skipevent = config.skipEvent();  // events to skip, from "skipevent"

    // Output name mirrors backtrack_tpstream: a "_cnvrt" marker (vs backtrack's
    // "_bktr<margin>") followed by the event skip/count, then "_tps.root".
    TString inFile = inPath.c_str();
    std::string skipper = "_cnvrt_s" + std::to_string(skipevent) + "_l" + std::to_string(maxcount);
    std::string suffix  = skipper + "_tps.root";
    TString outFile = inFile;
    outFile.ReplaceAll(".root", TString(suffix.c_str()));
    // Strip any directory path so the output is written into the current
    // working directory (the input file's directory may be write-protected).
    Ssiz_t _slash = outFile.Last('/');
    if (_slash != kNPOS) outFile.Remove(0, _slash + 1);

    // Unless -f/--override is given, skip if the output already exists, and
    // return success so a sequence/batch moves on to the next file or step.
    if (!config.override && std::filesystem::exists(outFile.Data())) {
        LogInfo << "Output already exists, skipping (use -f to overwrite): " << outFile << std::endl;
        return 0;
    }

    std::unique_ptr<TFile> file(TFile::Open(inPath.c_str(), "READ"));
    if (!file || file->IsZombie()) {
        std::cerr << "Error: could not open ROOT file: " << inPath << '\n';
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
    // Event cap for the FileMap index (0 = all events), from the config.
    int fullcount = maxcount;
    if (maxcount > 0) {
        LogInfo << "Processing at most " << maxcount << " events per file (maxcount).\n";
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
                inputs[tree_path.first] = make_input<tpstream>(tree);
            } else if (tree_path.first == "tpmakerTPCsimpleThr__TPGen") {
                inputs[tree_path.first] = make_input<tpstream>(tree);
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
    // make an instance of TPSClass to write the output tree
    TPSClass tps_class;
    tps_class.Create(outFile, "tps");

    // NOTE: the ES/CC interaction label is now derived per event from the
    // neutrino's w (in the event loop below), not from the filename. ES (nu-e
    // elastic) has w ~ electron mass; CC (on the nucleus) has w ~ Ar-40 mass.

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
            if (maxcount > 0 && counter > maxcount) {
                LogInfo << "Reached maxcount=" << maxcount << " events, stopping.\n";
                break;
            }
            if (tp_count < 1) {
                LogInfo << "Event: " << event_key << " has no trigger primitives.\n";
            }

            std::map<UInt_t, Int_t> g4id_to_pindex;
            std::map<UInt_t, Int_t> truth_to_pindex;


            // DO PARTICLE LOOP FIRST
            UInt_t index = 0;
            bool hasparticles = false;
            const auto particles =
                digger.find_mcparticles(trees.at("mcparticles"), event_key, file_maps.at("mcparticles"));
            {
                ScopeTimer _timer_particles("particle loop (event " + std::to_string(event_key) + ")");
                for (const auto& particle : particles) {
                    hasparticles = true;
                    truth_to_pindex[particle.truth_block_id] = index;
                    g4id_to_pindex[particle.g4_track_id] = index++;
                    if (debugMode) {
                        particle.Print();
                        // LogInfo << "Event: " << event_key << ", Index: " << index - 1 
                        //         << ", Truth Block ID: " << particle.truth_block_id 
                        //         << ", Particle PDG: " << particle.pdg
                        //         << ", g4id: " << particle.g4_track_id
                        //         << ", Energy: " << particle.energy << ", Truth Block ID: " << particle.truth_block_id << "\n";
                    }
                }
            }

            // DO TRUTH LOOP NEXT
            bool hastruths = false;
            int incoming_neutrino_index = -1;
            int outgoing_lepton_index = -1;
            std::string generator_name = "UNKNOWN";
            UInt_t trcount = 0;
            const auto truths =
                digger.find_mctruths(trees.at("mctruths"), event_key, file_maps.at("mctruths"));
            {
                ScopeTimer _timer_truths("truth loop (event " + std::to_string(event_key) + ")");
                for (const auto& truth : truths) {
                    hastruths = true;
                    if (truth.pdg == 12 && truth.status_code == 0) {
                        incoming_neutrino_index = trcount;
                        generator_name = truth.generator_name ? *truth.generator_name : std::string();
                    }
                    if (truth.pdg == 11 && truth.status_code == 1) {
                        outgoing_lepton_index = trcount;
                    }
                    trcount++;
                    if (debugMode) {
                        truth.Print();
                    //     LogInfo << "Event: " << event_key << ", Truth Block ID: " << truth.block_id
                    //             << ", Truth PDG: " << truth.pdg
                    //             << ", Energy: " << truth.energy << "\n";
                    }
                }
            }
            // DO NEUTRINO LOOP NEXT
            bool hasneutrinos = false;
            const auto neutrinos =
                digger.find_mcneutrinos(trees.at("mcneutrinos"), event_key, file_maps.at("mcneutrinos"));
            {
                ScopeTimer _timer_neutrinos("neutrino loop (event " + std::to_string(event_key) + ")");
                for (const auto& neutrino : neutrinos) {
                    hasneutrinos = true;
                    if (debugMode) {
                        neutrino.Print();
                        // LogInfo << "Event: " << event_key << ", Neutrino PDG: " << neutrino.nupdg
                        //       << "\n";
                    }
                }
            }

            // ES/CC interaction label from the neutrino's w (units: GeV).
            // ES (nu-e elastic scattering off an electron) has w ~ 0.000511
            // (electron mass); CC (on the Ar-40 nucleus) has w ~ 37.2 (nucleus
            // mass). The two clusters are ~5 orders apart, so w > 0.001 GeV
            // cleanly selects CC. Applied to matched TPs in WriteSingleTPS.
            std::string interaction_label = "UNKNOWN";
            if (hasneutrinos) {
                interaction_label = (neutrinos.front().w > 0.001) ? "CC" : "ES";
            }
            tps_class.SetInteractionLabel(interaction_label);

            // ONCE ALL IS GATHERED add that info to trigger primitve
                      
            int tpindex = 0;
            
            const auto primitives =
                digger.find_tpstream(trees.at("tpmakerTPCsimpleThr__TPGen"), event_key,
                                     file_maps.at("tpmakerTPCsimpleThr__TPGen"));
            // SimIDEs for this event, used to sum simide_energy per TP by trackID.
            const auto simidelist =
                digger.find_simides(trees.at("simides"), event_key, file_maps.at("simides"));
            {
                ScopeTimer _timer_primitives("primitive loop (event " + std::to_string(event_key) + ")");
                for (const auto& primitive : primitives) {
                    if (primitive.samples_over_threshold < 2) {
                        continue;
                    }

                    // Clear truth associations from the previous TP so a TP only
                    // gets truth that its OWN backtrack sets. Without this the
                    // particle/neutrino pointers leak to later TPs in the event
                    // (over-matching: every TP after the first match looked like
                    // signal). bt_primary_track_id == -99999 TPs stay UNKNOWN.
                    tps_class.Reset();

                    tpindex++;
                    if (debugMode) {
                        LogInfo << "Event: " << event_key << ", TP Index: " << tpindex << ", Primitive Channel: " << primitive.channel
                                << ", g4id: " << primitive.bt_primary_track_id << ", Time Start: " << primitive.time_start << "\n";
                    }

                    if (g4id_to_pindex.find(primitive.bt_primary_track_id) != g4id_to_pindex.end()) {
                        Int_t pindex = g4id_to_pindex[primitive.bt_primary_track_id];
                        // if (debugMode) {
                        //     primitive.Print
                        // }
                            // LogInfo << "Event: " << event_key << ", Primitive Channel: " << primitive.channel
                            //     << ", g4id: " << primitive.bt_primary_track_id << ", Time Start: " << primitive.time_start
                            //     << ", Particle Index: " << pindex << "\n";
                        if (primitive.bt_primary_track_id > 0) {
                            tps_class.SetParticle(particles[pindex]);
                        }
                    }
                    else {
                        if (debugMode) LogInfo << "Event: " << event_key << ", Primitive Channel: " << primitive.channel
                                << ", g4id: " << primitive.bt_primary_track_id << ", Time Start: " << primitive.time_start
                                << ", Particle Index: NOT FOUND\n";
                    }
                    // if (truth_to_pindex.find(primitive.bt_truth_block_id) != truth_to_pindex.end()) {
                    //     Int_t tindex = truth_to_pindex[primitive.bt_truth_block_id];
                    //     tps_class.SetTruth(truths[tindex]);
                    // }

                    // I have a match so use the truth index to find the incoming neutrino and outgoing lepton
                    if (g4id_to_pindex.find(primitive.bt_primary_track_id) != g4id_to_pindex.end()) {
                        if (incoming_neutrino_index >= 0) {
                            tps_class.SetIncomingNeutrino(truths[incoming_neutrino_index]);
                        }
                        if (outgoing_lepton_index >= 0) {
                            tps_class.SetOutgoingLepton(truths[outgoing_lepton_index]);
                        }
                    }

                    
                    if (debugMode) LogInfo << "Event: " << event_key << ", Primitive Channel: " << primitive.channel
                            << ", g4id: " << primitive.bt_primary_track_id << ", Time Start: " << primitive.time_start << "\n";
                    TriggerPrimitive tp;
                    tp.FillFrom(primitive);
                    if (debugMode and (primitive.bt_primary_track_id > -1)) {
                        primitive.Print();
                    }
                    tps_class.SetPrimitive(tp);
                    // DEBUG: carry the raw tpstream backtracking fractions into the output tree
                    tps_class.SetBtDebug(primitive.bt_primary_track_energy_frac,
                                         primitive.bt_primary_track_numelectron_frac,
                                         primitive.bt_primary_track_id);
                    // Sum SimIDE energy for this TP's matched track + channel
                    // (only when the TP has a valid backtrack). Using the
                    // non-time-windowed match for now; the time-windowed overload
                    // exists in simides but needs the timestamp/time_start clocks
                    // reconciled before it is meaningful.
                    if (g4id_to_pindex.find(primitive.bt_primary_track_id) != g4id_to_pindex.end()) {
                        tps_class.SetSimideEnergy(
                            simides::SumEnergy(simidelist, primitive.bt_primary_track_id, primitive.channel));
                    }
                    tps_class.WriteSingleTPS();        
                }
            }
        }
    }
    tps_class.Close();
    return 0;
}
