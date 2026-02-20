#ifndef INPUT_OUTPUT_H
#define INPUT_OUTPUT_H

#include <initializer_list>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "root.h"

// Discovery helpers (shared across pipeline stages)
std::vector<std::string> find_input_files(const nlohmann::json& j, const std::vector<std::string>& file_suffixes);
std::vector<std::string> find_input_files(const nlohmann::json& j, const std::string& pattern);

// Basename handling for consistent skip/max propagation
std::string extractBasename(const std::string& filepath);
std::vector<std::string> findFilesMatchingBasenames(const std::vector<std::string>& basenames, const std::vector<std::string>& candidate_files);
std::vector<std::string> find_input_files_by_tpstream_basenames(const nlohmann::json& j, const std::string& pattern, int skip_files, int max_files);

// Folder/path utilities
bool ensureDirectoryExists(const std::string& folder);
std::string getTpstreamBaseFolder(const nlohmann::json& j);
std::string resolveFolderAgainstTpstream(const nlohmann::json& j, const std::string& folder, bool useBaseOnEmpty = false);
std::string getClustersFolder(const nlohmann::json& j);
std::string getConditionsString(const nlohmann::json& j);
std::string getOutputFolder(const nlohmann::json& j, const std::string& folder_type, const std::string& json_key);

// ROOT helpers
void bindBranch(TTree* tree, const char* name, void* address);
template <typename T> bool SetBranchWithFallback(TTree*, std::initializer_list<const char*>, T*, const std::string&);

#endif // INPUT_OUTPUT_H
