#include "Clustering.h"
#include "Cluster.h"
#include "ParametersManager.h"
#include "utils.h"
#include "verbosity.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <vector>

LoggerInit([] { Logger::getUserHeader() << "[" << FILENAME << "]"; });

namespace {

struct TruthMatchSample {
    int x_id;
    int x_event;
    int x_apa;
    float best_u_dist;
    int best_u_delta_event;
    float best_v_dist;
    int best_v_delta_event;
};

bool has_valid_truth(const std::vector<float>& pos) {
    if (pos.size() < 3) return false;
    return std::fabs(pos[0]) + std::fabs(pos[1]) + std::fabs(pos[2]) > 1e-3f;
}

float truth_distance(const std::vector<float>& a, const std::vector<float>& b) {
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    float dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace

int main(int argc, char* argv[]) {
    CmdLineParser clp;

    clp.getDescription() << "> match_clusters_truth diagnostic app." << std::endl;

    clp.addDummyOption("Main options");
    clp.addOption("json", {"-j", "--json"}, "JSON file containing the configuration");
    clp.addOption("skip_files", {"-s", "--skip", "--skip-files"}, "Number of files to skip at start (overrides JSON)", -1);
    clp.addOption("max_files", {"-m", "--max", "--max-files"}, "Maximum number of files to process (overrides JSON)", -1);
    clp.addOption("truth_tolerance_cm", {"--truth-tolerance"}, "Maximum 3D distance (cm) allowed between truth positions", 10.0f);

    clp.addDummyOption("Triggers");
    clp.addTriggerOption("verboseMode", {"-v"}, "RunVerboseMode, bool");
    clp.addTriggerOption("debugMode", {"-d"}, "RunDebugMode, bool");

    clp.addDummyOption();
    LogInfo << clp.getDescription().str() << std::endl;
    LogInfo << "Usage: " << std::endl;
    LogInfo << clp.getConfigSummary() << std::endl << std::endl;

    clp.parseCmdLine(argc, argv);
    LogThrowIf(clp.isNoOptionTriggered(), "No option was provided.");

    ParametersManager::getInstance().loadParameters();

    LogInfo << "Provided arguments: " << std::endl;
    LogInfo << clp.getValueSummary() << std::endl << std::endl;

    verboseMode = clp.isOptionTriggered("verboseMode") || clp.isOptionTriggered("debugMode");
    debugMode = clp.isOptionTriggered("debugMode");

    std::string json_path = clp.getOptionVal<std::string>("json");
    std::ifstream json_file(json_path);
    LogThrowIf(!json_file.is_open(), "Unable to open JSON config: " + json_path);

    nlohmann::json config;
    json_file >> config;

    int max_files = config.value("max_files", -1);
    int skip_files = config.value("skip_files", 0);
    if (clp.isOptionTriggered("skip_files")) {
        skip_files = clp.getOptionVal<int>("skip_files");
    }
    if (clp.isOptionTriggered("max_files")) {
        max_files = clp.getOptionVal<int>("max_files");
    }

    float truth_tolerance_cm = clp.isOptionTriggered("truth_tolerance_cm")
                                   ? clp.getOptionVal<float>("truth_tolerance_cm")
                                   : config.value("truth_tolerance_cm", 10.0f);
    if (truth_tolerance_cm <= 0.0f) {
        LogWarning << "Provided truth tolerance " << truth_tolerance_cm << " cm is <= 0. Using 1 cm." << std::endl;
        truth_tolerance_cm = 1.0f;
    }

    if (verboseMode) {
        LogInfo << "Truth matching tolerance: " << truth_tolerance_cm << " cm" << std::endl;
    }

    std::vector<std::string> cluster_files = find_input_files_by_tpstream_basenames(config, "clusters", skip_files, max_files);

    LogInfo << "=========================================" << std::endl;
    LogInfo << "Processing " << cluster_files.size() << " cluster files" << std::endl;
    LogInfo << "Truth tolerance: " << truth_tolerance_cm << " cm" << std::endl;
    LogInfo << "=========================================" << std::endl;

    int global_main_x = 0;
    int global_with_truth = 0;
    int global_truth_matched_complete = 0;
    int global_truth_matched_u_only = 0;
    int global_truth_matched_v_only = 0;
    int global_truth_unmatched = 0;
    int global_missing_truth = 0;
    double global_u_distance_sum = 0.0;
    double global_v_distance_sum = 0.0;
    int global_u_distance_count = 0;
    int global_v_distance_count = 0;

    const size_t max_unmatched_samples = 5;

    for (size_t file_idx = 0; file_idx < cluster_files.size(); ++file_idx) {
        const std::string& input_file = cluster_files[file_idx];
        if (verboseMode) {
            LogInfo << "[" << (file_idx + 1) << "/" << cluster_files.size() << "] Processing: "
                    << std::filesystem::path(input_file).filename().string() << std::endl;
        }

    std::vector<Cluster> clusters_u = read_clusters_from_tree(input_file, "U");
    std::vector<Cluster> clusters_v = read_clusters_from_tree(input_file, "V");
    std::vector<Cluster> clusters_x = read_clusters_from_tree(input_file, "X");

        int main_x_count = 0;
        for (const auto& cluster : clusters_x) {
            if (cluster.get_is_main_cluster()) main_x_count++;
        }

        int with_truth = 0;
        int truth_matched_complete = 0;
        int truth_matched_u_only = 0;
        int truth_matched_v_only = 0;
        int truth_unmatched = 0;
        int missing_truth = 0;
        double u_distance_sum = 0.0;
        double v_distance_sum = 0.0;
        int u_distance_count = 0;
        int v_distance_count = 0;
        std::vector<TruthMatchSample> unmatched_samples;

        auto find_best_match = [&](Cluster& target,
                                  std::vector<Cluster>& candidates,
                                  float& best_distance_cm,
                                  int& best_index,
                                  int& best_event_delta) {
            const auto& target_truth = target.get_true_pos();
            best_distance_cm = std::numeric_limits<float>::max();
            best_index = -1;
            best_event_delta = 0;
            for (size_t i = 0; i < candidates.size(); ++i) {
                auto& candidate = candidates[i];
                const auto& candidate_truth = candidate.get_true_pos();
                if (!has_valid_truth(candidate_truth)) continue;
                float dist = truth_distance(target_truth, candidate_truth);
                if (dist < best_distance_cm) {
                    best_distance_cm = dist;
                    best_index = static_cast<int>(i);
                    best_event_delta = candidate.get_event() - target.get_event();
                }
            }
        };

        for (auto& x_cluster : clusters_x) {
            if (!x_cluster.get_is_main_cluster()) continue;
            global_main_x++;

            if (!has_valid_truth(x_cluster.get_true_pos())) {
                missing_truth++;
                global_missing_truth++;
                continue;
            }

            with_truth++;
            global_with_truth++;

            float best_u_distance = 0.0f;
            int best_u_index = -1;
            int best_u_event_delta = 0;
            find_best_match(x_cluster, clusters_u, best_u_distance, best_u_index, best_u_event_delta);

            float best_v_distance = 0.0f;
            int best_v_index = -1;
            int best_v_event_delta = 0;
            find_best_match(x_cluster, clusters_v, best_v_distance, best_v_index, best_v_event_delta);

            bool has_u_match = best_u_index >= 0 && best_u_distance <= truth_tolerance_cm;
            bool has_v_match = best_v_index >= 0 && best_v_distance <= truth_tolerance_cm;

            if (has_u_match) {
                u_distance_sum += best_u_distance;
                u_distance_count++;
                global_u_distance_sum += best_u_distance;
                global_u_distance_count++;
            }
            if (has_v_match) {
                v_distance_sum += best_v_distance;
                v_distance_count++;
                global_v_distance_sum += best_v_distance;
                global_v_distance_count++;
            }

            if (has_u_match && has_v_match) {
                truth_matched_complete++;
                global_truth_matched_complete++;
            } else if (has_u_match) {
                truth_matched_u_only++;
                global_truth_matched_u_only++;
            } else if (has_v_match) {
                truth_matched_v_only++;
                global_truth_matched_v_only++;
            } else {
                truth_unmatched++;
                global_truth_unmatched++;
                if (unmatched_samples.size() < max_unmatched_samples) {
                    TruthMatchSample sample;
                    sample.x_id = x_cluster.get_cluster_id();
                    sample.x_event = x_cluster.get_event();
                    sample.x_apa = int(x_cluster.get_tps()[0]->GetDetectorChannel() / APA::total_channels);
                    sample.best_u_dist = (best_u_index >= 0) ? best_u_distance : -1.0f;
                    sample.best_u_delta_event = (best_u_index >= 0) ? best_u_event_delta : 0;
                    sample.best_v_dist = (best_v_index >= 0) ? best_v_distance : -1.0f;
                    sample.best_v_delta_event = (best_v_index >= 0) ? best_v_event_delta : 0;
                    unmatched_samples.push_back(sample);
                }
            }
        }

        if (verboseMode) {
            LogInfo << "  Clusters: U=" << clusters_u.size() << " V=" << clusters_v.size()
                    << " X=" << clusters_x.size() << " (main=" << main_x_count << ")" << std::endl;
        }

        LogInfo << "  Truth-matching stats: with_truth=" << with_truth
                << " missing_truth=" << missing_truth
                << " complete=" << truth_matched_complete
                << " U-only=" << truth_matched_u_only
                << " V-only=" << truth_matched_v_only
                << " unmatched=" << truth_unmatched << std::endl;

        auto print_average = [&](const char* label, double sum, int count) {
            if (count > 0) {
                LogInfo << "    Avg " << label << " distance: " << (sum / count)
                        << " cm (" << count << " matches)" << std::endl;
            }
        };
        print_average("U", u_distance_sum, u_distance_count);
        print_average("V", v_distance_sum, v_distance_count);

        if (!unmatched_samples.empty()) {
            LogInfo << "  Sample unmatched main X clusters (best candidate distances):" << std::endl;
            for (size_t idx = 0; idx < unmatched_samples.size(); ++idx) {
                const auto& sample = unmatched_samples[idx];
                LogInfo << "    X_id=" << sample.x_id
                        << " event=" << sample.x_event
                        << " apa=" << sample.x_apa
                        << " best_U_dist=" << sample.best_u_dist
                        << " (delta_event=" << sample.best_u_delta_event << ")"
                        << " best_V_dist=" << sample.best_v_dist
                        << " (delta_event=" << sample.best_v_delta_event << ")" << std::endl;
            }
        }
    }

    LogInfo << "=========================================" << std::endl;
    LogInfo << "GLOBAL TRUTH-MATCHING STATISTICS" << std::endl;
    LogInfo << "=========================================" << std::endl;
    LogInfo << "Main X clusters processed: " << global_main_x << std::endl;
    LogInfo << "With truth info: " << global_with_truth << std::endl;
    LogInfo << "Missing truth: " << global_missing_truth << std::endl;
    LogInfo << "Truth matches (complete): " << global_truth_matched_complete << std::endl;
    LogInfo << "Truth matches (U-only): " << global_truth_matched_u_only << std::endl;
    LogInfo << "Truth matches (V-only): " << global_truth_matched_v_only << std::endl;
    LogInfo << "Truth unmatched (with truth info): " << global_truth_unmatched << std::endl;

    auto print_global_average = [&](const char* label, double sum, int count) {
        if (count > 0) {
            LogInfo << "Average " << label << " truth distance: " << (sum / count)
                    << " cm across " << count << " matches" << std::endl;
        }
    };
    print_global_average("U", global_u_distance_sum, global_u_distance_count);
    print_global_average("V", global_v_distance_sum, global_v_distance_count);
    LogInfo << "=========================================" << std::endl;

    return 0;
}
