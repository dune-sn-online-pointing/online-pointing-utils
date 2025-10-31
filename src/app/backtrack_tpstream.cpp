#include "Backtracking.h"

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

    // If no CLI, use utility function for JSON-based file finding
    if (filenames.empty()) filenames = find_input_files(j, "tpstream");


    LogInfo << "Number of valid files: " << filenames.size() << std::endl;
    LogThrowIf(filenames.empty(), "No valid input files.");

    // Check if we should limit the number of files to process
    // Priority: CLI --max-files > JSON max_files > unlimited
    int max_files = j.value("max_files", -1);
    if (clp.isOptionTriggered("maxFiles")) {
        max_files = clp.getOptionVal<int>("maxFiles");
        LogInfo << "Max files (from CLI): " << max_files << std::endl;
    } else if (max_files > 0) {
        if (max_files > filenames.size()) max_files = filenames.size();
        LogInfo << "Max files (from JSON): " << max_files << std::endl;
    } else {
        LogInfo << "Max files: unlimited" << std::endl;
        max_files = filenames.size();
    }

    // Priority: CLI --skip-files > JSON skip_files > 0
    int skip_files = j.value("skip_files", 0);
    if (clp.isOptionTriggered("skipFiles")) {
        skip_files = clp.getOptionVal<int>("skipFiles");
        LogInfo << "Number of files to skip at start (from CLI): " << skip_files << std::endl;
    } else {
        LogInfo << "Number of files to skip at start (from JSON): " << skip_files << std::endl;
    }

    // Output folder: CLI outFolder > JSON sig_folder > JSON outputFolder > tpstream_folder
    std::string outfolder;
    if (clp.isOptionTriggered("outFolder")) outfolder = clp.getOptionVal<std::string>("outFolder");
    else if (j.contains("sig_folder")) { try { outfolder = j.at("sig_folder").get<std::string>(); } catch (...) { /* ignore */ }}
    else if (j.contains("outputFolder")) { try { outfolder = j.at("outputFolder").get<std::string>(); } catch (...) { /* ignore */ }}
    if (outfolder.empty()) {
        // Auto-generate from tpstream_folder
        outfolder = j.value("tpstream_folder", std::string("."));
        // Remove trailing slash if present
        if (!outfolder.empty() && outfolder.back() == '/') {
            outfolder.pop_back();
        }
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

    int count_files = 0, done_files = 0;

    for (auto& filename : filenames) {

        if (count_files < skip_files) {
            count_files++;
            LogInfo << "Skipping file " << count_files << ": " << filename << std::endl;
            continue;
        }

        done_files++;
        if (done_files > max_files) {
            LogInfo << "Reached max_files limit (" << max_files << "), stopping." << std::endl;
            break;
        }

        GenericToolbox::displayProgressBar(done_files, max_files, "Processing files...");

        // Compute expected output path early to allow skip-if-exists behavior
        std::string input_basename = filename.substr(filename.find_last_of("/\\") + 1);
        input_basename = input_basename.substr(0, input_basename.length() - 14); // remove _tpstream.root
        std::ostringstream suffix;
        if (bktr_margin != standard_backtracker_error_margin) {
            suffix << "_tps_bktr" << bktr_margin << ".root";
        } else {
            suffix << "_tps.root";
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

        if (verboseMode) LogInfo << "Reading file: " << filename << std::endl;
        // count events
        // using this tree just because it's the smallest
        std::string MCtree_path = "triggerAnaDumpTPs/mctruths";
        TFile *file = TFile::Open(filename.c_str());
        if (!file || file->IsZombie()) { LogError << "Failed to open file: " << filename << std::endl; continue; }
        // TTree *TPtree = dynamic_cast<TTree*>(file->Get(TPtree_path.c_str()));
        // if (!TPtree) { LogError << "Tree not found: " << TPtree_path << std::endl; file->Close(); delete file; continue; }
        TTree *MCtree = dynamic_cast<TTree*>(file->Get(MCtree_path.c_str()));
        int n_events = 0;
        UInt_t this_event_number = 0;
        if (!MCtree) { LogError << "Tree not found: " << MCtree_path << std::endl; file->Close(); delete file; continue; }
        if (MCtree) {
            MCtree->SetBranchAddress("Event", &this_event_number);
            std::set<UInt_t> unique_events;
            for (Long64_t i = 0; i < MCtree->GetEntries(); ++i) {
                MCtree->GetEntry(i);
                unique_events.insert(this_event_number);
            }
            n_events = unique_events.size();
            if (verboseMode) LogInfo << " Found " << n_events << " unique events in tree: " << MCtree_path << std::endl;
        }
        
        MCtree->GetEntry(0);
        int first_event = this_event_number;

        if (verboseMode) LogInfo << "Number of events in file: " << n_events << std::endl;
        file->Close(); delete file; file = nullptr;

        tps.clear(); true_particles.clear(); neutrinos.clear();
        tps.resize(n_events); true_particles.resize(n_events); neutrinos.resize(n_events);

        // loop over events
        for (int iEvent = first_event; iEvent < first_event + n_events; ++iEvent) {
            int event_index = iEvent - first_event;
            if (verboseMode) LogInfo << "Reading event " << iEvent << std::endl;
            if (debugMode) LogDebug << "Beginning read_tpstream for event " << iEvent << std::endl;
            
            read_tpstream(
                filename,
                tps.at(event_index),
                true_particles.at(event_index),
                neutrinos.at(iEvent - first_event),
                /*supernova_option*/0,
                iEvent,
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
        write_tps(out_abs, tps, true_particles, neutrinos);
        output_files.push_back(out_abs);
    }

    // also write a filelist for convenience
    try {
        std::ostringstream list_name;
        if (bktr_margin != standard_backtracker_error_margin) {
            list_name << outfolder << "/test_files_bktr" << bktr_margin << "_tps.txt";
        } else {
            list_name << outfolder << "/test_files_tps.txt";
        }
        std::string list_out = list_name.str();
        std::error_code _ec_list_abs;
        std::filesystem::path list_abs_p = std::filesystem::absolute(std::filesystem::path(list_out), _ec_list_abs);
        std::string list_out_abs = _ec_list_abs ? list_out : list_abs_p.string();
        
        if (verboseMode) LogInfo << "Writing file list to: " << list_out_abs << std::endl;
        std::ofstream of(list_out_abs);
        for (const auto& p : output_files) of << p << "\n";
        of << "\n### This is a break point\n";
        of.close();
        if (verboseMode) LogInfo << "Wrote list of TPs files: " << list_out_abs << std::endl;
    } catch (...) { LogWarning << "Failed to write TPs file list." << std::endl; }

    LogInfo << "\nList of output files (" << output_files.size() << "):" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(10, output_files.size()); ++i) {
        LogInfo << " - " << output_files[i] << std::endl;
    }
    if (output_files.size() > 10) {
        LogInfo << " ... (" << output_files.size() - 10 << " more files not shown)" << std::endl;
    }

    return 0;
}
