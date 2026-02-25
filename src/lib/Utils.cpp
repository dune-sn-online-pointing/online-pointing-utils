#include "Global.h"

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

namespace {

bool has_all_view_trees(TDirectory* dir) {
  if (dir == nullptr) return false;
  for (const auto& view : APA::views) {
    const std::string tree_name = "clusters_tree_" + view;
    if (dir->Get(tree_name.c_str()) == nullptr) {
      return false;
    }
  }
  return true;
}

}

std::string toLower(std::string s){
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
  return s;
}

int toTPCticks(int tdcTicks) {
  return tdcTicks / get_conversion_tdc_to_tpc();
}

int toTDCticks(int tpcTicks) {
  return tpcTicks * get_conversion_tdc_to_tpc();
}

bool is_valid_clusters_output_file(const std::string& file_path) {
  TFile in_file(file_path.c_str(), "READ");
  if (in_file.IsZombie()) {
    return false;
  }

  auto* clusters_dir = dynamic_cast<TDirectory*>(in_file.Get("clusters"));
  auto* discarded_dir = dynamic_cast<TDirectory*>(in_file.Get("discarded"));
  if (clusters_dir == nullptr || discarded_dir == nullptr) {
    return false;
  }

  return has_all_view_trees(clusters_dir) && has_all_view_trees(discarded_dir);
}
