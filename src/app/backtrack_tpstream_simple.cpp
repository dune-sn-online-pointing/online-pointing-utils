#include "BacktrackingSimple.h"
#include "generate_filemap.h"
#include "write_tps_class.h"
#include <memory>

LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

int LIMITCOUNT=0;
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

    
    // Determine backtracker_error_margin value: CLI > JSON > default from parameters/timing.h
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

    std::vector<std::vector<TriggerPrimitive>> tps;
    std::vector<std::vector<TrueParticle>> true_particles;
    std::vector<std::vector<Neutrino>> neutrinos;

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
    TpsWriter(tps_writer);

    for (auto& filename : filenames) {
        std::unique_ptr<TFile> file(TFile::Open(filename.c_str(), "READ"));
        if (!file || file->IsZombie()) { LogError << "Failed to open file: " << filename << std::endl; continue; }
             
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
        if (bktr_margin != standard_backtracker_error_margin) {
            suffix << skipper << "_tps_bktr" << bktr_margin << ".root";
        } else {
            suffix << skipper << "_tps.root";
        }
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
        if (verboseMode) LogInfo << " open an output file " << out_abs << std::endl;
        tps_writer.Create(out_abs, "tps");

        if (verboseMode) LogInfo << "Reading file: " << filename << std::endl;
        

        std::string TPtree_path = "triggerAna/TriggerPrimitives/tpmakerTPCsimpleThr__TPGen"; // TODO make flexible for 1x2x6 and maybe else    
        TTree *TPtree = dynamic_cast<TTree*>(file->Get(TPtree_path.c_str()));
        if (!TPtree) {
            LogError << " Tree not found: " << TPtree_path << std::endl;
            continue; // can still go to next file
        }
 
        std::string MCparticlestree_path = "triggerAna/mcparticles";
        TTree *MCparticlestree = dynamic_cast<TTree*>(file->Get(MCparticlestree_path.c_str()));
        if (!MCparticlestree) {
            LogError << "Tree not found: " << MCparticlestree_path << std::endl;
            continue;
        }

        std::string MCtruthtree_path = "triggerAna/mctruths"; 
        TTree *MCtruthtree = dynamic_cast<TTree*>(file->Get(MCtruthtree_path.c_str()));
        if (!MCtruthtree) {
            LogError << " Tree not found: " << MCtruthtree_path << std::endl;
            continue;
        }

        std::string simidestree_path = "triggerAna/simides";
        TTree *simidestree = dynamic_cast<TTree*>(file->Get(simidestree_path.c_str()));
        if (!simidestree) {
            LogError << " Tree not found: " << simidestree_path << std::endl;
            continue;
        }

        std::map<const ULong_t, UInt_t>  tp_map_lo; 
        std::map<const ULong_t, UInt_t>  tp_map_hi;
        std::map<const ULong_t, UInt_t>  mc_map_lo; 
        std::map<const ULong_t, UInt_t>  mc_map_hi;
        std::map<const ULong_t, UInt_t>  tr_map_lo;
        std::map<const ULong_t, UInt_t>  tr_map_hi;
        std::map<const ULong_t, UInt_t>  sm_map_lo;
        std::map<const ULong_t, UInt_t>  sm_map_hi;
        
        int tp_count = generate_filemap(TPtree, tp_map_lo, tp_map_hi, maxcount);
        int mc_count = generate_filemap(MCparticlestree, mc_map_lo, mc_map_hi, maxcount);
        int truth_count = generate_filemap(MCtruthtree, tr_map_lo, tr_map_hi, maxcount);
        int simide_count = generate_filemap(simidestree, sm_map_lo, sm_map_hi, maxcount);

        LogInfo << " File maps generated: " << std::endl;
        LogInfo << "  TP tree: " << tp_count << " entries" << std::endl;
        LogInfo << "  MC particles tree: " << mc_count << " entries" << std::endl;
        LogInfo << "  MC truth tree: " << truth_count << " entries" << std::endl;
        LogInfo << "  SimIDE tree: " << simide_count << " entries" << std::endl;

        // // count events
        // // using this tree just because it's the smallest
        // #ifdef STANDARD_FORMAT
        // std::string MCtree_path = "triggerAna/mctruths";
        // #else
        // std::string MCtree_path = "triggerAnaDumpTPs/mctruths";
        // #endif
        // std::unique_ptr<TFile> file(TFile::Open(filename.c_str(), "READ"));
        // if (!file || file->IsZombie()) { LogError << "Failed to open file: " << filename << std::endl; continue; }
        // TTree *MCtree = dynamic_cast<TTree*>(file->Get(MCtree_path.c_str()));
        // if (!MCtree) { LogError << "Tree not found: " << MCtree_path << std::endl; continue; }

        // // Collect unique event numbers in order of first occurrence (handles non-consecutive events
        // // from hash-based file splitting).
        // std::vector<UInt_t> event_numbers;
        // UInt_t ev = 0;
        // MCtree->SetBranchAddress("event", &ev);
        // std::set<UInt_t> seen;
        // int an_event = -1;
        // int evcount = -1;
        // for (Long64_t i = 0; i < MCtree->GetEntries(); ++i) {
        //     MCtree->GetEntry(i);
        //     UInt_t event_index = ev;

        //     if (event_index != an_event){
                
        //         an_event = event_index;
        //         if (seen.insert(ev).second) event_numbers.push_back(ev);
        //         evcount++;
        //         if (evcount < skipevents && skipevents > -1){
        //         //if (debugMode)                                                                                       
        //             LogInfo << "Skipping event " << event_index << " " << an_event << " " << evcount << std::endl;
        //             continue;
        //         }

                                    
        //         LogDebug << "Adding event " << event_index << " " << evcount << std::endl;
        //     }
        //     if ( evcount >= maxcount+skipevents-1 && maxcount > 0) {
        //         LogInfo << "reached " << maxcount << " events, breaking loop after maxcount iterations." << std::endl;
        //         break;
        //     }
        // }
        int n_events = static_cast<int>(tp_map_lo.size());
        if (verboseMode) LogInfo << " Found " << n_events << " unique events in tree: " << TPtree_path << std::endl;
        
        tps.clear(); true_particles.clear(); neutrinos.clear();
        tps.resize(n_events); true_particles.resize(n_events); neutrinos.resize(n_events);

        // loop over the actual event numbers (not a consecutive range from first_event)
        int count = 0;
        for (auto run_event : tp_map_lo) {
            ULong_t event_key = run_event.first;
            UInt_t run = run_from_event_key(event_key);
            UInt_t event = event_from_event_key(event_key);
            UInt_t tp_lo = tp_map_lo[event_key];
            UInt_t tp_hi = tp_map_hi[event_key];
            UInt_t mc_lo = mc_map_lo[event_key];
            UInt_t mc_hi = mc_map_hi[event_key];
            UInt_t tr_lo = tr_map_lo[event_key];
            UInt_t tr_hi = tr_map_hi[event_key];
            UInt_t sm_lo = sm_map_lo[event_key];
            UInt_t sm_hi = sm_map_hi[event_key];
            if (verboseMode) LogInfo << "Reading event " << event << std::endl;
            if (debugMode) LogDebug << "Beginning read_tpstream for event " << event << std::endl;
            std::vector<TriggerPrimitive> tps_by_event; 
            std::vector<TrueParticle> true_particles_by_event;
            std::vector<Neutrino> neutrinos_by_event;

            read_tpstream(
                filename,
                tps_by_event,
                true_particles_by_event,
                neutrinos_by_event,
                0,                //*supernova_option*/
                event, 
                run,
                tp_lo,
                tp_hi,
                mc_lo,
                mc_hi,
                tr_lo,
                tr_hi,
                sm_lo,
                sm_hi, 
                static_cast<double>(effective_time_window),
                channel_tolerance
            );
            

            // Summarise direct TP-to-truth associations built inside read_tpstream
            int matched_tps_counter = 0;
            for (const auto& tp : tps_by_event) {
                if (tp.GetTrueParticle() != nullptr) { matched_tps_counter++; }
            }
            if (verboseMode) LogInfo << "Matched " << matched_tps_counter << "/" << tps_by_event.size()
                << " TPs to true particles via SimIDE association." << std::endl;

            if (debugMode) {
                LogDebug << "Event " << event_key << " processing complete with "
                         << tps_by_event.size() << " TPs " << std::endl;
            }
            tps.push_back(tps_by_event);
            tps_writer.WriteSingleEvent(tps_by_event);
            true_particles.push_back(true_particles_by_event);
            neutrinos.push_back(neutrinos_by_event);
            if (verboseMode && count == 1){
                LogInfo << " dump second event " << event << " " << run << std::endl;
                for (auto tp:tps_by_event){
                    tp.Print();
                }
            }
            tps_by_event.clear();
            true_particles_by_event.clear();
            neutrinos_by_event.clear();
        }

        // write *_tps_bktr<N>.root where N is backtracker_error_margin
        if (verboseMode) LogInfo << "Writing output to: " << out_abs << std::endl;
        //write_tps(out_abs, tps, true_particles, neutrinos);
        tps_writer.Close();
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
