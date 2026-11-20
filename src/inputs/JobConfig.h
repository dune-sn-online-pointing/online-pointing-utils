// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-19 at 21:52:25 UTC
// modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-20 at 01:09:38 UTC (override/-f flag)
#ifndef JOBCONFIG_H
#define JOBCONFIG_H

#include <string>
#include <nlohmann/json.hpp>

// JobConfig: wraps command-line parsing (via CmdLineParser) and the JSON
// configuration (via nlohmann::json) so the reader works with a config object
// instead of raw variables. Typical use:
//
//     JobConfig config;
//     config.readArgs(argc, argv);        // -j <json>, -v, -d
//     config.readJson(config.jsonFile);   // load the parameters
//     ... config.inputFile(), config.maxCount(), config.energyCut() ...
//
// Known fields mirror json/local_es_bg.json; anything else is reachable with
// the generic get<T>(key, default) / has(key).
class JobConfig {
public:
    // Filled by readArgs().
    std::string jsonFile;        // -j / --json
    bool        verbose = false; // -v / --verbose
    bool        debug   = false; // -d / --debug (implies verbose)
    bool        override = false;// -f / --override (overwrite existing output)

    // Parse the command line for -j/--json, -v/--verbose, -d/--debug.
    // Also sets the global verboseMode/debugMode flags the rest of the code uses.
    void readArgs(int argc, char** argv);

    // Load the JSON configuration from `path` (usually config.jsonFile).
    // Throws (LogThrowIf) if the file cannot be opened.
    void readJson(const std::string& path);

    // --- typed accessors for the known config fields (JSON key in comment) ---
    std::string inputFile()             const { return str("inputFile"); }
    std::string bgFile()                const { return str("bgFile"); }
    std::string tpstreamInputFile()     const { return str("tpstream_input_file"); }
    std::string outputFolder()          const { return str("outputFolder"); }
    std::string sigFolder()             const { return str("sig_folder"); }
    std::string tpstreamFolder()        const { return str("tpstream_folder"); }
    std::string tpsBgFolder()           const { return str("tps_bg_folder"); }
    std::string clustersFolder()        const { return str("clusters_folder"); }
    std::string matchedClustersFolder() const { return str("matched_clusters_folder"); }
    std::string volumeImagesFolder()    const { return str("volume_images_folder"); }
    std::string productsPrefix()        const { return str("products_prefix"); }
    std::string filePattern()           const { return str("file_pattern", "*_tpstream.root"); }

    int    maxFiles()               const { return num<int>("max_files", -1); }
    int    backtrackerErrorMargin() const { return num<int>("backtracker_error_margin", 0); }
    int    timeToleranceTicks()     const { return num<int>("time_tolerance_ticks", 0); }
    int    spatialToleranceCm()     const { return num<int>("spatial_tolerance_cm", 0); }
    int    maxCount()               const { return num<int>("maxcount", 0); }
    int    skipEvent()              const { return num<int>("skipevent", 0); }
    int    tickLimit()              const { return num<int>("tick_limit", 0); }
    int    channelLimit()           const { return num<int>("channel_limit", 0); }
    int    minTpsToCluster()        const { return num<int>("min_tps_to_cluster", 0); }
    int    totCut()                 const { return num<int>("tot_cut", 0); }
    double energyCut()              const { return num<double>("energy_cut", 0.0); }

    // Generic access for anything not covered by a named getter.
    template <typename T> T get(const std::string& key, const T& def) const { return j_.value(key, def); }
    bool has(const std::string& key) const { return j_.contains(key); }
    const nlohmann::json& json() const { return j_; }

private:
    std::string str(const std::string& key, const std::string& def = "") const { return j_.value(key, def); }
    template <typename T> T num(const std::string& key, T def) const { return j_.value(key, def); }

    nlohmann::json j_;
};

#endif // JOBCONFIG_H
