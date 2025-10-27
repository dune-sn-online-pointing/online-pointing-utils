#!/bin/bash

# Script to update analyze_clusters.cpp with:
# 1. Conversion factors from parameters
# 2. Clustering conditions extraction from filename
# 3. Energy plots only for X plane

cd /afs/cern.ch/work/e/evilla/private/dune/online-pointing-utils

# Create the helper function to add after the binned average function
cat > /tmp/helper_func.txt << 'EOF'

// Helper function to extract clustering parameters from filename
std::map<std::string, int> extractClusteringParams(const std::string& filename) {
  std::map<std::string, int> params;
  params["tick"] = -1;
  params["ch"] = -1;
  params["min"] = -1;
  params["tot"] = -1;
  
  // Extract from filename like: xxx_tick3_ch2_min2_tot2_clusters.root
  std::regex tick_regex("_tick(\\d+)");
  std::regex ch_regex("_ch(\\d+)");
  std::regex min_regex("_min(\\d+)");
  std::regex tot_regex("_tot(\\d+)");
  
  std::smatch match;
  if (std::regex_search(filename, match, tick_regex) && match.size() > 1) {
    params["tick"] = std::stoi(match[1].str());
  }
  if (std::regex_search(filename, match, ch_regex) && match.size() > 1) {
    params["ch"] = std::stoi(match[1].str());
  }
  if (std::regex_search(filename, match, min_regex) && match.size() > 1) {
    params["min"] = std::stoi(match[1].str());
  }
  if (std::regex_search(filename, match, tot_regex) && match.size() > 1) {
    params["tot"] = std::stoi(match[1].str());
  }
  
  return params;
}

// Helper function to format clustering conditions for plot title
std::string formatClusteringConditions(const std::map<std::string, int>& params) {
  std::stringstream ss;
  bool first = true;
  if (params.at("ch") >= 0) {
    ss << "ch:" << params.at("ch");
    first = false;
  }
  if (params.at("tick") >= 0) {
    if (!first) ss << ", ";
    ss << "tick:" << params.at("tick");
    first = false;
  }
  if (params.at("tot") >= 0) {
    if (!first) ss << ", ";
    ss << "tot:" << params.at("tot");
    first = false;
  }
  if (params.at("min") >= 0) {
    if (!first) ss << ", ";
    ss << "min:" << params.at("min");
  }
  return ss.str();
}
EOF

echo "Helper functions created. Now updating analyze_clusters.cpp..."
echo "Please apply the changes manually - this is complex file editing"
