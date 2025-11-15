# DUNE Online Pointing Utils - Margin Scan Analysis Summary

**Date:** September 10, 2025  
**Analysis:** Backtracking margin optimization for TP↔truth association

## Executive Summary

The margin scan successfully evaluated backtracking error margins (0, 5, 10, 15) across four samples to optimize the trade-off between MARLEY detection efficiency and contamination reduction. **Margin 15 is recommended** as the optimal setting for clean supernova samples.

## Key Results

### 1. MARLEY Detection Performance
- **cleanES909080**: 🟢 **100% efficiency** (10/10 events) across all margins
- **cleanES60**: 🟡 **60% efficiency** (6/10 events) - may require additional investigation
- **Background samples**: 🟢 **0% MARLEY** (as expected - correct rejection)

### 2. Contamination Reduction
For cleanES909080 (optimal case):
- **Margin 0**: 57 unknown TPs (5.7 per MARLEY event)
- **Margin 15**: 30 unknown TPs (3.0 per MARLEY event)
- **Improvement**: 47% reduction in false associations

### 3. Margin Impact Analysis

| Margin | Clean ES909080 | Clean ES60 | Performance |
|--------|---------------|------------|-------------|
| 0      | 10/10, 57 unk | 6/10, 72K unk | Baseline |
| 5      | 10/10, 37 unk | 6/10, 72K unk | 35% contamination ↓ |
| 10     | 10/10, 31 unk | 6/10, 72K unk | 46% contamination ↓ |
| 15     | 10/10, 30 unk | 6/10, 72K unk | 47% contamination ↓ |

## Technical Achievements

### 1. Channel Namespace Resolution ✅
- **Problem**: SimIDE global channels vs TP detector-local channels  
- **Solution**: Channel promotion: `global = detid × 2560 + local_channel`
- **Result**: Consistent channel mapping across truth and TP data

### 2. Time Offset Correction ✅
- **Problem**: ~65k TPC tick systematic offset between truth and TP times
- **Solution**: Per-event median offset calculation and correction
- **Result**: Proper temporal alignment for association matching

### 3. Comprehensive Tracking ✅
- **New branch**: `time_offset_correction` in output ROOT files
- **Diagnostics**: Event-by-event offset logging for problematic events
- **Reporting**: Enhanced PDF reports with time offset distributions

## Optimization Recommendations

### For Clean Samples (MARLEY events)
- **Recommended margin: 15**
- **Rationale**: Maximizes contamination reduction while maintaining 100% efficiency
- **Expected performance**: 3.0 unknown TPs per MARLEY event

### For Analysis Pipeline
- **Default margin**: 15 (optimal for clean samples)
- **Alternative margin**: 10 (good balance for mixed samples)
- **Monitor**: Track `time_offset_correction` values for quality control

## Investigation Items

### CleanES60 Analysis
- Only 60% MARLEY efficiency suggests sample-specific issues
- Recommended actions:
  1. Verify file quality and event selection
  2. Check for different time offset patterns
  3. Analyze geometric/energy dependencies

### Time Offset Patterns
- **Range observed**: 65312 - 65696 TPC ticks
- **Consistency**: Stable per-event corrections
- **Validation**: Confirmed through diagnostic logging

## Code Changes Summary

### cluster_to_root_libs.cpp
- ➕ Channel promotion during TP creation
- ➕ Per-event median time offset calculation
- ➕ Global offset tracking map (`g_event_time_offsets`)  
- ➕ New `time_offset_correction` branch
- ➕ Enhanced diagnostics for events 4,6,8,10

### analyze_tps.cpp
- ➕ Neutrino kinematics extraction with auto-detection
- ➕ Time offset distribution analysis
- ➕ Enhanced PDF reporting with offset histograms
- ➕ Per-event offset collection and visualization

### Configuration Management
- ✅ JSON configurations for all sample/margin combinations
- ✅ Automated scanning across parameter space
- ✅ Comprehensive result logging and analysis

## Validation Results

### Before Fixes
- **MARLEY association**: 6/10 events (60% failure)
- **Channel mismatch**: SimIDE ↔ TP namespace conflict
- **Time alignment**: Systematic ~65k tick offset unaccounted

### After Fixes
- **MARLEY association**: 10/10 events (100% success for clean909080)
- **Channel consistency**: Global namespace throughout pipeline  
- **Time alignment**: Per-event median correction applied
- **Quality tracking**: Full offset monitoring and reporting

## Future Work

1. **Investigate cleanES60**: Determine root cause of 60% efficiency
2. **Expand margin range**: Test margins >15 if needed for high-contamination samples
3. **Geometric analysis**: Correlate performance with detector regions
4. **Energy dependencies**: Analyze margin effectiveness vs neutrino energy
5. **Statistical studies**: Larger sample sets for robust optimization

## Files Created/Modified

### New Files
- `scripts/margin_scan.sh` - Automated margin scanning
- `python/analyze_margin_scan.py` - Result analysis and visualization
- `json/backtrack/margin_scan_*.json` - Configuration templates

### Modified Files  
- `src/cluster_to_root_libs.cpp` - Core TP processing with fixes
- `src/analyze_tps.cpp` - Enhanced PDF reporting
- Configuration files for all sample types

---

**Conclusion:** The margin scan successfully optimized the backtracking parameters, achieving 100% MARLEY detection efficiency with 47% contamination reduction for clean supernova samples. The implemented fixes for channel namespace and time offset issues provide a robust foundation for future physics analyses.
