#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

#include "ParametersManager.h"

// APA configuration for 1x2x2 detector
const int CHANNELS_PER_APA = 2560;

// Structure matching the ROOT file format (same as in Clustering.cpp)
struct TPBranches {
    int event;
    UShort_t version;
    UInt_t detid;
    UInt_t channel;
    ULong64_t samples_over_threshold;
    ULong64_t time_start;
    ULong64_t samples_to_peak;
    UInt_t adc_integral;
    UShort_t adc_peak;
};

void setupInputBranches(TTree* tree, TPBranches& tp) {
    tree->SetBranchAddress("event", &tp.event);
    tree->SetBranchAddress("version", &tp.version);
    tree->SetBranchAddress("detid", &tp.detid);
    tree->SetBranchAddress("channel", &tp.channel);
    tree->SetBranchAddress("samples_over_threshold", &tp.samples_over_threshold);
    tree->SetBranchAddress("time_start", &tp.time_start);
    tree->SetBranchAddress("samples_to_peak", &tp.samples_to_peak);
    tree->SetBranchAddress("adc_integral", &tp.adc_integral);
    tree->SetBranchAddress("adc_peak", &tp.adc_peak);
}

void setupOutputBranches(TTree* tree, TPBranches& tp) {
    tree->Branch("event", &tp.event, "event/I");
    tree->Branch("version", &tp.version, "version/s");
    tree->Branch("detid", &tp.detid, "detid/i");
    tree->Branch("channel", &tp.channel, "channel/i");
    tree->Branch("samples_over_threshold", &tp.samples_over_threshold, "samples_over_threshold/l");
    tree->Branch("time_start", &tp.time_start, "time_start/l");
    tree->Branch("samples_to_peak", &tp.samples_to_peak, "samples_to_peak/l");
    tree->Branch("adc_integral", &tp.adc_integral, "adc_integral/i");
    tree->Branch("adc_peak", &tp.adc_peak, "adc_peak/s");
}

int getAPA(int channel) {
    return channel / CHANNELS_PER_APA;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file.root> <output_directory>" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Splits TriggerPrimitive data by APA (N APAs, 2560 channels each)" << std::endl;
        std::cerr << "Creates N output files: <output_dir>/apa0_tps.root, apa1_tps.root, etc." << std::endl;
        return 1;
    }

    std::string input_filename = argv[1];
    std::string output_dir = argv[2];

    // Ensure output directory ends with /
    if (output_dir.back() != '/') {
        output_dir += "/";
    }

    // Load detector configuration (optional; defaults preserve old behavior)
    int n_apas = 4;
    std::string detector_name;
    try {
        ParametersManager::getInstance().loadParameters();
        if (ParametersManager::getInstance().hasParameter("detector.n_apas")) {
            n_apas = ParametersManager::getInstance().getInt("detector.n_apas");
        }
        if (ParametersManager::getInstance().hasParameter("detector.name")) {
            detector_name = ParametersManager::getInstance().getString("detector.name");
        }
    } catch (const std::exception&) {
        // Keep defaults if parameter loading fails
    }

    std::cout << "======================================" << std::endl;
    std::cout << "APA Splitter for TriggerPrimitive Data" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "Input file: " << input_filename << std::endl;
    std::cout << "Output directory: " << output_dir << std::endl;
    if (!detector_name.empty()) {
        std::cout << "Detector: " << detector_name << std::endl;
    }
    std::cout << "Configuration:" << std::endl;
    std::cout << "  - APAs: " << n_apas << std::endl;
    std::cout << "  - Channels per APA: " << CHANNELS_PER_APA << std::endl;
    std::cout << "======================================" << std::endl;

    // Open input file
    TFile* input_file = TFile::Open(input_filename.c_str(), "READ");
    if (!input_file || input_file->IsZombie()) {
        std::cerr << "ERROR: Cannot open input file: " << input_filename << std::endl;
        return 1;
    }

    // Get TriggerPrimitive tree (try both naming conventions)
    TTree* input_tree = (TTree*)input_file->Get("TriggerPrimitive");
    if (!input_tree) {
        input_tree = (TTree*)input_file->Get("tps/TriggerPrimitive");
    }
    if (!input_tree) {
        input_tree = (TTree*)input_file->Get("tps");  // TPs at root level
    }
    if (!input_tree) {
        input_tree = (TTree*)input_file->Get("tps/tps");  // Old format: TPs inside directory
    }
    if (!input_tree) {
        std::cerr << "ERROR: Cannot find TriggerPrimitive tree in input file" << std::endl;
        std::cerr << "Tried: 'TriggerPrimitive', 'tps/TriggerPrimitive', 'tps', and 'tps/tps'" << std::endl;
        input_file->Close();
        return 1;
    }

    Long64_t n_entries = input_tree->GetEntries();
    std::cout << "Total TPs in input: " << n_entries << std::endl;

    // Setup input branches
    TPBranches tp_in;
    setupInputBranches(input_tree, tp_in);

    // Create output files and trees for each APA
    std::vector<TFile*> output_files(n_apas);
    std::vector<TTree*> output_trees(n_apas);
    std::vector<TPBranches> tp_out(n_apas);
    std::vector<Long64_t> apa_counters(n_apas, 0);

    for (int apa = 0; apa < n_apas; apa++) {
        std::string output_filename = output_dir + "apa" + std::to_string(apa) + "_tps.root";
        output_files[apa] = new TFile(output_filename.c_str(), "RECREATE");
        if (!output_files[apa] || output_files[apa]->IsZombie()) {
            std::cerr << "ERROR: Cannot create output file: " << output_filename << std::endl;
            return 1;
        }

        // Create tps directory (to match input file structure)
        TDirectory* tps_dir = output_files[apa]->mkdir("tps");
        tps_dir->cd();
        
        // Create tree inside tps directory with name "tps"
        output_trees[apa] = new TTree("tps", ("TriggerPrimitive data for APA " + std::to_string(apa)).c_str());
        setupOutputBranches(output_trees[apa], tp_out[apa]);
    }

    // Process all TPs
    std::cout << "Processing TPs..." << std::endl;
    Long64_t report_interval = n_entries / 10;
    if (report_interval == 0) report_interval = 1;

    for (Long64_t i = 0; i < n_entries; i++) {
        input_tree->GetEntry(i);

        // Determine which APA this TP belongs to
        int apa_id = getAPA(tp_in.channel);

        // Validate APA ID
        if (apa_id < 0 || apa_id >= n_apas) {
            std::cerr << "WARNING: Invalid channel " << tp_in.channel 
                      << " maps to APA " << apa_id << " (entry " << i << ")" << std::endl;
            continue;
        }

        // Copy to appropriate APA output
        tp_out[apa_id].event = tp_in.event;
        tp_out[apa_id].version = tp_in.version;
        tp_out[apa_id].detid = tp_in.detid;
        tp_out[apa_id].channel = tp_in.channel;
        tp_out[apa_id].samples_over_threshold = tp_in.samples_over_threshold;
        tp_out[apa_id].time_start = tp_in.time_start;
        tp_out[apa_id].samples_to_peak = tp_in.samples_to_peak;
        tp_out[apa_id].adc_integral = tp_in.adc_integral;
        tp_out[apa_id].adc_peak = tp_in.adc_peak;

        output_trees[apa_id]->Fill();
        apa_counters[apa_id]++;

        // Progress report
        if (i % report_interval == 0) {
            std::cout << "  Progress: " << (100 * i / n_entries) << "%" << std::endl;
        }
    }

    std::cout << "Processing complete!" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "TPs per APA:" << std::endl;

    // Write and close output files
    for (int apa = 0; apa < n_apas; apa++) {
        // Navigate to tps directory and write tree there
        TDirectory* tps_dir = (TDirectory*)output_files[apa]->Get("tps");
        if (tps_dir) {
            tps_dir->cd();
            output_trees[apa]->Write();
        }
        output_files[apa]->Close();
        delete output_files[apa];

        std::cout << "  APA " << apa << ": " << apa_counters[apa] << " TPs" << std::endl;
    }

    // Close input file
    input_file->Close();
    delete input_file;

    std::cout << "======================================" << std::endl;
    std::cout << "Split complete! Output files:" << std::endl;
    for (int apa = 0; apa < n_apas; apa++) {
        std::string output_filename = output_dir + "apa" + std::to_string(apa) + "_tps.root";
        std::cout << "  " << output_filename << std::endl;
    }
    std::cout << "======================================" << std::endl;

    return 0;
}
