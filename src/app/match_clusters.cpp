// Batch-mode match_clusters - matches U/V/X plane clusters across files
#include "Clustering.h"
#include "Cluster.h"
#include "geometry.h"
#include "Backtracking.h"
#include "match_clusters_libs.h"
#include "ParametersManager.h"
#include "utils.h"

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

// Helper function to get cluster time range in TDC ticks (original tick units)
std::pair<int, int> getClusterTimeRange(const Cluster& cluster) {
    if (cluster.get_tps().empty()) return {0, 0};
    
    int min_time_tdc = INT_MAX;
    int max_time_tdc = INT_MIN;
    
    for (auto* tp : cluster.get_tps()) {
        int start_tdc = tp->GetTimeStart();
        int end_tdc = tp->GetTimeStart() + tp->GetSamplesOverThreshold();
        min_time_tdc = std::min(min_time_tdc, start_tdc);
        max_time_tdc = std::max(max_time_tdc, end_tdc);
    }
    
    return {min_time_tdc, max_time_tdc};
}

// Check if two cluster time ranges overlap within tolerance (in TDC ticks)
bool timesOverlap(const std::pair<int, int>& range1, const std::pair<int, int>& range2, int tolerance_tdc) {
    // Check if ranges overlap considering tolerance
    // range1: [min1, max1], range2: [min2, max2]
    // They overlap if: max1 + tolerance >= min2 AND max2 + tolerance >= min1
    return (range1.second + tolerance_tdc >= range2.first) && (range2.second + tolerance_tdc >= range1.first);
}

int main(int argc, char* argv[]) {
    CmdLineParser clp;

    clp.getDescription() << "> match_clusters batch processing app."<< std::endl;

    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");
    clp.addOption("skip_files", {"-s", "--skip", "--skip-files"}, "Number of files to skip at start (overrides JSON)", -1);
    clp.addOption("max_files", {"-m", "--max", "--max-files"}, "Maximum number of files to process (overrides JSON)", -1);
    clp.addOption("outFolder", {"--outFolder", "--output-folder"}, "Output folder path (overrides JSON)");

    clp.addDummyOption("Triggers");
    clp.addTriggerOption("override", {"-f", "--override"}, "Override existing output files");
    clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");
    clp.addTriggerOption("debugMode", {"-d"}, "RunDebugMode, bool");

    clp.addDummyOption();
    LogInfo << clp.getDescription().str() << std::endl;
    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;

    clp.parseCmdLine(argc, argv);
    LogThrowIf( clp.isNoOptionTriggered(), "No option was provided." );

    // Load parameters
    ParametersManager::getInstance().loadParameters();

    LogInfo << "Provided arguments: " << std::endl;
    LogInfo << clp.getValueSummary() << std::endl << std::endl;

    std::string json = clp.getOptionVal<std::string>("json");
    std::ifstream i(json);
    nlohmann::json j;
    i >> j;
    
    // Handle skip and max files with CLI override
    int max_files = j.value("max_files", -1);
    int skip_files = j.value("skip_files", 0);
    if (clp.isOptionTriggered("skip_files")) {
        skip_files = clp.getOptionVal<int>("skip_files");
    }
    if (clp.isOptionTriggered("max_files")) {
        max_files = clp.getOptionVal<int>("max_files");
    }
    bool override = false;
    if (clp.isOptionTriggered("override")) {
        override = true;
    }
    
    // Get matching parameters with defaults
    int time_tolerance_ticks_tpc = j.value("time_tolerance_ticks", 100);  // In TPC ticks (from JSON)
    int time_tolerance_ticks_tdc = toTDCticks(time_tolerance_ticks_tpc);  // Convert to TDC ticks for matching
    float spatial_tolerance_cm = j.value("spatial_tolerance_cm", 5.0);
    
    if (verboseMode) {
        LogInfo << "Matching parameters:" << std::endl;
        LogInfo << "  time_tolerance: " << time_tolerance_ticks_tpc << " TPC ticks = " << time_tolerance_ticks_tdc << " TDC ticks" << std::endl;
        LogInfo << "  spatial_tolerance: " << spatial_tolerance_cm << " cm" << std::endl;
    }
    
    // Use tpstream-based file tracking
    std::vector<std::string> cluster_files = find_input_files_by_tpstream_basenames(j, "clusters", skip_files, max_files);
    
    // Get output folder
    std::string matched_clusters_folder;
    if (clp.isOptionTriggered("outFolder")) {
        matched_clusters_folder = clp.getOptionVal<std::string>("outFolder");
    } else {
        matched_clusters_folder = getOutputFolder(j, "matched_clusters", "matched_clusters_folder");
    }
    
    LogInfo << "=========================================" << std::endl;
    LogInfo << "Processing clusters from folder: " << j.value("clusters_folder", "") << std::endl;
    LogInfo << "Output will be written to: " << matched_clusters_folder << std::endl;
    LogInfo << "Maximum files to process: " << max_files << std::endl;
    LogInfo << "Skip files: " << skip_files << std::endl;
    LogInfo << "Found " << cluster_files.size() << " cluster files to process" << std::endl;
    LogInfo << "=========================================" << std::endl;
    
    // Create output directory
    ensureDirectoryExists(matched_clusters_folder);
    
    int processed = 0;
    int failed = 0;
    verboseMode = clp.isOptionTriggered("verboseMode");  // Set global verboseMode
    
    // Process each cluster file
    for (size_t file_idx = 0; file_idx < cluster_files.size(); file_idx++) {
        std::string input_clusters_file = cluster_files[file_idx];
        
        // Generate output filename
        std::string basename = std::filesystem::path(input_clusters_file).stem().string();
        if (basename.size() > 14 && basename.substr(basename.size()-14) == "_bg_clusters") {
            basename = basename.substr(0, basename.size()-14);
        } else if (basename.size() > 9 && basename.substr(basename.size()-9) == "_clusters") {
            basename = basename.substr(0, basename.size()-9);
        }
        std::string output_file = matched_clusters_folder + "/" + basename + "_matched.root";
        
        if (verboseMode) LogInfo << "[" << (file_idx+1) << "/" << cluster_files.size() << "] Processing: " 
                << std::filesystem::path(input_clusters_file).filename().string() << std::endl;
        
        // Check if output exists
        if (!override && std::filesystem::exists(output_file)) {
            LogInfo << "  Output exists (" << output_file << "), skipping (use -f to override)" << std::endl;
            continue;
        }
        
        try {
            std::clock_t start = std::clock();
            
            // Read clusters from clusters/ directory
            if (verboseMode) LogInfo << "  Reading clusters..." << std::endl;
            std::vector<Cluster> clusters_u = read_clusters_from_tree(input_clusters_file, "U");
            std::vector<Cluster> clusters_v = read_clusters_from_tree(input_clusters_file, "V");
            std::vector<Cluster> clusters_x = read_clusters_from_tree(input_clusters_file, "X");

            if (verboseMode) LogInfo << "  Clusters: U=" << clusters_u.size() << " V=" << clusters_v.size() << " X=" << clusters_x.size() << std::endl;
            
            // Read discarded clusters from discarded/ directory
            std::vector<Cluster> discarded_u = read_clusters_from_tree(input_clusters_file, "U", "discarded");
            std::vector<Cluster> discarded_v = read_clusters_from_tree(input_clusters_file, "V", "discarded");
            std::vector<Cluster> discarded_x = read_clusters_from_tree(input_clusters_file, "X", "discarded");
            
            if (verboseMode && (discarded_u.size() > 0 || discarded_v.size() > 0 || discarded_x.size() > 0)) {
                LogInfo << "  Discarded: U=" << discarded_u.size() 
                        << " V=" << discarded_v.size() 
                        << " X=" << discarded_x.size() << std::endl;
            }
            
            // Match clusters
            std::vector<std::vector<Cluster>> matches;
            std::vector<Cluster> multiplane_clusters;

            int start_j = 0;
            int start_k = 0;
            
            int test_combinations = 0;
            int failed_time_u = 0, failed_event_u = 0, failed_apa_u = 0;
            int failed_time_v = 0, failed_event_v = 0, failed_apa_v = 0;
            int failed_spatial = 0;

            for (size_t i = 0; i < clusters_x.size(); i++) {
                auto x_time_range = getClusterTimeRange(clusters_x[i]);
                
                // Binary search for starting point in U using TPC ticks
                int min_range_j = 0;
                int max_range_j = clusters_u.size();
                while (min_range_j < max_range_j) {
                    start_j = (min_range_j + max_range_j) / 2;
                    auto u_time = getClusterTimeRange(clusters_u[start_j]);
                    if (u_time.first < x_time_range.first) {
                        min_range_j = start_j + 1;
                    } else {
                        max_range_j = start_j;
                    }
                }
                start_j = std::max(start_j-10, 0);
                
                for (size_t j = start_j; j < clusters_u.size(); j++) {
                    auto u_time_range = getClusterTimeRange(clusters_u[j]);
                    
                    // Check if U cluster time range overlaps with X cluster time range
                    if (u_time_range.first > x_time_range.second + time_tolerance_ticks_tdc) break;
                    if (!timesOverlap(u_time_range, x_time_range, time_tolerance_ticks_tdc)) { failed_time_u++; continue; }
                    if (clusters_u[j].get_tps()[0]->GetEvent() != clusters_x[i].get_tps()[0]->GetEvent()) { failed_event_u++; continue; }
                    if (int(clusters_u[j].get_tps()[0]->GetDetectorChannel()/APA::total_channels) != int(clusters_x[i].get_tps()[0]->GetDetectorChannel()/APA::total_channels)) { failed_apa_u++; continue; }
                    
                    // Binary search for starting point in V
                    int min_range_k = 0;
                    int max_range_k = clusters_v.size();
                    while (min_range_k < max_range_k) {
                        start_k = (min_range_k + max_range_k) / 2;
                        auto v_time = getClusterTimeRange(clusters_v[start_k]);
                        if (v_time.first < x_time_range.first) {
                            min_range_k = start_k + 1;
                        } else {
                            max_range_k = start_k;
                        }
                    }
                    start_k = std::max(start_k-10, 0);
                    
                    for (size_t k = start_k; k < clusters_v.size(); k++) {
                        test_combinations++;
                        auto v_time_range = getClusterTimeRange(clusters_v[k]);
                        
                        // Check if V cluster time range overlaps with U and X cluster time ranges
                        if (v_time_range.first > u_time_range.second + time_tolerance_ticks_tdc) break;
                        if (!timesOverlap(v_time_range, u_time_range, time_tolerance_ticks_tdc)) { failed_time_v++; continue; }
                        if (clusters_v[k].get_tps()[0]->GetEvent() != clusters_x[i].get_tps()[0]->GetEvent()) { failed_event_v++; continue; }
                        if (int(clusters_v[k].get_tps()[0]->GetDetectorChannel()/APA::total_channels) != int(clusters_x[i].get_tps()[0]->GetDetectorChannel()/APA::total_channels)) { failed_apa_v++; continue; }

                        if (are_compatibles(clusters_u[j], clusters_v[k], clusters_x[i], spatial_tolerance_cm)) {
                            matches.push_back({clusters_u[j], clusters_v[k], clusters_x[i]});
                            Cluster c = join_clusters(clusters_u[j], clusters_v[k], clusters_x[i]);
                            multiplane_clusters.push_back(c);
                        } else {
                            failed_spatial++;
                        }
                    }
                }
            }

            if (verboseMode) {
                LogInfo << "  Found " << matches.size() << " compatible matches" << std::endl;
                LogInfo << "  Total clusters: U=" << clusters_u.size() << " V=" << clusters_v.size() << " X=" << clusters_x.size() << std::endl;
                LogInfo << "  Combinations tested: " << test_combinations << std::endl;
                LogInfo << "  Failed filters: time_u=" << failed_time_u << " event_u=" << failed_event_u << " apa_u=" << failed_apa_u;
                LogInfo << " time_v=" << failed_time_v << " event_v=" << failed_event_v << " apa_v=" << failed_apa_v;
                LogInfo << " spatial=" << failed_spatial << std::endl;
            }
            
            // Assign match IDs and track X plane matching details
            std::map<int, int> u_cluster_to_match;
            std::map<int, int> v_cluster_to_match;
            std::map<int, int> x_cluster_to_match;
            std::map<int, int> match_type_map;
            
            // Track which U and V clusters each X cluster matched to
            std::map<int, int> x_to_u_map;  // X cluster_id -> U cluster_id
            std::map<int, int> x_to_v_map;  // X cluster_id -> V cluster_id
            
            for (size_t match_id = 0; match_id < matches.size(); match_id++) {
                int u_id = matches[match_id][0].get_cluster_id();
                int v_id = matches[match_id][1].get_cluster_id();
                int x_id = matches[match_id][2].get_cluster_id();
                
                if (u_cluster_to_match.find(u_id) == u_cluster_to_match.end()) u_cluster_to_match[u_id] = match_id;
                if (v_cluster_to_match.find(v_id) == v_cluster_to_match.end()) v_cluster_to_match[v_id] = match_id;
                if (x_cluster_to_match.find(x_id) == x_cluster_to_match.end()) {
                    x_cluster_to_match[x_id] = match_id;
                    // Track the first U and V matched to this X cluster
                    if (x_to_u_map.find(x_id) == x_to_u_map.end()) x_to_u_map[x_id] = u_id;
                    if (x_to_v_map.find(x_id) == x_to_v_map.end()) x_to_v_map[x_id] = v_id;
                }
                match_type_map[match_id] = 3;
            }
            
            // Compute matching statistics
            int x_matched_u_only = 0;
            int x_matched_v_only = 0;
            int x_matched_both = 0;
            for (size_t i = 0; i < clusters_x.size(); i++) {
                int x_id = clusters_x[i].get_cluster_id();
                bool has_u = x_to_u_map.find(x_id) != x_to_u_map.end();
                bool has_v = x_to_v_map.find(x_id) != x_to_v_map.end();
                if (has_u && has_v) x_matched_both++;
                else if (has_u) x_matched_u_only++;
                else if (has_v) x_matched_v_only++;
            }
            
            // Write output
            TFile* output_root = new TFile(output_file.c_str(), "RECREATE");
            if (output_root && !output_root->IsZombie()) {
                // Create clusters directory and write matched clusters
                output_root->mkdir("clusters");
                output_root->cd("clusters");
                
                write_clusters_with_match_id(clusters_u, u_cluster_to_match, output_root, "U");
                write_clusters_with_match_id(clusters_v, v_cluster_to_match, output_root, "V");
                write_clusters_with_match_id(clusters_x, x_cluster_to_match, output_root, "X", &x_to_u_map, &x_to_v_map);
                
                // Create discarded directory for consistency (will be empty in current production)
                output_root->cd();
                output_root->mkdir("discarded");
                output_root->cd("discarded");
                
                // Write empty discarded clusters with match_id=-1 (currently none to write)
                std::map<int, int> empty_match_map;  // Empty map means all get match_id=-1
                write_clusters_with_match_id(discarded_u, empty_match_map, output_root, "U");
                write_clusters_with_match_id(discarded_v, empty_match_map, output_root, "V");
                write_clusters_with_match_id(discarded_x, empty_match_map, output_root, "X");
                
                output_root->Close();
                
                int matched_u = u_cluster_to_match.size();
                int matched_v = v_cluster_to_match.size();
                int matched_x = x_cluster_to_match.size();
                if (verboseMode) {
                    LogInfo << "  ✓ Success (" << matches.size() << " matches)" << std::endl;
                    LogInfo << "  Matched clusters: U=" << matched_u << "/" << clusters_u.size()
                            << " V=" << matched_v << "/" << clusters_v.size()
                            << " X=" << matched_x << "/" << clusters_x.size() << std::endl;
                    LogInfo << "  X plane matching: U+V=" << x_matched_both 
                            << ", U-only=" << x_matched_u_only 
                            << ", V-only=" << x_matched_v_only 
                            << ", unmatched=" << (clusters_x.size() - matched_x) << std::endl;
                }
                processed++;
            } else {
                LogError << "  ✗ Failed to create output file" << std::endl;
                failed++;
            }
            delete output_root;
            
        } catch (std::exception& e) {
            LogError << "  ✗ Failed: " << e.what() << std::endl;
            failed++;
        }
    }
    
    LogInfo << "=========================================" << std::endl;
    LogInfo << "Batch processing complete!" << std::endl;
    LogInfo << "Processed: " << processed << " files" << std::endl;
    LogInfo << "Failed: " << failed << " files" << std::endl;
    LogInfo << "=========================================" << std::endl;
    
    return (failed > 0) ? 1 : 0;
}
