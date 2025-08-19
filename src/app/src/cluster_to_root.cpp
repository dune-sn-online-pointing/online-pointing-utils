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

#include "CmdLineParser.h"
#include "Logger.h"
#include "GenericToolbox.Utils.h"

// #include "position_calculator.h"
#include "cluster_to_root_libs.h"
#include "cluster.h"
#include "functions.h"


LoggerInit([]{  Logger::getUserHeader() << "[" << FILENAME << "]";});

int main(int argc, char* argv[]) {  
    CmdLineParser clp;

    clp.getDescription() << "> cluster_to_root app."<< std::endl;

    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");
    clp.addOption("outFolder", {"--output-folder"}, "Output folder path (optional, defaults to input file folder)");
    clp.addOption("outputSuffix", {"--output-suffix"}, "Output filename suffix");

    clp.addDummyOption("Triggers");
    clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");

    clp.addDummyOption();
    // usage always displayed
    LogInfo << clp.getDescription().str() << std::endl;

    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;

    clp.parseCmdLine(argc, argv);

    LogThrowIf( clp.isNoOptionTriggered(), "No option was provided." );

    LogInfo << "Provided arguments: " << std::endl;
    LogInfo << clp.getValueSummary() << std::endl << std::endl;

    std::string json = clp.getOptionVal<std::string>("json");
    // read the configuration file
    std::ifstream i(json);
    nlohmann::json j;
    i >> j;
    std::string list_file = j["filename"];
    LogInfo << "File with list of tpstreams: " << list_file << std::endl;
    
    // Determine output folder: command line option takes precedence, otherwise use input file folder
    std::string outfolder;
    if (clp.isOptionTriggered("outFolder")) {
        outfolder = clp.getOptionVal<std::string>("outFolder");
        LogInfo << "Output folder (from command line): " << outfolder << std::endl;
    } else {
        // Extract folder from list_file path and place outputs in a fixed 'clusters' subfolder
        std::string base_folder = list_file.substr(0, list_file.find_last_of("/\\"));
        if (base_folder.empty()) base_folder = "."; // current directory if no path
        outfolder = base_folder + "/clusters";
        LogInfo << "Output folder (default): " << outfolder << std::endl;
    }
    int ticks_limit = j["tick_limit"];
    LogInfo << "Tick limit: " << ticks_limit << std::endl;
    int channel_limit = j["channel_limit"];
    LogInfo << "Channel limit: " << channel_limit << std::endl;
    int min_tps_to_cluster = j["min_tps_to_cluster"];
    LogInfo << "Min TPs to cluster: " << min_tps_to_cluster << std::endl;
    int min_time_over_threshold = j["min_time_over_threshold"];
    LogInfo << "Min time over threshold: " << min_time_over_threshold << std::endl;
    int energy_cut = j["energy_cut_mev"];
    LogInfo << "Energy cut: " << energy_cut << std::endl;

    int supernova_option = 0; //j["supernova_option"]; TODO decide to keep or not
    // LogInfo << "Supernova option: " << supernova_option << std::endl;

    int max_events_per_filename = j["max_events_per_filename"];
    LogInfo << "Max events per filename: " << max_events_per_filename << std::endl;

    std::vector <int> adc_to_mev_conversion (APA::views.size());
    adc_to_mev_conversion.at(0) = j["adc_to_mev_induction"];
    adc_to_mev_conversion.at(1) = j["adc_to_mev_induction"];
    adc_to_mev_conversion.at(2) = j["adc_to_mev_collection"];
    LogInfo << "ADC to MeV conversion for induction: " << adc_to_mev_conversion.at(0) << std::endl;
    LogInfo << "ADC to MeV conversion for collection: " << adc_to_mev_conversion.at(2) << std::endl;

    std::vector <int> adc_integral_cut (APA::views.size());
    adc_integral_cut.at(0) = j["adc_integral_cut_induction"];
    adc_integral_cut.at(1) = j["adc_integral_cut_induction"];
    adc_integral_cut.at(2) = j["adc_integral_cut_collection"];
    LogInfo << "ADC integral cut for induction: " << adc_integral_cut.at(0) << std::endl;
    LogInfo << "ADC integral cut for collection: " << adc_integral_cut.at(2) << std::endl;
    // int use_electron_direction = j["use_electron_direction"];
    // LogInfo << "Use electron direction: " << use_electron_direction << std::endl;

    // start the clock
    std::clock_t start = std::clock();

    // filename is the name of the file containing the filenames to read
    std::vector<std::string> filenames;
    // read the file containing the filenames and save them in a vector
    std::ifstream infile(list_file);
    std::string line;
    LogInfo<<"Opening file: "<< list_file << std::endl;
    // read and save the TPs
    while (std::getline(infile, line)) {
        // check that the line is not a break point, starting with ###
        if (line.size() >= 3 && line.substr(0, 3) == "###") break;
        if (line.empty() || line[0] == '#')continue; // skip empty lines and comments
        // Extract filename from path and check if it starts with "tpstream"
        std::string basename = line.substr(line.find_last_of("/\\") + 1);
        if (basename.length() < 14 || basename.substr(basename.length() - 14) != "_tpstream.root") {
            LogInfo << "Skipping file (doesn't end with '_tpstream.root'): " << basename << std::endl;
            continue;
        }
        // Check if file exists before adding to filenames vector
        std::ifstream file_check(line);
        if (!file_check.good()) {
            LogInfo << "Skipping file (does not exist): " << line << std::endl;
            continue;
        }
        file_check.close();
        filenames.push_back(line);
    }
    
    LogInfo << "Number of valid files: " << filenames.size() << std::endl;
    
    // TODO: parallelize this
    
    // each entry of the vector is an event
    std::vector<std::vector<TriggerPrimitive>> tps;
    std::vector<std::vector<TrueParticle>> true_particles;
    std::vector<std::vector<Neutrino>> neutrinos;

    std::vector<std::string> output_files;

    for (auto& filename : filenames) {
        LogInfo << "Reading file: " << filename << std::endl;

        // find how many events there are, reading the Event branch of the TP branch,
        // it's a root file
        std::string TPtree_path = "triggerAnaDumpTPs/TriggerPrimitives/tpmakerTPC__TriggerAnaTree1x2x2"; // TODO make flexible for 1x2x6 and maybe else
        TFile *file = TFile::Open(filename.c_str());
        if (!file || file->IsZombie()) {
            LogError << "Failed to open file: " << filename << std::endl;
            continue;
        }
        TTree *TPtree = dynamic_cast<TTree*>(file->Get(TPtree_path.c_str()));
        if (!TPtree) {
            LogError << " Tree not found: " << TPtree_path << std::endl;
            continue; // can still go to next file
        }

        // read branch Event, knowing that the first value is 1, find the last number occurring
        // in the branch, this is the number of events
        int event = 0;
        UInt_t this_event_number = 0;
        TPtree->SetBranchAddress("Event", &this_event_number);
        int n_events = 0;
        for (Long64_t i = 0; i < TPtree->GetEntries(); ++i) {
            TPtree->GetEntry(i);
            if (this_event_number > n_events) {
                n_events = this_event_number;
            }
        }
        // close the file
        file->Close();

        LogInfo << "Number of events in file " << filename << ": " << n_events << std::endl;
        // read the tps from the file event by event, starting from 1
        // tps.reserve(n_events);
        // LOGIC: event 0 will still exist but empty, for easier indexing
        tps.resize(n_events+1);
        true_particles.resize(n_events+1);
        neutrinos.resize(n_events+1);

        LogInfo << "############################" << std::endl;

        // mind that events actually start from 1
        for (int iEvent = 1; iEvent < n_events+1; iEvent++) {        // read the tps from the file

            LogInfo << "Reading event " << iEvent << std::endl;
            
            file_reader(filename, tps.at(iEvent), true_particles.at(iEvent), neutrinos.at(iEvent), supernova_option, iEvent);
            // file_reader(filenames, tps, true_particles, neutrinos, supernova_option, max_events_per_filename);

            LogInfo << "Loaded TPs and true particles from file, for event " << iEvent << ". Number of TPs is now: " << tps.at(iEvent).size() << std::endl;

            // print generator of all true particles
            // for (int i = 0; i < true_particles.size(); i++) {
            //     std::cout << "True particle " << i << ": " << true_particles.at(i).GetGeneratorName();
            //     true_particles.at(i).Print();
            // }

            // return 0;

            // connect TPs to the true particles
            LogInfo << "Connecting TPs to true particles" << std::endl;
            LogInfo << tps.at(iEvent).size() << " TPs, " << true_particles.at(iEvent).size() << " true particles" << std::endl;
            
            int matched_tps_counter = 0;

            for (int iTP = 0; iTP < tps.at(iEvent).size(); iTP++) {
                
                // progress bar
                // if (iTP % 1000 == 0)
                //     GenericToolbox::displayProgressBar(iTP, tps.at(iEvent).size(), "Matching TPs and true particles for this event...");

                for (int iTruePart = 0; iTruePart < true_particles.at(iEvent).size(); iTruePart++) {
                    // print timestart, time end, channels in the true particle
                    // LogInfo << "True particle " << iTruePart << ": " << true_particles.at(iEvent).at(iTruePart).GetGeneratorName() << std::endl;
                    // LogInfo << "True particle start " << iTruePart << ": " << true_particles.at(iEvent).at(iTruePart).GetTimeStart() << " end: " << true_particles.at(iEvent).at(iTruePart).GetTimeEnd() << std::endl;
                    // LogInfo << "True particle " << iTruePart << ": " << true_particles.at(iEvent).at(iTruePart).GetChannels().size() << " channels" << std::endl;
                    // for (auto& channel : true_particles.at(iEvent).at(iTruePart).GetChannels()) {
                    //     LogInfo << "Channel: " << channel << std::endl;
                    // }

                    // if there are no channels, skip directly
                    if (true_particles.at(iEvent).at(iTruePart).GetChannels().size() == 0) {
                        // LogInfo << "True particle " << iTruePart << ": no channels, skipping" << std::endl;
                        continue;
                    }

                        
                    if (isTimeCompatible(&true_particles.at(iEvent).at(iTruePart), &tps.at(iEvent).at(iTP), time_window) 
                        && isChannelCompatible(&true_particles.at(iEvent).at(iTruePart), &tps.at(iEvent).at(iTP))) 
                    {   
                        // LogInfo << "TP connected to true particle, generator " << true_particles.at(iEvent).at(iTruePart).GetGeneratorName() << std::endl;
                        tps.at(iEvent).at(iTP).SetTrueParticle(&true_particles.at(iEvent).at(iTruePart));
                        matched_tps_counter++;
                        break;
                    }
                }
            }
            // Compute matched TPs percentage for each view
            for (size_t iView = 0; iView < APA::views.size(); ++iView) {
                std::vector<TriggerPrimitive*> these_tps_per_view;
                getPrimitivesForView(APA::views.at(iView), tps.at(iEvent), these_tps_per_view);

                int matched_in_view = 0;
                for (auto* tp : these_tps_per_view) {
                    if (tp->GetTrueParticle() != nullptr) {
                        matched_in_view++;
                    }
                }

                float matched_fraction = these_tps_per_view.empty() ? 0.0f : (matched_in_view * 100.0f) / these_tps_per_view.size();
                LogInfo << "Matched TPs in view " << APA::views.at(iView) << ": " << matched_fraction << " %" << std::endl;
                LogInfo << "Of these, the ones for which the true particle contains truth: " << std::endl;
                int truth_count = 0;
                for (auto* tp : these_tps_per_view) {
                    if (tp->GetTrueParticle() != nullptr && tp->GetTrueParticle()->GetGeneratorName() != "UNKNOWN") {
                        truth_count++;
                        // std::cout << "; Generator: " << tp->GetTrueParticle()->GetGeneratorName() ;
                    }
                }
                int gen_tp_fraction = these_tps_per_view.empty() ? 0 : static_cast<int>(truth_count * 100.0f / these_tps_per_view.size());
                LogInfo << "TPs with non-UNKNOWN generator: " << gen_tp_fraction << " %" << std::endl;
            }
        
            // LogInfo << "The views are " << APA::views.size() << std::endl;

            std::vector <std::vector<TriggerPrimitive*>> tps_per_view;
            tps_per_view.reserve(APA::views.size());

            std::vector <std::vector<cluster>> clusters_per_view;
            clusters_per_view.reserve(APA::views.size());
            
            LogInfo << "-------------------------" << std::endl;
            for (uint iView = 0; iView < APA::views.size(); iView++) {
                // divide the tps in views
                std::vector<TriggerPrimitive*> these_tps_per_view;
                getPrimitivesForView(APA::views.at(iView), tps.at(iEvent), these_tps_per_view);
                LogInfo << "Number of TPs in " << APA::views.at(iView) << " view: " << these_tps_per_view.size() << std::endl;
                tps_per_view.emplace_back(these_tps_per_view);
                // cluster the tps
                clusters_per_view.emplace_back(cluster_maker(tps_per_view.at(iView), ticks_limit, channel_limit, min_tps_to_cluster, adc_integral_cut.at(iView)));
                LogInfo << "Number of clusters in " << APA::views.at(iView) << " view: " << clusters_per_view.at(iView).size() << std::endl;
                LogInfo << "-------------------------" << std::endl;
            }

            // write clusters to root files

            // Extract base filename without _tpstream.root suffix
            std::string input_basename = filename.substr(filename.find_last_of("/\\") + 1);
            input_basename = input_basename.substr(0, input_basename.length() - 14); // Remove "_tpstream.root"
            std::string energy_cut_str = std::to_string(energy_cut);
            energy_cut_str.erase(energy_cut_str.find_last_not_of('0') + 1, std::string::npos);
            std::replace(energy_cut_str.begin(), energy_cut_str.end(), '.', 'p');
            std::string clusters_filename = outfolder + 
                "/" + input_basename + 
                "_clusters_tick" + std::to_string(ticks_limit) + 
                "_ch" + std::to_string(channel_limit) + 
                "_min" + std::to_string(min_tps_to_cluster)  + 
                "_tot" + std::to_string(min_time_over_threshold) + 
                "_en" + energy_cut_str +
            ".root";
            // Keep all events in the same output file: delete only before the first event
            if (iEvent == 1) {
                if (std::ifstream(clusters_filename)) {
                    std::remove(clusters_filename.c_str());
                    LogInfo << "Deleted existing file before first event: " << clusters_filename << std::endl;
                }
            }


            for (int iView = 0; iView < APA::views.size(); iView++) {
                LogInfo << "Writing " << APA::views.at(iView) << " clusters " << std::endl;
                write_clusters_to_root(clusters_per_view.at(iView), clusters_filename, APA::views.at(iView));
            }

            output_files.push_back(clusters_filename);

            LogInfo << " Output file is " << clusters_filename << std::endl;
            LogInfo << "############################" << std::endl;

            // break; // TEMP, just for testing one event at the time
        }
        // break; // TEMP, just for testing one file at the time
    }

    LogInfo << "Output files are:" << std::endl;

    for (const auto& file : output_files) {
        LogInfo << file << std::endl;
    }

    LogInfo << "Output folder: " << outfolder << std::endl;

    // stop the clock
    std::clock_t end = std::clock();

    // std::setprecision(3);
    LogInfo << "Time take  in seconds: " << float(end - start) / CLOCKS_PER_SEC << std::endl;
    LogInfo << "Time taken in minutes: " << float(end - start) / CLOCKS_PER_SEC / 60  << std::endl;
    LogInfo << "Time taken in hours: " << float(end - start) / CLOCKS_PER_SEC / 3600 << std::endl;


    // free the memory
    for (int i = 0; i < tps.size(); i++) {
        tps.at(i).clear();
    }
    tps.clear();
    for (int i = 0; i < true_particles.size(); i++) {
        true_particles.at(i).clear();
    }
    true_particles.clear();
    for (int i = 0; i < neutrinos.size(); i++) {
        neutrinos.at(i).clear();
    }
    neutrinos.clear();

    return 0;
}

