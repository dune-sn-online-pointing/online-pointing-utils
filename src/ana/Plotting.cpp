#include "Plotting.h"

// Helper function to add page number to canvas
void addPageNumber(TCanvas* canvas, int pageNum, int totalPages) {
  canvas->cd();
  TText* pageText = new TText(0.95, 0.97, Form("Page %d/%d", pageNum, totalPages));
  pageText->SetTextAlign(31); // right-aligned
  pageText->SetTextSize(0.02);
  pageText->SetNDC();
  pageText->Draw();
}


TGraphErrors* createBinnedAverageGraph(const std::vector<double>& x_data, const std::vector<double>& y_data) {
  if (x_data.size() != y_data.size() || x_data.empty()) {
    return nullptr;
  }
  
  // Create pairs and sort by x value
  std::vector<std::pair<double, double>> data;
  for (size_t i = 0; i < x_data.size(); i++) {
    data.push_back({x_data[i], y_data[i]});
  }
  std::sort(data.begin(), data.end());
  
  // Define bins with equal size of 5 MeV
  std::vector<double> bin_edges;
  
  // Find data range
  double x_min = 0.0;  // Start from 0
  double x_max = 70.0; // Maximum energy we care about
  
  // Create equal-sized bins of 5 MeV
  double bin_width = 5.0;
  for (double edge = x_min; edge <= x_max; edge += bin_width) {
    bin_edges.push_back(edge);
  }
  
  // Calculate averages in each bin
  std::vector<double> bin_centers, bin_y_avg, bin_x_err, bin_y_err;
  
  size_t data_idx = 0;
  for (size_t bin_idx = 0; bin_idx < bin_edges.size() - 1; bin_idx++) {
    double bin_low = bin_edges[bin_idx];
    double bin_high = bin_edges[bin_idx + 1];
    
    std::vector<double> x_in_bin, y_in_bin;
    
    // Collect points in this bin
    while (data_idx < data.size() && data[data_idx].first < bin_high) {
      if (data[data_idx].first >= bin_low) {
        x_in_bin.push_back(data[data_idx].first);
        y_in_bin.push_back(data[data_idx].second);
      }
      data_idx++;
    }
    
    // Calculate averages if we have points
    if (!x_in_bin.empty()) {
      double x_mean = std::accumulate(x_in_bin.begin(), x_in_bin.end(), 0.0) / x_in_bin.size();
      double y_mean = std::accumulate(y_in_bin.begin(), y_in_bin.end(), 0.0) / y_in_bin.size();
      
      // Calculate standard errors
      double x_std = 0, y_std = 0;
      for (size_t i = 0; i < x_in_bin.size(); i++) {
        x_std += (x_in_bin[i] - x_mean) * (x_in_bin[i] - x_mean);
        y_std += (y_in_bin[i] - y_mean) * (y_in_bin[i] - y_mean);
      }
      x_std = std::sqrt(x_std / x_in_bin.size()) / std::sqrt(x_in_bin.size());
      y_std = std::sqrt(y_std / y_in_bin.size()) / std::sqrt(y_in_bin.size());
      
      bin_centers.push_back(x_mean);
      bin_y_avg.push_back(y_mean);
      bin_x_err.push_back(x_std);
      bin_y_err.push_back(y_std);
    }
    
    // Reset index for next bin (allow overlap)
    data_idx = 0;
    while (data_idx < data.size() && data[data_idx].first < bin_high) {
      data_idx++;
    }
  }
  
  // Create TGraphErrors
  TGraphErrors* gr = new TGraphErrors(bin_centers.size(),
                                       bin_centers.data(),
                                       bin_y_avg.data(),
                                       bin_x_err.data(),
                                       bin_y_err.data());
  return gr;
}