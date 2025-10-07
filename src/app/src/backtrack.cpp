#include "global.h"

#include "cluster_to_root_libs.h"
#include "Cluster.h"


LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

int main(int argc, char* argv[]) {
    CmdLineParser clp;
    clp.getDescription() << "> backtrack app - extract TPs and attach truth, writing *_tps_bktr<N>.root files (N: backtracker_error_margin)."<< std::endl;
    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");
    clp.addOption("outFolder", {"--output-folder"}, "Output folder path (default: data)");
    clp.addOption("inputFile", {"-i", "--input-file"}, "Input file with list OR single ROOT file path (overrides JSON inputs)");
    clp.addOption("bktrMargin", {"--bktr-margin"}, "Override backtracker_error_margin (int)");
    clp.addDummyOption("Triggers");
    clp.addTriggerOption("verboseMode", {"-v", "--verbose"}, "Run in verbose mode");
    clp.addTriggerOption("debugMode", {"-d", "--debug"}, "Run in debug mode (more detailed than verbose)");
    clp.addDummyOption();
    LogInfo << clp.getDescription().str() << std::endl;
    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;
    clp.parseCmdLine(argc, argv);
    LogThrowIf( clp.isNoOptionTriggered(), "No option was provided." );

    // Set logging verbosity based on command line options
    if (clp.isOptionTriggered("debugMode")) {
        debugMode = true;
    }
    else if (clp.isOptionTriggered("verboseMode")) {
        verboseMode = true;
    }

    std::string json = clp.getOptionVal<std::string>("json");
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
    filenames.reserve(64);

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

    // Priority 2: JSON inputFile (single root)
    if (filenames.empty()) {
        try {
            if (j.contains("inputFile")) {
                auto single = j.at("inputFile").get<std::string>();
                if (!single.empty()) {
                    LogInfo << "JSON inputFile: " << single << std::endl;
                    LogThrowIf(!file_exists(single), "inputFile does not exist: " << single);
                    LogThrowIf(!is_tpstream(single),  "inputFile not a *_tpstream.root: " << single);
                    filenames.push_back(single);
                }
            }
        } catch (...) { /* ignore */ }
    }

    // Priority 3: JSON inputFolder (gather *_tpstream.root)
    if (filenames.empty()) {
        try {
            if (j.contains("inputFolder")) {
                auto folder = j.at("inputFolder").get<std::string>();
                if (!folder.empty()) {
                    LogInfo << "JSON inputFolder: " << folder << std::endl;
                    std::error_code ec;
                    for (auto const& entry : std::filesystem::directory_iterator(folder, ec)) {
                        if (ec) break;
                        if (!entry.is_regular_file()) continue;
                        std::string p = entry.path().string();
                        if (!is_tpstream(p)) continue;
                        if (!file_exists(p)) continue;
                        filenames.push_back(p);
                    }
                    std::sort(filenames.begin(), filenames.end());
                }
            }
        } catch (...) { /* ignore */ }
    }

    // Priority 4: JSON inputList (array of files)
    if (filenames.empty()) {
        try {
            if (j.contains("inputList") && j.at("inputList").is_array()) {
                LogInfo << "JSON inputList provided." << std::endl;
                for (const auto& el : j.at("inputList")) {
                    if (!el.is_string()) continue;
                    std::string p = el.get<std::string>();
                    if (p.empty()) continue;
                    if (!file_exists(p)) { LogWarning << "Skipping (missing): " << p << std::endl; continue; }
                    if (!is_tpstream(p)) { LogWarning << "Skipping (not *_tpstream.root): " << p << std::endl; continue; }
                    filenames.push_back(p);
                }
            }
        } catch (...) { /* ignore */ }
    }

    // Priority 5: JSON inputListFile (file with list)
    if (filenames.empty()) {
        try {
            if (j.contains("inputListFile")) {
                auto list_file = j.at("inputListFile").get<std::string>();
                if (!list_file.empty()) {
                    LogInfo << "JSON inputListFile: " << list_file << std::endl;
                    std::ifstream infile(list_file);
                    std::string line;
                    while (std::getline(infile, line)) {
                        if (line.size() >= 3 && line.substr(0,3) == "###") break;
                        if (line.empty() || line[0] == '#') continue;
                        if (!file_exists(line)) { LogWarning << "Skipping (missing): " << line << std::endl; continue; }
                        if (!is_tpstream(line)) { LogWarning << "Skipping (not *_tpstream.root): " << line << std::endl; continue; }
                        filenames.push_back(line);
                    }
                }
            }
        } catch (...) { /* ignore */ }
    }

    LogInfo << "Number of valid files: " << filenames.size() << std::endl;
    LogThrowIf(filenames.empty(), "No valid input files.");

    // Output folder: CLI outFolder > JSON outputFolder > "data"
    std::string outfolder;
    if (clp.isOptionTriggered("outFolder")) outfolder = clp.getOptionVal<std::string>("outFolder");
    else if (j.contains("outputFolder")) { try { outfolder = j.at("outputFolder").get<std::string>(); } catch (...) { /* ignore */ }}
    if (outfolder.empty()) outfolder = std::string("data");
    LogInfo << "Output folder: " << outfolder << std::endl;

    std::vector<std::string> produced;

    std::vector<std::vector<TriggerPrimitive>> tps;
    std::vector<std::vector<TrueParticle>> true_particles;
    std::vector<std::vector<Neutrino>> neutrinos;

    // Effective time window for TP<->truth association in TDC ticks (base 1 TPC sample + margin in TPC samples)
    int effective_time_window = (1 + bktr_margin) * conversion_tdc_to_tpc;
    LogInfo << "Effective time window (TDC ticks): " << effective_time_window << " (conversion_tdc_to_tpc=" << conversion_tdc_to_tpc << ")" << std::endl;

    int channel_tolerance = 50; // default fallback
    if (j.contains("backtracker_channel_tolerance")) {
        try { channel_tolerance = j.at("backtracker_channel_tolerance").get<int>(); }
        catch (...) { LogWarning << "Invalid backtracker_channel_tolerance in JSON, keeping default (50)." << std::endl; }
    }
    LogInfo << "Channel tolerance (channels): " << channel_tolerance << std::endl;

    for (auto& filename : filenames) {
        LogInfo << "Reading file: " << filename << std::endl;
        // count events
        // std::string TPtree_path = "triggerAnaDumpTPs/TriggerPrimitives/tpmakerTPC__TriggerAnaTree1x2x2";
        std::string MCtree_path = "triggerAnaDumpTPs/mcneutrinos";
        TFile *file = TFile::Open(filename.c_str());
        if (!file || file->IsZombie()) { LogError << "Failed to open file: " << filename << std::endl; continue; }
        // TTree *TPtree = dynamic_cast<TTree*>(file->Get(TPtree_path.c_str()));
        // if (!TPtree) { LogError << "Tree not found: " << TPtree_path << std::endl; file->Close(); delete file; continue; }
        TTree *MCtree = dynamic_cast<TTree*>(file->Get(MCtree_path.c_str()));
        if (!MCtree) { LogError << "Tree not found: " << MCtree_path << std::endl; file->Close(); delete file; continue; }
        UInt_t this_event_number = 0; 
        MCtree->SetBranchAddress("Event", &this_event_number);
        int n_events = MCtree->GetEntries(); 

        MCtree->GetEntry(0);
        int first_event = this_event_number;

        LogInfo << "Number of events in file: " << n_events << std::endl;
        file->Close(); delete file; file = nullptr;

        tps.clear(); true_particles.clear(); neutrinos.clear();
        tps.resize(n_events); true_particles.resize(n_events); neutrinos.resize(n_events);

        // loop over events
        for (int iEvent = first_event; iEvent < first_event + n_events; ++iEvent) {
            int event_index = iEvent - first_event;
            if (verboseMode) LogInfo << "Reading event " << iEvent << std::endl;
            if (debugMode) LogDebug << "Beginning file_reader for event " << iEvent << std::endl;
            
            file_reader(
                filename,
                tps.at(event_index),
                true_particles.at(event_index),
                neutrinos.at(iEvent - first_event),
                /*supernova_option*/0,
                iEvent,
                static_cast<double>(effective_time_window),
                channel_tolerance
            );

            // Summarise direct TP-to-truth associations built inside file_reader
            int matched_tps_counter = 0;
            for (const auto& tp : tps.at(event_index)) {
                if (tp.GetTrueParticle() != nullptr) { matched_tps_counter++; }
            }
            LogInfo << "Matched " << matched_tps_counter << "/" << tps.at(event_index).size()
                    << " TPs to true particles via SimIDE association." << std::endl;
                    
            if (debugMode) {
                LogDebug << "Event " << iEvent << " processing complete with " 
                         << tps.at(event_index).size() << " TPs and " 
                         << true_particles.at(event_index).size() << " true particles" << std::endl;
            }
        }

        // write *_tps_bktr<N>.root where N is backtracker_error_margin
        std::string input_basename = filename.substr(filename.find_last_of("/\\") + 1);
        input_basename = input_basename.substr(0, input_basename.length() - 14); // remove _tpstream.root
        std::ostringstream suffix;
        suffix << "_tps_bktr" << bktr_margin << ".root";
        std::string out = outfolder + "/" + input_basename + suffix.str();
        // Use absolute path for output
        std::error_code _ec_abs;
        std::filesystem::path out_abs_p = std::filesystem::absolute(std::filesystem::path(out), _ec_abs);
        std::string out_abs = _ec_abs ? out : out_abs_p.string();
        
        if (verboseMode) LogInfo << "Writing output to: " << out_abs << std::endl;
        write_tps_to_root(out_abs, tps, true_particles, neutrinos);
        produced.push_back(out_abs);
    }

    // also write a filelist for convenience
    try {
        std::ostringstream list_name;
        list_name << outfolder << "/test_files_tps_bktr" << bktr_margin << ".txt";
        std::string list_out = list_name.str();
        std::error_code _ec_list_abs;
        std::filesystem::path list_abs_p = std::filesystem::absolute(std::filesystem::path(list_out), _ec_list_abs);
        std::string list_out_abs = _ec_list_abs ? list_out : list_abs_p.string();
        
        if (verboseMode) LogInfo << "Writing file list to: " << list_out_abs << std::endl;
        std::ofstream of(list_out_abs);
        for (const auto& p : produced) of << p << "\n";
        of << "\n### This is a break point\n";
        of.close();
        LogInfo << "Wrote list of TPs files: " << list_out_abs << std::endl;
    } catch (...) { LogWarning << "Failed to write TPs file list." << std::endl; }

    LogInfo << "\nList of produced files (" << produced.size() << "):" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(10, produced.size()); ++i) {
        LogInfo << " - " << produced[i] << std::endl;
    }
    if (produced.size() > 10) {
        LogInfo << " ... (" << produced.size() - 10 << " more files not shown)" << std::endl;
    }

    return 0;
}
