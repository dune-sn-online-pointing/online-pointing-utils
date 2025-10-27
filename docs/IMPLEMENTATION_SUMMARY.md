# Summary of Changes - Clustering Analysis Updates

## Changes Made

### 1. `analyze_clusters.cpp` - Use Separate Conversion Factors

**Changes:**
- Added `#include <regex>` for filename parsing
- Modified conversion factors to read from `parameters/conversion.dat`:
  - `ADC_TO_MEV_COLLECTION` from `conversion.adc_to_energy_factor_collection` (3600.0 ADC/MeV)
  - `ADC_TO_MEV_INDUCTION` from `conversion.adc_to_energy_factor_induction` (900.0 ADC/MeV)
- Added helper functions:
  - `extractClusteringParams()` - Extracts tick, ch, min, tot from filename
  - `formatClusteringConditions()` - Formats params for display (e.g., "ch:2, tick:3, tot:2")

**Energy Histogram Changes:**
- All `h_energy_*` histogram fills now restricted to **X-plane only** using `if (pd.name == "X")`
- This applies to:
  - `h_energy_pure_marley`
  - `h_energy_pure_noise`
  - `h_energy_hybrid`
  - `h_energy_background`
  - `h_energy_mixed_signal_bkg`

### 2. `make_clusters.cpp` - Metadata Tree

**Changes:**
- Added metadata tree creation before file close
- Tree name: `clustering_metadata`
- Stores clustering parameters as single-entry tree with branches:
  - `tick_limit` (Int)
  - `channel_limit` (Int)
  - `min_tps_to_cluster` (Int)
  - `adc_integral_cut_induction` (Int)
  - `adc_integral_cut_collection` (Int)
  - `tot_cut` (Int)

**Benefits:**
- Downstream tools can read clustering parameters directly from ROOT file
- No need to parse filename
- More reliable and self-documenting

### 3. `parameters/conversion.dat` - Conversion Factors

**Existing values** (already in file):
```
< conversion.adc_to_energy_factor_collection = 3600.0 >
< conversion.adc_to_energy_factor_induction = 900.0 >
```

These are now used by `analyze_clusters.cpp` instead of hardcoded values.

## Usage

### Creating Clusters with Metadata
```bash
./scripts/make_clusters.sh -j config.json
```
The output file will now contain a `clustering_metadata` tree.

### Reading Metadata
In ROOT or Python:
```python
import ROOT
f = ROOT.TFile("clusters.root")
meta = f.Get("clustering_metadata")
meta.GetEntry(0)
print(f"Clustering: tick={meta.tick_limit}, ch={meta.channel_limit}")
```

### Analyzing Clusters
```bash
./scripts/analyze_clusters.sh -j config.json
```
- Energy plots now use correct conversion factors for each plane
- Energy distributions are X-plane only
- Plot titles can include clustering conditions (if extracted from filename)

## Testing

Compile:
```bash
cd build
make analyze_clusters make_clusters
```

Both applications compiled successfully! ✅

## Technical Notes

1. **Conversion Factors**: Collection plane has ~4x higher ADC/MeV than induction planes
2. **Energy Histograms**: Now properly represent collection plane energy only
3. **Metadata**: Enables future tools to read parameters programmatically
4. **Backward Compatibility**: Old cluster files without metadata still work (uses filename parsing as fallback)

## Files Modified

- `src/app/analyze_clusters.cpp` - Conversion factors, X-plane restriction, helper functions
- `src/app/make_clusters.cpp` - Metadata tree creation
- No changes to `parameters/conversion.dat` (already had correct values)
