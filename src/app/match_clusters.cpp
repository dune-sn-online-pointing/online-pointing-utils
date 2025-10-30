// #include "position_calculator.h"
#include "Clustering.h"
#include "Cluster.h"
#include "geometry.h"
#include "Backtracking.h"
#include "match_clusters_libs.h"
#include "ParametersManager.h"

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

int main(int argc, char* argv[]) {
    CmdLineParser clp;

    clp.getDescription() << "> cluster_to_root app."<< std::endl;

    clp.addDummyOption("Main options");
    clp.addOption("json",    {"-j", "--json"}, "JSON file containing the configuration");

    clp.addDummyOption("Triggers");
    clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");

    clp.addDummyOption();
    // usage always displayed
    LogInfo << clp.getDescription().str() << std::endl;

    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;

    clp.parseCmdLine(argc, argv);

    LogThrowIf( clp.isNoOptionTriggered(), "No option was provided." );

    // Load parameters for energy conversion in Cluster constructor
    ParametersManager::getInstance().loadParameters();

    LogInfo << "Provided arguments: " << std::endl;
    LogInfo << clp.getValueSummary() << std::endl << std::endl;

    std::string json = clp.getOptionVal<std::string>("json");
    // read the configuration file
    std::ifstream i(json);
    nlohmann::json j;
    i >> j;
    
    // Get input/output paths
    std::string input_clusters_file;
    std::string output_file;
    
    if (j.contains("input_clusters_file")) {
        // Single file mode (explicit input file)
        input_clusters_file = j["input_clusters_file"];
        output_file = j["output_file"];
    } else {
        // Batch mode: input and output folders will be handled by the shell script
        LogThrowIf(true, "This executable expects explicit input_clusters_file and output_file in JSON.");
    }
    
    // Get matching parameters with defaults
    int time_tolerance_ticks = j.value("time_tolerance_ticks", 100);  // Default 100 ticks
    float spatial_tolerance_cm = j.value("spatial_tolerance_cm", 5.0);  // Default 5 cm
    
    LogInfo << "Input:  " << input_clusters_file << std::endl;
    LogInfo << "Output: " << output_file << std::endl;
    
    LogInfo << "Matching parameters:" << std::endl;
    LogInfo << "  Time tolerance: ±" << time_tolerance_ticks << " ticks (" 
            << (time_tolerance_ticks * 0.512) << " µs)" << std::endl;
    LogInfo << "  Spatial tolerance: " << spatial_tolerance_cm << " cm" << std::endl;

    // start the clock
    std::clock_t start;
    start = std::clock();
    
    // Read pre-computed clusters from single ROOT file (from clusters/ directory)
    // All views (U, V, X) are in separate trees in the same file
    LogInfo << "Reading clusters from file: " << input_clusters_file << std::endl;
    std::vector<Cluster> clusters_u = read_clusters_from_tree(input_clusters_file, "U");
    std::vector<Cluster> clusters_v = read_clusters_from_tree(input_clusters_file, "V");
    std::vector<Cluster> clusters_x = read_clusters_from_tree(input_clusters_file, "X");

    LogInfo << "Number of clusters after filtering: " << clusters_x.size() << std::endl;
    std::map<std::string, int> label_to_count;

    for (int i = 0; i < clusters_x.size(); i++) {
        if (label_to_count.find(clusters_x[i].get_true_label()) == label_to_count.end()) {
            label_to_count[clusters_x[i].get_true_label()] = 0;
        }
        label_to_count[clusters_x[i].get_true_label()]++;
    }
    for (auto const& x : label_to_count) {
        // LogInfo << "Label " << x.first << " has " << x.second << " clusters" << std::endl;
        LogInfo << x.first << " ";
    }
    LogInfo << std::endl;
    // if no clusters are found, return 0
    for (auto const& x : label_to_count) {
        // LogInfo << "Label " << x.first << " has " << x.second << " clusters" << std::endl;
        LogInfo << x.second << " ";
    }
    LogInfo << std::endl;
    LogInfo << "Number of clusters: " << clusters_u.size() << " " << clusters_v.size() << " " << clusters_x.size() << std::endl;
    std::vector<std::vector<Cluster>> clusters;
    std::vector<Cluster> multiplane_clusters;

    int start_j = 0;
    int start_k = 0;
    std::clock_t str;

    for (int i = 0; i < clusters_x.size(); i++) {
        if (i % 10000 == 0 && i != 0) {
            str = std::clock();
            LogInfo << "Cluster " << i << " Time: " << (str - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
            LogInfo << "Number of compatible clusters: " << multiplane_clusters.size() << std::endl;
        }

        int min_range_j = 0;
        int max_range_j = clusters_u.size();
        while (min_range_j < max_range_j) {
            start_j = (min_range_j + max_range_j) / 2;
            if (clusters_u[start_j].get_tps()[0]->GetTimeStart() < clusters_x[i].get_tps()[0]->GetTimeStart()) {
                min_range_j = start_j + 1;
            } else {
                max_range_j = start_j;
            }
        }
        start_j = std::max(start_j-10, 0);
        for (int j = start_j; j < clusters_u.size(); j++) {

            if (clusters_u[j].get_tps()[0]->GetTimeStart() > clusters_x[i].get_tps()[0]->GetTimeStart() + time_tolerance_ticks) {
                // LogInfo << "continue" << std::endl;
                break;
            }

            if (clusters_u[j].get_tps()[0]->GetTimeStart() < clusters_x[i].get_tps()[0]->GetTimeStart() - time_tolerance_ticks) {
                // LogInfo << "continue" << std::endl;
                continue;
            }

            // if not in the same apa skip
            if (int(clusters_u[j].get_tps()[0]->GetDetectorChannel()/APA::total_channels) != int(clusters_x[i].get_tps()[0]->GetDetectorChannel()/APA::total_channels)) {
                continue;
            }
            int min_range_k = 0;
            int max_range_k = clusters_v.size();
            while (min_range_k < max_range_k) {
                start_k = (min_range_k + max_range_k) / 2;
                if (clusters_v[start_k].get_tps()[0]->GetTimeStart() < clusters_x[i].get_tps()[0]->GetTimeStart()) {
                    min_range_k = start_k + 1;
                } else {
                    max_range_k = start_k;
                }
            }
            start_k = std::max(start_k-10, 0);
            for (int k = start_k; k < clusters_v.size(); k++) {
                if (clusters_v[k].get_tps()[0]->GetTimeStart() > clusters_u[j].get_tps()[0]->GetTimeStart() + time_tolerance_ticks) {
                    // LogInfo << "break" << std::endl;
                    break;
                }
                if (clusters_v[k].get_tps()[0]->GetTimeStart() < clusters_u[j].get_tps()[0]->GetTimeStart() - time_tolerance_ticks) {
                    // LogInfo << "break" << std::endl;
                    continue;
                }
                

                if (int(clusters_v[k].get_tps()[0]->GetDetectorChannel()/APA::total_channels) != int(clusters_x[i].get_tps()[0]->GetDetectorChannel()/APA::total_channels)) {
                    continue;
                }

                if (are_compatibles(clusters_u[j], clusters_v[k], clusters_x[i], spatial_tolerance_cm)) {
                    clusters.push_back({clusters_u[j], clusters_v[k], clusters_x[i]});
                    Cluster c = join_clusters(clusters_u[j], clusters_v[k], clusters_x[i]);
                    multiplane_clusters.push_back(c);
                    // TODO: include option to use true information
                    // if (match_with_true_pos(clusters_u[j], clusters_v[k], clusters_x[i], 5)) {
                    //     clusters.push_back({clusters_u[j], clusters_v[k], clusters_x[i]});
                    //     Cluster c = join_clusters(clusters_u[j], clusters_v[k], clusters_x[i]);
                    //     multiplane_clusters.push_back(c);
                    // }
                }
            }
        }
    }


    LogInfo << "Number of compatible clusters: " << clusters.size() << std::endl;
    // return counts of label
    std::map<std::string, int> label_to_count_compatible;
    for (int i = 0; i < clusters.size(); i++) {
        if (label_to_count_compatible.find(clusters[i][2].get_true_label()) == label_to_count_compatible.end()) {
            label_to_count_compatible[clusters[i][2].get_true_label()] = 0;
        }
        label_to_count_compatible[clusters[i][2].get_true_label()]++;
    }
    for (auto const& x : label_to_count_compatible) {
        // LogInfo << "Label " << x.first << " has " << x.second << " clusters" << std::endl;
        LogInfo << x.first << " ";
    }
    LogInfo << std::endl;
    for (auto const& x : label_to_count_compatible) {
        // LogInfo << "Label " << x.first << " has " << x.second << " clusters" << std::endl;
        LogInfo << x.second << " ";
    }
    LogInfo << std::endl;

    // Assign match IDs to clusters based on their matches
    // match_id: unique ID for each match group
    // match_type: 3 for U+V+X, 2 for U+X or V+X (future), -1 for no match
    // Note: Each cluster can only belong to ONE match (first one found)
    std::map<int, int> u_cluster_to_match;  // cluster_id -> match_id
    std::map<int, int> v_cluster_to_match;
    std::map<int, int> x_cluster_to_match;
    std::map<int, int> match_type_map;  // match_id -> match_type
    
    LogInfo << "Creating match_id mappings for " << clusters.size() << " matches..." << std::endl;
    for (size_t match_id = 0; match_id < clusters.size(); match_id++) {
        int u_id = clusters[match_id][0].get_cluster_id();
        int v_id = clusters[match_id][1].get_cluster_id();
        int x_id = clusters[match_id][2].get_cluster_id();
        
        // Only assign if not already matched (keep first match)
        if (u_cluster_to_match.find(u_id) == u_cluster_to_match.end()) {
            u_cluster_to_match[u_id] = match_id;
        }
        if (v_cluster_to_match.find(v_id) == v_cluster_to_match.end()) {
            v_cluster_to_match[v_id] = match_id;
        }
        if (x_cluster_to_match.find(x_id) == x_cluster_to_match.end()) {
            x_cluster_to_match[x_id] = match_id;
        }
        match_type_map[match_id] = 3;  // 3-plane match
    }
    
    LogInfo << "Match mappings created: U=" << u_cluster_to_match.size() 
            << ", V=" << v_cluster_to_match.size() 
            << ", X=" << x_cluster_to_match.size() << std::endl;

    
    LogInfo << "Writing matched clusters with match IDs to " << output_file << std::endl;

    // Save clusters with match_id information
    TFile* output_root = new TFile(output_file.c_str(), "RECREATE");
    if (output_root && !output_root->IsZombie()) {
        output_root->mkdir("clusters");
        output_root->cd("clusters");
        
        // Write U, V, X clusters with match_id
        write_clusters_with_match_id(clusters_u, u_cluster_to_match, output_root, "U");
        write_clusters_with_match_id(clusters_v, v_cluster_to_match, output_root, "V");
        write_clusters_with_match_id(clusters_x, x_cluster_to_match, output_root, "X");
        
        output_root->Close();
        LogInfo << "Matched clusters written to " << output_file << std::endl;
        LogInfo << "  U-plane clusters with match info: " << clusters_u.size() << std::endl;
        LogInfo << "  V-plane clusters with match info: " << clusters_v.size() << std::endl;
        LogInfo << "  X-plane clusters with match info: " << clusters_x.size() << std::endl;
        LogInfo << "  Total matches: " << clusters.size() << std::endl;
    } else {
        LogError << "Failed to create output file: " << output_file << std::endl;
    }
    delete output_root;
       

    // stop the clock
    std::clock_t end;
    end = std::clock();
    LogInfo << "Time: " << (end - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    return 0;
}
