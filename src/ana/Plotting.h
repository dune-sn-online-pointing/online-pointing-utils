#ifndef PLOTTING_H
#define PLOTTING_H

#include  "Global.h"

// Helper function to add page number to canvas
void addPageNumber(TCanvas* canvas, int pageNum, int totalPages);

// Helper function to create binned averages with equal bin sizes
// Returns a TGraphErrors with averaged points
TGraphErrors* createBinnedAverageGraph(const std::vector<double>& x_data, const std::vector<double>& y_data);

#endif // PLOTTING_H