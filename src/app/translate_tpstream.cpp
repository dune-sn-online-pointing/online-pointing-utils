#include "../translate/read_tpstream_simple.h"
#include "../translate/write_tps_simple.h"
#include "../translate/generate_filemap.h"
#include "../translate/TriggerPrimitiveSimple.hpp"
#include "../translate/TrueParticleSimple.h"
#include "../translate/NeutrinoSimple.h"

#include <memory>

int COUNT=10000000;

#define STANDARD_FORMAT
LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

int main(int argc, char* argv[]) {
    // Ensure parameters are loaded
    ParametersManager::getInstance().loadParameters();
    
    CmdLineParser clp;
    clp.getDescription() << "> backtrack app - extract TPs and attach truth, writing *_tps_bktr<N>.root files (N: backtracker_error_margin)."<< std::endl;
    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");
    clp.addOption("outFolder", {"--output-folder"}, "Output folder path (default: data)");
    clp.addOption("inputFile", {"-i", "--input-file"}, "Input file with list OR single ROOT file path (overrides JSON inputs)");
    clp.addOption("bktrMargin", {"--bktr-margin"}, "Override backtracker_error_margin (int)");
    clp.addOption("maxFiles", {"--max-files"}, "Maximum number of files to process (overrides JSON max_files)");
    clp.addOption("skipFiles", {"--skip-files"}, "Number of files to skip at start (overrides JSON skip_files)");
    clp.addDummyOption("Triggers");
    clp.addTriggerOption("verboseMode", {"-v", "--verbose"}, "Run in verbose mode");
    clp.addTriggerOption("debugMode", {"-d", "--debug"}, "Run in debug mode (more detailed than verbose)");
    clp.addTriggerOption("overrideMode", {"-f", "--override"}, "Force reprocessing even if output already exists");
    clp.addDummyOption();
    LogInfo << clp.getDescription().str() << std::endl;
    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;
    clp.parseCmdLine(argc, argv);
    LogThrowIf( clp.isNoOptionTriggered(), "No option was provided." );

    // Set logging verbosity based on command line options
    if (clp.isOptionTriggered("debugMode")) debugMode = true; // global variable
    if (clp.isOptionTriggered("verboseMode")) verboseMode = true; // global variable
    bool overrideMode = clp.isOptionTriggered("overrideMode");

    std::string json = clp.getOptionVal<std::string>("json");
    LogInfo << "Loading JSON config: " << json << std::endl;
    std::ifstream i(json);
    LogThrowIf(!i.good(), "Failed to open JSON config: " << json);
    nlohmann::json j; i >> j;

    // get maxcount and skip from json file
    int maxcount = -1;
    int skipevents = -1;
    if (j.contains("maxcount")) {
        try { maxcount = j.at("maxcount").get<int>(); }
        catch (...) { LogWarning << "Failed to read 'maxcount' from JSON, using default (-1 for no limit)." << std::endl; 
        }
    }
    if (j.contains("skipevents")) {
        try { skipevents = j.at("skipevents").get<int>(); }
        catch (...) { LogWarning << "Failed to read 'skipevents' from JSON, using default (-1 for no skip)." << std::endl; }
    }

    
    //Determine backtracker_error_margin value: CLI > JSON > default from parameters/timing.h
    int bktr_margin = backtracker_error_margin; // from parameters/timing.h
    if (j.contains("backtracker_error_margin")) {
        try { bktr_margin = j.at("backtracker_error_margin").get<int>(); }
        catch (...) { /* ignore and keep default */ }
    }
    if (clp.isOptionTriggered("bktrMargin")) {
        bktr_margin = clp.getOptionVal<int>("bktrMargin");
    }
    LogInfo << "Using backtracker_error_margin: " << bktr_margin << std::endl;
    auto is_tpstream = [](const std::string& path){
        std::string basename = path.substr(path.find_last_of("/\\") + 1);
        return basename.length() >= 14 && basename.substr(basename.length()-14) == "_tpstream.root";
    };
    auto file_exists = [](const std::string& path){ std::ifstream f(path); return f.good(); };

    std::vector<std::string> filenames;
    filenames.reserve(64); // arbitrary, could reconsider TODO

    // Priority 1: CLI --input-file
    if (clp.isOptionTriggered("inputFile")) {
        std::string in = clp.getOptionVal<std::string>("inputFile");
        LogInfo << "Input specified on CLI: " << in << std::endl;
        if (file_exists(in)) {
            if (is_tpstream(in)) filenames.push_back(in);
            else {
                // Treat as list file
                std::ifstream infile(in);
                std::string line;
                while (std::getline(infile, line)) {
                    if (line.size() >= 3 && line.substr(0,3) == "###") break;
                    if (line.empty() || line[0] == '#') continue;
                    if (!file_exists(line)) { LogWarning << "Skipping (missing): " << line << std::endl; continue; }
                    if (!is_tpstream(line)) { LogWarning << "Skipping (not *_tpstream.root): " << line << std::endl; continue; }
                    filenames.push_back(line);
                }
            }
        } else {
            LogError << "CLI input path does not exist: " << in << std::endl;
        }
    }

    // Get skip/max parameters (CLI overrides JSON)
    int skip_files = j.value("skip_files", 0);
    int max_files = j.value("max_files", -1);
    
    if (clp.isOptionTriggered("skipFiles")) {
        skip_files = clp.getOptionVal<int>("skipFiles");
    }
    if (clp.isOptionTriggered("maxFiles")) {
        max_files = clp.getOptionVal<int>("maxFiles");
    }

    // If no CLI, use utility function for JSON-based file finding with tpstream basename logic
    if (filenames.empty()) {
        filenames = find_input_files_by_tpstream_basenames(j, "tpstream", skip_files, max_files);
    }

    LogInfo << "Number of valid files (after skip/max): " << filenames.size() << std::endl;
    LogThrowIf(filenames.empty(), "No valid input files.");

    // Output folder: CLI outFolder > JSON tps_folder > Auto-generate from main_folder/signal_folder
    std::string outfolder;
    if (clp.isOptionTriggered("outFolder")) {
        outfolder = clp.getOptionVal<std::string>("outFolder");
    } else if (j.contains("tps_folder") && !j["tps_folder"].get<std::string>().empty()) {
        try { outfolder = j.at("tps_folder").get<std::string>(); } catch (...) { /* ignore */ }
    } else if (j.contains("sig_folder") && !j["sig_folder"].get<std::string>().empty()) {
        try { outfolder = j.at("sig_folder").get<std::string>(); } catch (...) { /* ignore */ }
    } else if (j.contains("outputFolder") && !j["outputFolder"].get<std::string>().empty()) {
        try { outfolder = j.at("outputFolder").get<std::string>(); } catch (...) { /* ignore */ }
    }
    
    if (outfolder.empty()) {
        // Auto-generate from main_folder or signal_folder
        if (j.contains("main_folder") && !j["main_folder"].get<std::string>().empty()) {
            outfolder = (std::filesystem::path(j["main_folder"].get<std::string>()) / "tps").string();
        } else if (j.contains("signal_folder") && !j["signal_folder"].get<std::string>().empty()) {
            outfolder = (std::filesystem::path(j["signal_folder"].get<std::string>()) / "tps").string();
        } else {
            // Fallback to tpstream_folder location
            outfolder = j.value("tpstream_folder", std::string("."));
            // Remove trailing slash if present
            if (!outfolder.empty() && outfolder.back() == '/') {
                outfolder.pop_back();
            }
        }
    }
    
    // Ensure output folder exists
    if (!ensureDirectoryExists(outfolder)) {
        LogError << "Failed to create output folder: " << outfolder << std::endl;
        return 1;
    }
    
    LogInfo << "Output folder (pure signal TPs): " << outfolder << std::endl;

    std::vector<std::string> output_files;

    std::vector<std::vector<TriggerPrimitiveSimple>> tps;
    std::vector<std::vector<TrueParticleSimple>> true_particles;
    std::vector<std::vector<NeutrinoSimple>> neutrinos;

    // Effective time window for TP<->truth association in TDC ticks (base 1 TPC sample + margin in TPC samples)
    int effective_time_window = (1 + bktr_margin) * conversion_tdc_to_tpc;
    LogInfo << "Effective time window (TDC ticks): " << effective_time_window << " (conversion_tdc_to_tpc=" << conversion_tdc_to_tpc << ")" << std::endl;

    int channel_tolerance = 0; // default fallback
    if (j.contains("backtracker_channel_tolerance")) {
        try { channel_tolerance = j.at("backtracker_channel_tolerance").get<int>(); }
        catch (...) { LogWarning << "Invalid backtracker_channel_tolerance in JSON, keeping default (50)." << std::endl; }
    }
    LogInfo << "Channel tolerance (channels): " << channel_tolerance << std::endl;

    int done_files = 0;

    for (auto& filename : filenames) {

        done_files++;
        if (max_files > 0 && done_files > max_files){
            LogInfo << "reached " << max_files << " files, breaking loop after max_files iterations." << std::endl;
            break; }
        GenericToolbox::displayProgressBar(done_files, filenames.size(), "Processing files...");

        // Compute expected output path early to allow skip-if-exists behavior
        std::string input_basename = filename.substr(filename.find_last_of("/\\") + 1);
        input_basename = input_basename.substr(0, input_basename.length() - 14); // remove _tpstream.root
        std::ostringstream suffix;
        std::string skipper = "";
        if (maxcount > 0 || skipevents > -1) {
            skipper =std::format("_s{}_l{}",skipevents,maxcount);
        } 
        // if (bktr_margin != standard_backtracker_error_margin) {
        //     suffix << skipper << "_tps_bktr" << bktr_margin << ".root";
        // } else {
            suffix << skipper << "_tps.root";
        // 
        std::string out = outfolder + "/" + input_basename + suffix.str();
        // Use absolute path for output
        std::error_code _ec_abs;
        std::filesystem::path out_abs_p = std::filesystem::absolute(std::filesystem::path(out), _ec_abs);
        std::string out_abs = _ec_abs ? out : out_abs_p.string();

        // Skip processing if output already exists and override is not set
        if (!overrideMode && file_exists(out_abs)) {
            LogInfo << "Output already exists, skipping: " << out_abs 
                    << " (use --override to force reprocessing)" << std::endl;
            done_files--;
            output_files.push_back(out_abs);
            continue;
        }

        if (verboseMode) LogInfo << "Reading file: " << filename << std::endl;
        // count events
        // using this tree just because it's the smallest
        // #ifdef STANDARD_FORMAT
        // std::string MCtree_path = "triggerAna/mctruths";
        // #else
        // std::string MCtree_path = "triggerAnaDumpTPs/mctruths";
        // #endif
        std::unique_ptr<TFile> file(TFile::Open(filename.c_str(), "READ"));
        if (!file || file->IsZombie()) { LogError << "Failed to open file: " << filename << std::endl; continue; }
                // TTree *MCtree = dynamic_cast<TTree*>(file->Get(MCtree_path.c_str()));
                // if (!MCtree) { LogError << "Tree not found: " << MCtree_path << std::endl; continue; }

        

        //-------------

                        
    // if (debugMode) LogInfo << " Reading file: " << filename << std::endl;

    // std::unique_ptr<TFile> file(TFile::Open(filename.c_str(), "READ"));
    // if (!file || file->IsZombie()) {
    //     LogError << " Error opening file: " << filename << std::endl;
    //     return;
    // }
    
        std::string this_interaction = "UNKNOWN";
        // Extract interaction type from filename by looking for exact _cc_ or _es_ patterns
        // Check _cc_ first to avoid false matches with substrings
        if (filename.find("cc_") != std::string::npos) {
            this_interaction = "CC";
        } else if (filename.find("CC_") != std::string::npos) {
            this_interaction = "CC";
        } else if (filename.find("es_") != std::string::npos) {
            this_interaction = "ES";
        } else if (filename.find("ES_") != std::string::npos) {
            this_interaction = "ES";
        } else {
            this_interaction = "UNKNOWN";
        }

        if (verboseMode) LogInfo << " For this file, interaction type: " << this_interaction << std::endl;
        #ifdef STANDARD_FORMAT
        std::string TPtree_path = "triggerAna/TriggerPrimitives/tpmakerTPCsimpleThr__TPGen"; // TODO make flexible for 1x2x6 and maybe else    
        #else
        std::string TPtree_path = "triggerAnaDumpTPs/TriggerPrimitives/tpmakerTPC__TriggerAnaTree1x2x2"; // TODO make flexible for 1x2x6 and maybe else
        #endif
        TTree *TPtree = dynamic_cast<TTree*>(file->Get(TPtree_path.c_str()));
        if (!TPtree) {
            LogError << " Tree not found: " << TPtree_path << std::endl;
            continue; // can still go to next file
        }

        #ifdef STANDARD_FORMAT
        std::string MCparticlestree_path = "triggerAna/mcparticles";
        #else
        std::string MCparticlestree_path = "triggerAnaDumpTPs/mcparticles";
        #endif

        TTree *MCparticlestree = dynamic_cast<TTree*>(file->Get(MCparticlestree_path.c_str()));
        if (!MCparticlestree) {
            LogError << "Tree not found: " << MCparticlestree_path << std::endl;
            continue;
        }

        #ifdef STANDARD_FORMAT
        std::string MCtruthtree_path = "triggerAna/mctruths"; 
        #else
        std::string MCtruthtree_path = "triggerAnaDumpTPs/mctruths"; 
        #endif
        TTree *MCtruthtree = dynamic_cast<TTree*>(file->Get(MCtruthtree_path.c_str()));
        if (!MCtruthtree) {
            LogError << " Tree not found: " << MCtruthtree_path << std::endl;
            continue;
        }

        #ifdef STANDARD_FORMAT
        std::string simidestree_path = "triggerAna/simides";
        #else
        std::string simidestree_path = "triggerAnaDumpTPs/simides";
        #endif
        TTree *simidestree = dynamic_cast<TTree*>(file->Get(simidestree_path.c_str()));
        if (!simidestree) {
            LogError << " Tree not found: " << simidestree_path << std::endl;
            continue;
        }

        LogInfo << " Creating file maps for: " << filename << " with COUNT:" << COUNT <<std::endl;

        std::map<const ULong_t, UInt_t>  tp_map_lo; 
        std::map<const ULong_t, UInt_t>  tp_map_hi;
        std::map<const ULong_t, UInt_t>  mc_map_lo; 
        std::map<const ULong_t, UInt_t>  mc_map_hi;
        std::map<const ULong_t, UInt_t>  tr_map_lo;
        std::map<const ULong_t, UInt_t>  tr_map_hi;
        std::map<const ULong_t, UInt_t>  sm_map_lo;
        std::map<const ULong_t, UInt_t>  sm_map_hi;

        int tp_count = generate_filemap(TPtree, tp_map_lo, tp_map_hi);
        int mc_count = generate_filemap(MCparticlestree, mc_map_lo, mc_map_hi, COUNT);
        int truth_count = generate_filemap(MCtruthtree, tr_map_lo, tr_map_hi, COUNT);
        int simide_count = generate_filemap(simidestree, sm_map_lo, sm_map_hi, COUNT);

        LogInfo << " File maps generated: " << std::endl;
        LogInfo << "  TP tree: " << tp_count << " entries" << std::endl;
        LogInfo << "  MC particles tree: " << mc_count << " entries" << std::endl;
        LogInfo << "  MC truth tree: " << truth_count << " entries" << std::endl;
        LogInfo << "  SimIDE tree: " << simide_count << " entries" << std::endl;
    //----

// Collect unique event numbers in order of first occurrence (handles non-consecutive events
        // from hash-based file splitting).
        std::vector<ULong_t> event_numbers;
        UInt_t ev = 0;
        UInt_t run = 0;
        MCtruthtree->SetBranchAddress("event", &ev);
        MCtruthtree->SetBranchAddress("run", &run);
        std::set<ULong_t> seen;
        ULong_t an_event = -1;
        ULong_t evcount = -1;
        maxcount = COUNT;

        for (ULong_t i = 0; i < MCtruthtree->GetEntries(); ++i) {
            MCtruthtree->GetEntry(i);
            ULong_t event_key = 10000000 * run + ev;

            if (event_key != an_event){
                
                an_event = event_key;
                if (seen.insert(event_key).second){ 
                    event_numbers.push_back(event_key);
                    evcount++;
                    //LogWarning << "New event added to tree: " << event_key << " " << an_event << " " << evcount << std::endl;
                }
                else{
                     LogWarning << "Duplicate event found in tree: " << event_key << " " << an_event << " " << evcount << std::endl;
                }
                
                if (evcount < skipevents && skipevents > -1){
                //if (debugMode)                                                                                       
                    LogInfo << "Skipping event " << event_key << " " << an_event << " " << evcount << std::endl;
                    continue;
                }                                    
                LogDebug << "Adding event " << event_key << " " << evcount << std::endl;
            }
            if ( evcount >= maxcount+skipevents-1 && maxcount > 0) {
                LogInfo << "reached " << maxcount << " events, breaking loop after maxcount iterations." << std::endl;
                break;
            }
        }
        int n_events = static_cast<int>(event_numbers.size());
        if (verboseMode) LogInfo << " Found " << n_events << " unique events in tree: " << MCtruthtree_path << std::endl;

        tps.clear(); true_particles.clear(); neutrinos.clear();
        tps.resize(n_events); true_particles.resize(n_events); neutrinos.resize(n_events);
        int iRun = -1;
        // loop over the actual event numbers (not a consecutive range from first_event)
        for (int event_index = 0; event_index < (int)event_numbers.size(); ++event_index) {
            UInt_t iEvent = (UInt_t)(event_numbers[event_index]%10000000);
            UInt_t iRun = (UInt_t)(event_numbers[event_index] / 10000000);
            if (verboseMode) LogInfo << "Reading event " << iEvent << " run " << iRun << std::endl;
            //if (debugMode) LogDebug << "Beginning read_tpstream for event " << iEvent << std::endl;

            read_tpstream_simple(
                this_interaction,
                TPtree,
                MCparticlestree,
                MCtruthtree,
                simidestree,
                tps.at(event_index),
                true_particles.at(event_index),
                neutrinos.at(event_index),
                /*supernova_option*/0,
                iEvent,
                iRun,
                tp_map_lo, tp_map_hi,
                mc_map_lo, mc_map_hi,
                tr_map_lo, tr_map_hi,
                sm_map_lo, sm_map_hi,
                static_cast<double>(effective_time_window),
                channel_tolerance
            );

            // Summarise direct TP-to-truth associations built inside read_tpstream
            int matched_tps_counter = 0;
            for (const auto& tp : tps.at(event_index)) {
                if (tp.GetTrueParticle() != nullptr) { matched_tps_counter++; }
            }
            if (verboseMode) LogInfo << "Matched " << matched_tps_counter << "/" << tps.at(event_index).size()
                << " TPs to true particles via SimIDE association." << std::endl;

            if (debugMode) {
                LogDebug << "Event " << iEvent << " processing complete with "
                         << tps.at(event_index).size() << " TPs and "
                         << true_particles.at(event_index).size() << " true particles" << std::endl;
            }
        }

        // write *_tps_bktr<N>.root where N is backtracker_error_margin
        if (verboseMode) LogInfo << "Writing output to: " << out_abs << std::endl;
        write_tps_simple(out_abs, tps, true_particles, neutrinos);
        output_files.push_back(out_abs);
    }

    LogInfo << "\nList of output files (" << output_files.size() << "):" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(10, output_files.size()); ++i) {
        LogInfo << " - " << output_files[i] << std::endl;
    }
    if (output_files.size() > 10) {
        LogInfo << " ... (" << output_files.size() - 10 << " more files not shown)" << std::endl;
    }

    return 0;
}
