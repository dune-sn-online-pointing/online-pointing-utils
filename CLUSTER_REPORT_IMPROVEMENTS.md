# Cluster Analysis Report Improvements

## Summary of Changes (October 13, 2025)

### 1. **Title Page - Clustering Parameters Summary**
- **Added**: Automatic extraction and display of clustering parameters from filename
- Extracts parameters like `tick5_ch2_min2_tot1` from filenames
- Displays:
  - Time tolerance (ticks)
  - Channel tolerance
  - Min cluster size (TPs)
  - Min total charge threshold
- Better organized layout with clustering parameters at 50% height

### 2. **Cluster Size Histograms**
- **Changed**: Maximum X-axis from 1000 to **100** for better visualization
- **Added**: Axis labels "n_TPs" and "Clusters"
- Applies to all three planes (U, V, X)

### 3. **Total Charge Plots**
- **Changed**: Maximum X-axis increased from 1000 to **300 ADC**
- **Added**: Missing axis labels "Total charge [ADC]" and "Clusters"
- Better range for typical cluster charges

### 4. **2D Histogram: nTPs vs Total Charge**
- **Changed**: X-axis range from 100 to **60** (max nTPs)
- **Changed**: Y-axis range from 1e6 to **300** (max total charge in ADC)
- **Added**: Axis label "Total charge [ADC]"
- Much better zoom for typical cluster distributions

### 5. **SN Clusters Per Event**
- **Changed**: Maximum X-axis capped at **40** clusters
- Better visualization for typical event multiplicities
- Prevents outliers from stretching the axis

### 6. **True Neutrino Energy**
- **Changed**: Scale from **logarithmic to linear**
- Better visualization for the energy distribution in the 0-100 MeV range
- Particle energy remains in log scale as appropriate

### 7. **Min Distance from True Position**
- **Removed**: Entire plot page deleted as requested
- Reduces report from 15 to 14 pages

### 8. **Additional Improvements**
- Updated total page count to 14
- Added proper axis labels throughout
- Improved histogram binning for better resolution
- Enhanced title page layout with parameters section

## Files Modified

- `src/app/analyze_clusters.cpp`:
  - Title page layout and parameter extraction (lines ~353-417)
  - Histogram initialization with proper ranges and labels (lines ~184-240)
  - 2D histogram ranges for nTPs vs charge (line ~190)
  - SN clusters per event max range (lines ~490-505)
  - Neutrino energy linear scale (lines ~574-589)
  - Removed min_distance plot (deleted lines ~600-607)

## Build and Run

```bash
# Build
cd /home/virgolaema/dune/online-pointing-utils/build
cmake --build . --target analyze_clusters -j8

# Run
cd /home/virgolaema/dune/online-pointing-utils
./build/src/app/analyze_clusters -j test_analyze.json

# Output
output/cc_valid_tick5_ch2_min2_tot1_clusters_report.pdf
```

## Sample JSON Configuration

```json
{
  "inputFile": "data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root",
  "output_folder": "output/"
}
```

## Results

✅ All requested changes implemented
✅ Report successfully generated
✅ PDF opened in VS Code for review

The report now provides:
- Clear clustering parameter summary on first page
- Better zoom ranges for all histograms
- Proper axis labels throughout
- Linear scale for neutrino energy
- Cleaner layout without min_distance plot
- More focused visualization on relevant data ranges
