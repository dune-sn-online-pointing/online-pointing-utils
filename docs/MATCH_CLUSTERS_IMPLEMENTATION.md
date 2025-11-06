# Match Clusters Pipeline - Implementation Summary

## Overview
Successfully completed the refactoring and enhancement of the match_clusters pipeline, ignoring compatibility with old cluster files. All new code is ready for use with freshly generated cluster files.

## Completed Tasks

### 1. ✅ Match Clusters Script (scripts/match_clusters.sh)
- Complete bash script with error handling
- Help documentation (-h/--help)
- Build verification before execution
- Flexible JSON config path (-j/--json)
- Clear output formatting

**Usage:**
```bash
./scripts/match_clusters.sh -j json/match_clusters/test.json
```

### 2. ✅ Python Scripts Updated for cluster_id

#### generate_cluster_arrays.py
**Changes:**
- Added `'cluster_id'` to branches list (line ~437)
- Extract cluster_id: `cluster_id = int(data['cluster_id'][i])` (line ~488)
- Added cluster_id to metadata array as final element (line ~503)

**Metadata array structure (16 elements):**
```python
[is_marley, is_main_track, 
 true_pos_x, true_pos_y, true_pos_z,
 true_dir_x, true_dir_y, true_dir_z,
 true_mom_x, true_mom_y, true_mom_z,
 true_nu_energy, true_particle_energy,
 plane_number, is_es_interaction,
 cluster_id]  # NEW
```

#### create_volumes.py
**Changes:**
- Added `'cluster_id'` to branches list (line ~171)
- Added cluster_id to cluster_info dict (line ~234)
- Added `'main_cluster_id'` to volume metadata dict (line ~471)

**Volume metadata now includes:**
```python
{
    'event': ...,
    'plane': ...,
    'interaction_type': ...,
    # ... other fields ...
    'main_cluster_id': main_cluster['cluster_id']  # NEW
}
```

### 3. ✅ Analyze Matching App (src/app/analyze_matching.cpp)

**New application that computes comprehensive matching metrics:**

#### Features:
- **Efficiency Metrics**: Percentage of U/V/X clusters that find matches
- **Match Multiplicity**: Average matches per cluster, max matches
- **Truth-Based Purity**: Pure MARLEY matches (all 3 planes) vs partial
- **Spatial/Temporal Distributions**: For matched/unmatched clusters

#### Implementation Details:
- Reads matched cluster ROOT files
- Analyzes clusters_tree_multiplane
- Tracks cluster_id participation in matches
- Uses marley_tp_fraction > 0.5 threshold
- Computes per-plane statistics

#### Output Format:
```
=========================================
Cluster Matching Analysis Results
=========================================

Input Cluster Counts:
  U-plane clusters: X
  V-plane clusters: X
  X-plane clusters: X
  Multiplane matches: X

Matching Efficiency:
  X-plane efficiency: XX% (matched/total)
  U-plane efficiency: XX%
  V-plane efficiency: XX%

Match Multiplicity:
  Average matches per X cluster: X.XX
  Max matches for single X cluster: X
  Average matches per U cluster: X.XX
  Average matches per V cluster: X.XX

Truth-Based Purity (MARLEY events):
  MARLEY multiplane clusters: X
  Pure MARLEY matches (all 3 planes): X
  Partial MARLEY matches: X
  Purity: XX%

=========================================
```

#### Supporting Files:
- **JSON config**: `json/analyze_matching/test.json`
  ```json
  {
      "matched_clusters_file": "path/to/matched_clusters.root"
  }
  ```
- **Bash script**: `scripts/analyze_matching.sh`
  - Same structure as match_clusters.sh
  - Help, error handling, build verification
  - Usage: `./scripts/analyze_matching.sh -j json/analyze_matching/test.json`

#### CMakeLists Integration:
- Added to `src/app/CMakeLists.txt` (line ~68)
- Links with: `clustersLibs globalLib`
- Successfully compiles

### 4. ⏸️ End-to-End Testing
**Status**: Not yet done (waiting for fresh cluster files)

**Testing Plan:**
1. Generate new clusters with make_clusters (includes cluster_id)
2. Run match_clusters on new cluster file
3. Run analyze_matching on matched output
4. Verify Python scripts work with new data

## File Summary

### Modified Files:
1. `scripts/generate_cluster_arrays.py` - Added cluster_id to metadata
2. `scripts/create_volumes.py` - Added cluster_id to volume metadata
3. `scripts/match_clusters.sh` - Already complete
4. `src/app/CMakeLists.txt` - Added analyze_matching app

### New Files:
1. `src/app/analyze_matching.cpp` - Matching metrics application (316 lines)
2. `json/analyze_matching/test.json` - Test configuration
3. `scripts/analyze_matching.sh` - Execution script

## Technical Notes

### Cluster ID System
- **Type**: `int`, initialized to -1
- **Scope**: Per-file, unique across all planes
- **Assignment**: Incremental counter in make_clusters.cpp
- **Storage**: ROOT tree branch "cluster_id"
- **Usage**: Links U, V, X clusters in multiplane matches

### Old File Compatibility
**Decision**: Ignoring old cluster files
- Old files lack cluster_id branch
- Old files trigger ParametersManager errors
- Solution: Generate fresh data with current pipeline

### Pipeline Flow (with new data):
```
make_clusters → clusters.root (with cluster_id)
              ↓
match_clusters → matched_clusters.root
              ↓
analyze_matching → metrics output
              ↓
Python scripts → cluster/volume images with cluster_id metadata
```

## Next Steps

### For Testing:
1. Generate fresh clusters:
   ```bash
   ./build/src/app/make_clusters -j json/make_clusters/YOUR_CONFIG.json
   ```

2. Match clusters:
   ```bash
   ./scripts/match_clusters.sh -j json/match_clusters/test.json
   ```

3. Analyze matching:
   ```bash
   ./scripts/analyze_matching.sh -j json/analyze_matching/test.json
   ```

4. Generate cluster arrays:
   ```bash
   python3 scripts/generate_cluster_arrays.py --root-file OUTPUT_FILE.root
   ```

### For Future Enhancement:
- Add histograms to analyze_matching output
- Create ROOT plots for spatial distributions
- Add time-series analysis of matching efficiency
- Implement cut optimization based on metrics

## Build Status
✅ All applications compile successfully
✅ No errors or warnings
✅ Ready for testing with fresh data

---
*Generated: October 30, 2025*
*Branch: evilla/direct-tp-simide-matching*
