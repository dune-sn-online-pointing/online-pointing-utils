# I/O and Folder Logic Explanation for Match Clusters Pipeline

## Overview

The match_clusters pipeline has three stages, each with specific I/O patterns and folder handling strategies. Here's how data flows through the system and how the truth preservation fix works.

## Pipeline Data Flow

```
TPs (trigger primitives)
    ↓ [make_clusters]
Single-plane clusters (U, V, X) with truth info
    ↓ [match_clusters]  
Multiplane matched clusters with preserved truth
    ↓ [analyze_matching]
Metrics and visualizations
```

---

## 1. make_clusters - Initial Clustering

### Input Handling
- **Config parameter**: `tps_folder` (path to directory)
- **File discovery**: `find_input_files()` searches for `*_tps.root` pattern
- **Filtering**: Only files with full TP information are processed
- **Key field**: `max_files` limits processing (we used `max_files: 1`)

### Output Folder Logic
```cpp
// Priority order for output folder:
1. CLI flag: --output-folder <path>
2. JSON config: "clusters_folder": "<path>"
3. JSON config: "outputFolder": "<path>"  
4. Default: "data/"
```

### Output File Naming
**Automatic naming scheme**:
```
{output_folder}/clusters_clusters_tick{T}_ch{C}_min{M}_tot{TOT}_e{E}/{basename}_clusters.root
```

Where:
- `T` = tick_limit (time window)
- `C` = channel_limit (spatial tolerance)
- `M` = min_tps_to_cluster (minimum size)
- `TOT` = tot_cut (time-over-threshold filter)
- `E` = energy_cut formatted (e.g., 0p0 for 0.0 MeV)

**Example**:
```
Input:  data/prod_es/prodmarley_nue_flat_es_...Z_gen_004499_..._tps.root
Output: output/clusters_clusters_tick5_ch2_min2_tot1_e0p0/
           prodmarley_nue_flat_es_...Z_gen_004499_..._clusters.root
```

### Truth Information at Creation
When creating clusters from TPs:
```cpp
// In Cluster::update_cluster_info() (Cluster.cpp:128-141)
if (tps_with_truth > 0) {
    // TPs have generator info, calculate from scratch
    supernova_tp_fraction = (float)count_marley / (float)tps.size();
} 
// else: preserve existing value (important for read/write cycle!)
```

**Why this matters**: When first created from TPs, clusters have full truth info. This fraction gets stored in the ROOT file.

---

## 2. match_clusters - Geometric Matching

### Input Handling
- **Config parameter**: `input_clusters_file` (single file path)
- **Tree reading**: Calls `read_clusters_from_tree()` 3 times (U, V, X views)
- **Important**: Clusters are read WITH their stored `marley_tp_fraction`

### The Critical I/O Fix

#### Problem Discovered
When reading clusters from file, TPs are reconstructed but lose generator truth:
```cpp
// In read_clusters_from_tree() (Clustering.cpp:725-792)
Float_t marley_tp_fraction = 0;  // Read from tree branch
// ... read all TPs and cluster metadata ...
cluster.set_supernova_tp_fraction(marley_tp_fraction);  // Stored value preserved!
```

But when writing, the OLD code recalculated:
```cpp
// OLD BUGGY CODE (before fix):
int marley_count = 0;
for (auto* tp : cl_tps) {
    if (tp->GetGeneratorName() contains "marley") marley_count++;
}
marley_tp_fraction = marley_count / cl_tps.size();  // Always 0!
```

**Why it failed**: Reconstructed TPs from file have `generator_name = "UNKNOWN"`, so recalculation always gives 0.

#### The Fix (Clustering.cpp:533-559)
```cpp
// Move declarations outside scope block
int cluster_truth_count = 0;
int marley_count = 0;

// Try to calculate from TPs
{
    const auto& cl_tps = Cluster.get_tps();
    for (auto* tp : cl_tps) {
        if (gen_name != "UNKNOWN") cluster_truth_count++;
        if (gen_name contains "marley") marley_count++;
    }
    // Calculate assuming TPs have truth
    marley_tp_fraction = marley_count / cl_tps.size();
}

// Critical fallback: if TPs lack truth, use stored value!
if (cluster_truth_count == 0) {
    marley_tp_fraction = Cluster.get_supernova_tp_fraction();  // Preserved from read!
    generator_tp_fraction = Cluster.get_generator_tp_fraction();
}
```

**Key insight**: The Cluster object itself stores the fraction when read from file. If TPs don't have truth info (detected by `cluster_truth_count == 0`), we use the stored value instead of the incorrect recalculation.

### Output Structure
```cpp
// Creates hierarchical ROOT structure:
TFile
  └─ TDirectory "clusters"
       └─ TTree "clusters_tree_multiplane"  // Note: different tree name!
```

**Branches written**:
- `marley_tp_fraction` ← Now correctly preserved!
- `n_tps`, `total_energy`, `total_charge`
- TP arrays: `tp_time_start`, `tp_detector`, etc.
- Truth info: `true_pos_x/y/z`, `true_label`, etc.

---

## 3. analyze_matching - Metrics Computation

### Input Handling
- **Config parameter**: `matched_clusters_file`
- **Tree name**: `clusters/clusters_tree_multiplane` (hardcoded path)
- **Key branches**: `marley_tp_fraction`, `n_tps`, `total_energy`

### Output
- Console metrics (efficiency, purity, counts)
- No file output (pure analysis tool)

---

## Folder Organization Strategy

### Working Directory Structure
```
online-pointing-utils/
├── data/
│   ├── prod_es/              # ES input TPs
│   │   └── *_tps.root
│   └── prod_cc/              # CC input TPs (unused - no tps files)
├── output/
│   ├── clusters_*/           # make_clusters outputs (auto-created)
│   │   └── *_clusters.root
│   ├── test_es_matched.root  # match_clusters output
│   └── *.json                # Config files
└── scripts/
    └── plot_matching_analysis.py  # Visualization
```

### Why This Organization?

1. **Separation of concerns**: Raw data (`data/`) vs processed results (`output/`)

2. **Automatic subfolder naming**: `make_clusters` creates descriptive folders encoding parameters, making it easy to identify clustering settings without opening files

3. **Config file locality**: Store configs near outputs for reproducibility

4. **Flat output structure**: match_clusters writes to single file in `output/`, not nested

---

## Key Takeaways

### 1. Truth Preservation Chain
```
make_clusters:  TPs (with truth) → Cluster.supernova_tp_fraction (calculated)
                                 ↓
                             ROOT file (stored)
                                 ↓
match_clusters: ROOT file (read) → Cluster.supernova_tp_fraction (preserved)
                                 → TPs reconstructed (NO truth)
                                 ↓
                            write_clusters:
                              - Try calculate from TPs
                              - If TPs lack truth (count=0)
                              - Use Cluster's stored value ✓
                                 ↓
                             ROOT file (preserved!)
```

### 2. No Recalculation Philosophy
**Core principle**: Once truth information is calculated at the TP level (make_clusters), it should NEVER be recalculated from downstream TPs that may lack truth info.

**Implementation**: The fix checks if TPs have truth before recalculating. If not, trust the Cluster object's stored value.

### 3. Folder Logic Is Conservative
- make_clusters: Creates descriptive subfolders automatically
- match_clusters: Writes to user-specified path exactly
- analyze_matching: Read-only, no folder creation

This prevents accidental overwrites while maintaining traceability.

---

## Testing Results

**Input**: 1 ES file with MARLEY events  
**Output**: 32 matched multiplane clusters  
**Success metrics**:
- ✅ 100% identified as MARLEY (marley_tp_fraction = 1.0)
- ✅ No loss of truth information through pipeline
- ✅ Energy range: 4.2 - 156.3 MeV
- ✅ Cluster sizes: 6 - 19 TPs per match

**Proof the fix works**: Before fix, all 32 showed `marley_tp_fraction = 0`. After fix, all 32 show `marley_tp_fraction = 1.0`.
