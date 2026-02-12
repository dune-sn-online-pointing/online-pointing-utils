#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <map>
#include <sstream>
#include <climits>
#include <algorithm>
#include <set>
#include <nlohmann/json.hpp>
#include <filesystem>

#include "CmdLineParser.h"
#include "Logger.h"
#include "GenericToolbox.Utils.h"

#include "cluster_to_root_libs.h"
#include "cluster.h"
#include "functions.h"


LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

int main(int argc, char* argv[]) {
    CmdLineParser clp;
    clp.getDescription() << "> backtrack app - extract TPs and attach truth, writing *_tps_bktr<N>.root files (N: backtracker_error_margin)."<< std::endl;
    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");
    clp.addOption("outFolder", {"--output-folder"}, "Output folder path (default: data)");
    clp.addOption("bktrMargin", {"--bktr-margin"}, "Override backtracker_error_margin (int)");
    clp.addDummyOption("Triggers");
    clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");
    clp.addDummyOption();
    LogInfo << clp.getDescription().str() << std::endl;
    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;
    clp.parseCmdLine(argc, argv);
    LogThrowIf( clp.isNoOptionTriggered(), "No option was provided." );

    std::string json = clp.getOptionVal<std::string>("json");
    std::ifstream i(json); nlohmann::json j; i >> j;
    
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
    std::string list_file = j["filename"]; // text file with tpstream list
    LogInfo << "File with list of tpstreams: " << list_file << std::endl;

    int max_events_per_filename = j.value("max_events_per_filename", -1);
    if (max_events_per_filename > 0) {
        LogInfo << "Max events per filename: " << max_events_per_filename << std::endl;
    }

    std::string outfolder = clp.isOptionTriggered("outFolder") ? clp.getOptionVal<std::string>("outFolder") : std::string("data");
    LogInfo << "Output folder: " << outfolder << std::endl;

    std::ifstream infile(list_file);
    std::string line;
    std::vector<std::string> filenames;
    while (std::getline(infile, line)) {
        if (line.size() >= 3 && line.substr(0,3) == "###") break;
        if (line.empty() || line[0] == '#') continue;
        std::string basename = line.substr(line.find_last_of("/\\") + 1);
        if (basename.length() < 14 || basename.substr(basename.length()-14) != "_tpstream.root") { LogInfo << "Skipping (not *_tpstream.root): " << basename << std::endl; continue; }
        std::ifstream fchk(line); if (!fchk.good()) { LogInfo << "Skipping (missing): " << line << std::endl; continue; }
        filenames.push_back(line);
    }
    LogInfo << "Number of valid files: " << filenames.size() << std::endl;
    LogThrowIf(filenames.empty(), "No valid input files.");

    std::vector<std::string> produced;

    std::vector<std::vector<TriggerPrimitive>> tps;
    std::vector<std::vector<TrueParticle>> true_particles;
    std::vector<std::vector<Neutrino>> neutrinos;

    // Effective time window for TP<->truth association in TDC ticks (base 1 TPC sample + margin in TPC samples)
    int effective_time_window = (1 + bktr_margin) * conversion_tdc_to_tpc;
    LogInfo << "Effective time window (TDC ticks): " << effective_time_window << " (conversion_tdc_to_tpc=" << conversion_tdc_to_tpc << ")" << std::endl;

    for (auto& filename : filenames) {
        LogInfo << "Reading file: " << filename << std::endl;
        // count events
        std::string TPtree_path = "triggerAnaDumpTPs/TriggerPrimitives/tpmakerTPC__TriggerAnaTree1x2x2";
        TFile *file = TFile::Open(filename.c_str());
        if (!file || file->IsZombie()) { LogError << "Failed to open file: " << filename << std::endl; continue; }
        TTree *TPtree = dynamic_cast<TTree*>(file->Get(TPtree_path.c_str()));
        if (!TPtree) { LogError << "Tree not found: " << TPtree_path << std::endl; file->Close(); delete file; continue; }
        UInt_t this_event_number = 0; TPtree->SetBranchAddress("Event", &this_event_number);
        int n_events = 0; for (Long64_t i = 0; i < TPtree->GetEntries(); ++i) { TPtree->GetEntry(i); if ((int)this_event_number > n_events) n_events = this_event_number; }
        file->Close(); delete file; file = nullptr;

        tps.clear(); true_particles.clear(); neutrinos.clear();
        tps.resize(n_events+1); true_particles.resize(n_events+1); neutrinos.resize(n_events+1);

        int last_event = n_events;
        if (max_events_per_filename > 0 && max_events_per_filename < last_event) {
            last_event = max_events_per_filename;
        }

        for (int iEvent = 1; iEvent <= last_event; ++iEvent) {
            LogInfo << "Reading event " << iEvent << std::endl;
            file_reader(filename, tps.at(iEvent), true_particles.at(iEvent), neutrinos.at(iEvent), /*supernova_option*/0, iEvent);

            // connect TPs to true particles
        for (int iTP = 0; iTP < (int)tps.at(iEvent).size(); ++iTP) {
                for (int iTruePart = 0; iTruePart < (int)true_particles.at(iEvent).size(); ++iTruePart) {
                    if (true_particles.at(iEvent).at(iTruePart).GetChannels().size() == 0) continue;
            // Use the effective_time_window derived from backtracker_error_margin
            if (isTimeCompatible(&true_particles.at(iEvent).at(iTruePart), &tps.at(iEvent).at(iTP), effective_time_window)
                        && isChannelCompatible(&true_particles.at(iEvent).at(iTruePart), &tps.at(iEvent).at(iTP)))
                    { tps.at(iEvent).at(iTP).SetTrueParticle(&true_particles.at(iEvent).at(iTruePart)); break; }
                }
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
    std::ofstream of(list_out_abs);
        for (const auto& p : produced) of << p << "\n";
        of << "\n### This is a break point\n";
        of.close();
    LogInfo << "Wrote list of TPs files: " << list_out_abs << std::endl;
    } catch (...) { LogWarning << "Failed to write TPs file list." << std::endl; }

    LogInfo << "\nSummary of produced files (" << produced.size() << "):" << std::endl;
    for (const auto& p : produced) LogInfo << " - " << p << std::endl;

    return 0;
}
