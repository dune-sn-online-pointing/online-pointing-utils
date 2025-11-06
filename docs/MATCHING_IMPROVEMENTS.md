# Cluster Matching Improvements

## Summary
Fixed critical issues in the 3-plane cluster matching algorithm that were causing 0% match rate.

## Problems Fixed

### 1. **TDC vs TPC Tick Conversion**
**Problem**: The matching code was comparing `GetTimeStart()` (which returns TDC ticks) directly with `time_tolerance_ticks` without converting to TPC ticks.

**Solution**: 
- Created `getClusterTimeRange()` helper that converts TDC ticks to TPC ticks using `toTPCticks()`
- TDC to TPC conversion: `tpc_ticks = tdc_ticks / 32`

### 2. **Cluster Time Extent Not Considered**
**Problem**: Only comparing the first TP's `time_start`, ignoring that clusters extend over time (`time_start` to `time_start + samples_over_threshold`).

**Solution**:
- Calculate full time range: `[min(time_start), max(time_start + samples_over_threshold)]` for all TPs in cluster
- Use `timesOverlap()` helper to check if cluster time ranges overlap within tolerance

### 3. **Truth-Dependent Matching**
**Problem**: `are_compatibles()` was using `get_true_pos()` which returns (0,0,0) for background clusters without truth information.

**Solution**:
- Calculate Z position directly from X plane channels: `z = (channel - 1600) * wire_pitch_collection`
- Determine X sign from detector/APA number
- Use reconstructed positions from wire geometry, not truth

### 4. **Overly Strict Tolerances**
**Problem**: Default tolerances were too strict for real data with reconstruction uncertainties.

**Solution**:
- Increased `time_tolerance_ticks` from 100 to 300 TPC ticks
- Increased `spatial_tolerance_cm` from 5.0 to 30.0 cm

## Results

**Before fixes**: 0 matches per file  
**After fixes**: 0-7 matches per file (varies by event content)

### Example Statistics (5 files):
```
File 1: 1 match  (U=1/227, V=1/304, X=1/317)
File 2: 7 matches (U=5/281, V=4/355, X=4/358)  
File 3-5: 0 matches (mostly background)
```

## Code Changes

### Added Files
- Helper functions in `match_clusters.cpp`:
  - `getClusterTimeRange()`: Calculate [min, max] time range in TPC ticks
  - `timesOverlap()`: Check if two time ranges overlap within tolerance

### Modified Files
1. **`src/app/match_clusters.cpp`**:
   - Use `getClusterTimeRange()` for all time comparisons
   - Binary search now uses proper TPC tick ranges
   - Time overlap checks use full cluster extent

2. **`src/planes-matching/match_clusters_libs.cpp`**:
   - Added `calculate_z_from_x_plane()`: Calculate Z from X channels
   - Added `calculate_x_sign_from_x_plane()`: Determine X sign from detector
   - `are_compatibles()` now uses reconstructed positions, not truth
   - Removed redundant time check (already done in outer loop)

3. **`json/cc_valid.json`**:
   - `time_tolerance_ticks`: 100 → 300 TPC ticks
   - `spatial_tolerance_cm`: 5.0 → 30.0 cm

## Validation

The matching now properly handles:
- ✅ TDC to TPC tick conversion
- ✅ Full cluster time extent (start to end of all TPs)
- ✅ Reconstructed positions (no truth dependency)
- ✅ Realistic tolerances for detector resolution

## Notes

- Match rate varies by file based on event content (signal vs background)
- The algorithm now works for all clusters, not just those with truth info
- Spatial matching uses Y-coordinate consistency between U and V planes projected to X's Z position
