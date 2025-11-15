# Code Structure and Architecture Guide

**Last Updated**: November 2025  
**Repository**: online-pointing-utils  
**Purpose**: Comprehensive technical documentation for developers

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Core Components](#core-components)
4. [Data Flow](#data-flow)
5. [Module Reference](#module-reference)
6. [Python Integration](#python-integration)
7. [Configuration System](#configuration-system)
8. [Testing Framework](#testing-framework)
9. [Performance Considerations](#performance-considerations)
10. [Development Workflow](#development-workflow)

---

## Overview

The online-pointing-utils repository implements a complete pipeline for processing DUNE detector Trigger Primitives (TPs) to enable real-time supernova neutrino pointing. The system combines C++ applications for performance-critical operations with Python tools for analysis and visualization.

### Design Principles

- **Modularity**: Distinct libraries for backtracking, clustering, matching, and analysis
- **Performance**: C++ for compute-intensive tasks, Python for flexible analysis
- **Configurability**: JSON-driven workflows with auto-generated folder structures
- **Truth Integration**: Monte Carlo truth embedded throughout for validation
- **Reproducibility**: Condition-encoded output names ensure traceability

---

## Architecture

### High-Level System Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         INPUT DATA                              │
│  TPStream ROOT files (*_tpstream.root) + Truth information      │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                    BACKTRACKING STAGE                           │
│  C++ App: backtrack_tpstream                                    │
│  - Extract signal TPs from full stream                          │
│  - Embed truth: particle ID, energy, position, direction        │
│  - Output: *_tps.root (signal only)                             │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                  BACKGROUND ADDITION                            │
│  C++ App: add_backgrounds                                       │
│  - Overlay realistic detector noise                             │
│  - Preserve truth information                                   │
│  - Output: tps_bg/*_tps_bg.root                                 │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                    CLUSTERING STAGE                             │
│  C++ App: make_clusters                                         │
│  - Group TPs by spatial-temporal proximity                      │
│  - Apply ToT and energy filters                                 │
│  - Identify main tracks vs. blips                               │
│  - Output: clusters_<conditions>/*_clusters.root                │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ├───────────────┬─────────────────┐
                           ▼               ▼                 ▼
                  ┌────────────────┐  ┌─────────────┐  ┌─────────────┐
                  │ 3-Plane Match  │  │   Volume    │  │   Cluster   │
                  │ (match_clusters)│  │  Analysis   │  │   Analysis  │
                  │                │  │  (Python)   │  │   (C++/Py)  │
                  └────────────────┘  └─────────────┘  └─────────────┘
```

### Component Layers

```
┌─────────────────────────────────────────────────────────────┐
│                    APPLICATION LAYER                        │
│  Executables: make_clusters, backtrack_tpstream, etc.      │
└───────────────────────────┬─────────────────────────────────┘
                            │
┌───────────────────────────┴─────────────────────────────────┐
│                     LIBRARY LAYER                           │
│  - clustering/: Clustering algorithms                       │
│  - backtracking/: Truth matching and position calculation   │
│  - planesMatching/: 3-plane geometric reconstruction        │
│  - lib/: Utilities (IO, parameters, geometry)               │
└───────────────────────────┬─────────────────────────────────┘
                            │
┌───────────────────────────┴─────────────────────────────────┐
│                    OBJECT LAYER                             │
│  Data Structures: TriggerPrimitive, Cluster, TrueParticle   │
└───────────────────────────┬─────────────────────────────────┘
                            │
┌───────────────────────────┴─────────────────────────────────┐
│                  EXTERNAL DEPENDENCIES                      │
│  ROOT (I/O), nlohmann/json, simple-cpp-logger, C++17 STL    │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Components

### 1. Data Structures (`src/objects/`)

#### TriggerPrimitive (`TriggerPrimitive.hpp`)
Represents a single hit in the detector.

```cpp
class TriggerPrimitive {
    // Core properties
    int time_start_;           // Start time (ticks since trigger)
    int time_over_threshold_;  // Pulse width (ticks)
    int channel_;              // Detector channel ID
    int adc_integral_;         // Integrated charge (ADC counts)
    int view_;                 // Plane: 0=U, 1=V, 2=X
    
    // Truth information (from backtracking)
    float true_x_, true_y_, true_z_;  // True 3D position (cm)
    int true_pdg_;                     // Particle type (PDG code)
    float true_energy_;                // Deposited energy (MeV)
    int file_number_;                  // Event index
    std::string label_;                // "MARLEY" or "BKG"
};
```

**Key Methods**:
- `GetTimeStart()`, `GetChannel()`, `GetAdcIntegral()`, `GetView()`
- `GetTruePos()`, `GetTrueEnergy()`, `GetLabel()`
- `SetTruth(...)` - Embed Monte Carlo information

#### Cluster (`Cluster.h`)
Collection of TPs forming a reconstructed object.

```cpp
class Cluster {
    std::vector<TriggerPrimitive> tps_;  // Constituent TPs
    
    // Reconstructed properties
    std::vector<float> reco_pos_;        // 3D position [x, y, z] (cm)
    std::vector<float> reco_dir_;        // Direction [dx, dy, dz]
    float total_charge_;                 // Sum of ADC integrals
    int size_;                           // Number of TPs
    
    // Classification
    int label_;                          // 77=main track, others=blips
    float supernova_tp_fraction_;        // Purity: MARLEY TPs / total TPs
    
    // Truth (for validation)
    std::vector<float> true_pos_;
    std::vector<float> true_dir_;
    float true_energy_;
    int true_interaction_;               // 0=CC, 1=ES
};
```

**Key Methods**:
- `get_tps()`, `get_reco_pos()`, `get_reco_dir()`
- `get_total_charge()`, `get_size()`
- `set_label(int)` - Mark as main track (77) or blip
- `get_supernova_tp_fraction()` - Signal purity metric

#### TrueParticle (`TrueParticle.h`)
Monte Carlo truth for validation.

```cpp
class TrueParticle {
    int pdg_code_;                    // Particle type (11=e-, 13=mu-, etc.)
    float energy_;                    // Initial kinetic energy (MeV)
    float momentum_[3];               // [px, py, pz] (GeV/c)
    float position_[3];               // Vertex [x, y, z] (cm)
    float direction_[3];              // Unit vector [dx, dy, dz]
    int interaction_type_;            // 0=CC, 1=ES
};
```

### 2. Clustering Algorithms (`src/clustering/`)

#### Main Clustering Function
Located in `cluster_to_root_libs.h`:

```cpp
std::vector<Cluster> cluster_maker(
    std::vector<std::vector<double>>& all_tps,
    int tick_limit = 3,          // Time proximity (ticks)
    int channel_limit = 2,       // Channel proximity
    int min_tps_to_cluster = 2,  // Minimum size
    int tot_cut = 0,             // Time-over-threshold filter
    int adc_integral_cut = 0     // Energy threshold
);
```

**Algorithm Steps**:
1. **Sorting**: TPs sorted by (channel, time) for efficient neighbor search
2. **Seeding**: Each TP starts as a potential cluster seed
3. **Growing**: 
   ```cpp
   for each TP in sorted order:
       if TP already clustered: continue
       create new cluster with TP as seed
       for each subsequent TP:
           if |Δchannel| <= channel_limit AND 
              |Δtime| <= tick_limit AND
              not already clustered:
               add TP to cluster
   ```
4. **Filtering**: Remove clusters with `size < min_tps_to_cluster`
5. **Post-processing**: Calculate reco_pos (centroid), total_charge

**Periodic Boundary Conditions** (`channel_condition_with_pbc`):
- Handles wrap-around at channel boundaries
- Relevant for modular detector geometry

#### Main Track Identification

```cpp
std::vector<Cluster> filter_main_tracks(std::vector<Cluster>& clusters);
```

Criteria for main track:
- `size >= threshold` (typically 20-50 TPs)
- High `total_charge` (strong signal)
- Extended spatial structure (not a point-like blip)

Identified main tracks get `label = 77` for easy filtering.

### 3. Backtracking (`src/backtracking/`)

#### Position Calculation (`Backtracking.h`)

```cpp
std::vector<float> calculate_position(
    int channel,
    int time_start,
    int view,        // 0=U, 1=V, 2=X
    float drift_velocity = 0.16  // cm/tick
);
```

**Geometry** (from `parameters/geometry.json`):
- **Channel spacing**: 0.479 cm
- **Drift velocity**: 0.0805 cm/tick (default), 0.16 cm/tick (alternative)
- **Plane angles**: U=+35.7°, V=-35.7°, X=0° (horizontal)

**Position Calculation**:
1. **Drift coordinate**: `x = time_start * drift_velocity`
2. **Transverse coordinates**: 
   - X-plane: `y = channel * pitch`, `z` from geometry
   - U/V-planes: Solve 2-plane intersection using trigonometry

Functions:
- `eval_y_knowing_z_U_plane(channel, z)` - U-plane transverse position
- `eval_y_knowing_z_V_plane(channel, z)` - V-plane transverse position

#### Truth Embedding

When backtracking TPStreams:
1. Read truth tree (`ana_tree`) with `TrueParticle` branches
2. For each TP, find nearest true energy deposition
3. Assign truth label: "MARLEY" (supernova signal) or "BKG" (noise/other)
4. Embed: `true_pos`, `true_energy`, `true_pdg`, `file_number`

### 4. 3-Plane Matching (`src/planesMatching/`)

#### Pentagon Algorithm

**Purpose**: Combine clusters from U, V, X planes into 3D objects.

**Matching Criteria** (from `MATCHING_CRITERIA_AND_HANDLING.md`):
1. **Geometric consistency**: 
   - Reconstructed 3D positions from U+V, U+X, V+X must agree within tolerance
   - Pentagon vertices (5 possible 2-plane combinations) form a compact structure
2. **Time window**: Clusters must overlap in drift time
3. **Charge consistency**: Total charge across planes should be consistent

**Implementation** (`PentagonMatching.h`):
```cpp
std::vector<MatchedCluster> match_clusters_pentagon(
    std::vector<Cluster>& u_clusters,
    std::vector<Cluster>& v_clusters,
    std::vector<Cluster>& x_clusters,
    float max_distance = 5.0  // cm tolerance
);
```

**Output**: `MatchedCluster` containing references to clusters from all 3 planes with reconstructed 3D position and direction.

### 5. Volume Analysis (`python/app/create_volumes.py`, `python/ana/analyze_volumes.py`)

#### Volume Creation

**Purpose**: Build 1m × 1m detector volumes around main tracks for ML training.

**Workflow**:
1. Load clusters from ROOT file
2. Identify main tracks (`label == 77`)
3. For each main track:
   - Define volume: `[x-50cm, x+50cm] × [y-50cm, y+50cm]`
   - Find all clusters within volume
   - Create 2D projection image (channel vs. time)
   - Save metadata: cluster count, MARLEY fraction, distances

**Distance Metrics**:
```python
# Geometry constants
CHANNEL_PITCH_CM = 0.479
DRIFT_VELOCITY_CM_PER_TICK = 0.0805

# Distance in (channel, time) space
def cluster_distance(cluster1, cluster2):
    Δch = cluster2.channel_mean - cluster1.channel_mean
    Δt = cluster2.time_mean - cluster1.time_mean
    return sqrt((Δch * CHANNEL_PITCH_CM)**2 + 
                (Δt * DRIFT_VELOCITY_CM_PER_TICK)**2)
```

**Output**: `.npz` files with:
- `image`: 2D array (channel × time)
- `metadata`: dict with cluster properties, MARLEY distances

#### Volume Analysis

**Purpose**: Generate statistical reports and visualizations.

**Metrics** (from `analyze_volumes.py`):
- **Composition**: Pure MARLEY, pure background, mixed volumes
- **MARLEY fraction**: Percentage of signal clusters per volume
- **Energy distribution**: Particle energies (MeV)
- **Momentum distribution**: Main track momentum (GeV/c)
- **Distance statistics**:
  - Average distance of MARLEY clusters from main track
  - Maximum distance per volume

**Output**: Multi-page PDF with histograms, scatter plots, and summary tables.

---

## Data Flow

### Complete Pipeline Example (CC Interaction)

```
INPUT: prodmarley_nue_flat_cc_..._tpstream.root
  ├─ TPs (all): ~50k TPs per 10ms readout window
  ├─ Truth tree: particle IDs, energies, positions
  └─ Geometry: channel maps, detector coordinates

STEP 1: Backtracking (backtrack_tpstream)
  ├─ Read TPStream + truth tree
  ├─ Match TPs to true energy depositions
  ├─ Filter: Keep only MARLEY signal TPs
  └─ Output: prodmarley_..._tps.root (~5k TPs)

STEP 2: Background Addition (add_backgrounds)
  ├─ Load signal TPs
  ├─ Overlay detector noise TPs (from background samples)
  ├─ Maintain truth labels
  └─ Output: tps_bg/prodmarley_..._tps_bg.root (~15k TPs)

STEP 3: Clustering (make_clusters)
  ├─ Load TPs with backgrounds
  ├─ Apply filters: ToT >= 2, energy >= 2 MeV
  ├─ Group TPs: tick_limit=3, channel_limit=2
  ├─ Identify main tracks (label=77)
  ├─ Calculate: reco_pos, total_charge, supernova_tp_fraction
  └─ Output: clusters_.../prodmarley_..._clusters.root
      (~200 clusters: 1 main track + 50 MARLEY blips + 150 BKG blips)

STEP 4a: 3-Plane Matching (match_clusters)
  ├─ Load clusters from U, V, X planes
  ├─ Pentagon matching algorithm
  └─ Output: matched_clusters_.../prodmarley_..._matched.root

STEP 4b: Volume Creation (create_volumes.py)
  ├─ Load clusters
  ├─ Find main track
  ├─ Create 1m × 1m volume
  ├─ Compute MARLEY distances
  ├─ Generate 2D image
  └─ Output: volume_images_.../event_12345.npz

STEP 5: Analysis (analyze_volumes.py)
  ├─ Load all .npz files
  ├─ Aggregate statistics
  ├─ Generate plots
  └─ Output: reports/volume_analysis_..._e2p0.pdf
```

---

## Module Reference

### C++ Applications (`src/app/`)

| Application | Source File | Purpose | Key Inputs | Outputs |
|------------|-------------|---------|------------|---------|
| `make_clusters` | `make_clusters.cpp` | Create clusters from TPs | TPs ROOT, JSON config | Clusters ROOT |
| `backtrack_tpstream` | `backtrack_tpstream.cpp` | Add truth, extract signal | TPStream ROOT | TPs ROOT |
| `add_backgrounds` | `add_backgrounds.cpp` | Overlay noise | Signal TPs, BKG TPs | Mixed TPs |
| `match_clusters` | `match_clusters.cpp` | 3-plane matching | 3× Clusters ROOT | Matched ROOT |
| `analyze_clusters` | `analyze_clusters.cpp` | Generate reports | Clusters ROOT | PDF report |
| `display` | `display.cpp` | Interactive viewer | TPs/Clusters ROOT | GUI window |
| `extract_calibration` | `extract_calibration.cpp` | ADC calibration | TPs ROOT | Calibration file |
| `diagnose_timing` | `diagnose_timing.cpp` | Timing analysis | Clusters ROOT | Timing stats |
| `split_by_apa` | `split_by_apa.cpp` | Separate APAs | TPs ROOT | Multiple TPs ROOT |

### Python Scripts (`python/`)

#### Applications (`python/app/`)
- `create_volumes.py`: Volume image generation (see above)
- `cluster_analyse.py`: Detailed cluster visualization and fitting
- `clusters_to_dataset.py`: ML dataset preparation
- `plot_cluster_variables.py`: Histogram generation

#### Analysis (`python/ana/`)
- `analyze_volumes.py`: Volume statistics and PDF reports

#### Utilities (`python/`)
- `cluster.py`: `Cluster` class and ROOT file reading
- `utils.py`: Distance calculations, channel maps
- `image_creator.py`: 2D image generation
- `dataset_creator.py`: NPY dataset creation

### Bash Scripts (`scripts/`)

Each script wraps a C++ app with:
1. Argument parsing (`-j` for JSON, `-m` for max files, etc.)
2. Automatic compilation (`cmake`, `make`)
3. Execution with timing
4. Error handling

**Naming Convention**: `<app_name>.sh` (e.g., `make_clusters.sh` wraps `make_clusters`)

**Sequence Scripts**:
- `sequence.sh`: Run full pipeline (backtrack → add BG → cluster → analyze)

---

## Configuration System

### JSON Structure

All apps use JSON for configuration. Standard format:

```json
{
  "// SECTION 1: Input/Output": "",
  "tpstream_folder": "/path/to/data/prod_cc/",
  "output_prefix": "cc_valid",
  
  "// SECTION 2: File Control": "",
  "skip_files": 0,
  "max_files": -1,
  
  "// SECTION 3: Clustering Parameters": "",
  "tick_limit": 3,
  "channel_limit": 2,
  "min_tps_to_cluster": 2,
  "tot_cut": 2,
  "energy_cut": 2.0,
  
  "// SECTION 4: Processing Options": "",
  "backtrack": true,
  "add_background": true,
  "background_folder": "/path/to/bkg/",
  "plane": 2,
  
  "// SECTION 5: Analysis Options": "",
  "main_track_threshold": 30,
  "volume_size_cm": 100.0
}
```

### Auto-Generated Paths

The system automatically creates output folders from `tpstream_folder`:

**Naming Convention**: `<type>_<prefix>_<conditions>/`

**Components**:
- `<type>`: `tps_bg`, `clusters`, `matched_clusters`, `volume_images`, etc.
- `<prefix>`: From `output_prefix` (e.g., `cc_valid_bg`)
- `<conditions>`: `tick{N}_ch{N}_min{N}_tot{N}_e{X}` (energy: `.` → `p`)

**Example**:
```
tpstream_folder: /data/prod_cc/
prefix: cc_valid_bg
parameters: tick=3, ch=2, min=2, tot=2, energy=2.0

Generated paths:
  /data/prod_cc/tps_bg/
  /data/prod_cc/clusters_cc_valid_bg_tick3_ch2_min2_tot2_e2p0/
  /data/prod_cc/volume_images_cc_valid_bg_tick3_ch2_min2_tot2_e2p0/
  reports/volume_analysis_cc_valid_bg_tick3_ch2_min2_tot2_e2p0.pdf
```

This ensures:
- **Traceability**: Conditions encoded in path
- **No overwrites**: Different parameters → different folders
- **Easy comparison**: Side-by-side testing

---

## Testing Framework

### Test Organization (`tests/`)

```
tests/
├── test_clustering.sh        # Unit test: clustering algorithm
├── test_backtracking.sh       # Unit test: truth embedding
├── test_matching.sh           # Unit test: 3-plane matching
├── integration_test.sh        # Full pipeline test
└── data/                      # Small test datasets
    ├── test_tpstream_cc.root
    └── test_tpstream_es.root
```

### Running Tests

```bash
cd tests

# Unit test: clustering
./test_clustering.sh
# Expected: ~10 clusters from 500 TPs

# Integration test: full pipeline
./integration_test.sh
# Expected: Complete workflow in <5 minutes
```

### Validation Metrics

Tests check:
1. **Cluster count**: Expected range for known input
2. **Purity**: MARLEY fraction should match known signal level
3. **Position accuracy**: Reco vs. true position within tolerance
4. **Performance**: Runtime within acceptable bounds

---

## Performance Considerations

### Benchmarking Results (November 2025)

**Hardware**: Intel Xeon, 6 cores, 16 GB RAM

**50-file CC dataset** (49 processed, ~2500 events):

| Stage | Time | Memory | Output Size | Notes |
|-------|------|--------|-------------|-------|
| Backtracking | ~5 min | 420 MB | 250 MB | I/O bound |
| Background Add | ~3 min | 400 MB | 350 MB | Fast overlay |
| Clustering (TOT=2) | 14:07 | 430 MB | 88 MB | **Recommended** |
| Clustering (TOT=3) | ~12:45 | 420 MB | 135 MB | Fastest |
| Clustering (TOT=1) | 19:19 | 420 MB | 95 MB | Slowest (avoid) |
| Volume Creation | ~2 min | 300 MB | 50 MB | Python, parallel-friendly |
| Analysis | <1 min | 200 MB | 5 MB PDF | Fast aggregation |

**Recommendations**:
- **TOT filter**: Use TOT=2 for best balance (speed vs. storage)
- **Parallel processing**: Run on multiple files simultaneously (no shared state)
- **Memory scaling**: ~8-10 MB per file; 16 GB → ~1500 files max

### Optimization Tips

1. **File chunking**: Process in batches of 50-100 files
2. **Plane selection**: Single-plane analysis (X) is 3× faster than 3-plane
3. **Python bottlenecks**: Use `uproot` instead of PyROOT for faster I/O
4. **Cluster size limits**: `max_tps_per_cluster` prevents runaway growth

---

## Development Workflow

### Adding a New Application

1. **Create source file**: `src/app/my_new_app.cpp`

```cpp
#include "CmdLineParser.h"
#include "Logger.h"
#include <nlohmann/json.hpp>

int main(int argc, char* argv[]) {
    CmdLineParser clp;
    clp.addOption("json", {"-j", "--json"}, "Config JSON");
    clp.parseCmdLine(argc, argv);
    
    std::string json_path = clp.getOptionVal<std::string>("json");
    // ... read JSON, process data ...
    
    return 0;
}
```

2. **Add to CMake**: Edit `src/app/CMakeLists.txt`

```cmake
add_executable( my_new_app ${CMAKE_CURRENT_SOURCE_DIR}/my_new_app.cpp )
target_link_libraries( my_new_app clustersLibs objectsLibs globalLib )
install( TARGETS my_new_app DESTINATION bin )
```

3. **Create bash wrapper**: `scripts/my_new_app.sh`

```bash
#!/bin/bash
REPO_HOME=$(git rev-parse --show-toplevel)
source ${REPO_HOME}/scripts/init.sh

# Parse arguments
INPUT_JSON=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        -j|--json) INPUT_JSON="$2"; shift 2 ;;
        *) shift ;;
    esac
done

# Compile and run
cd ${REPO_HOME}/build/
cmake .. && make -j $(nproc)
./src/app/my_new_app -j $INPUT_JSON
```

4. **Test**: 
```bash
chmod +x scripts/my_new_app.sh
./scripts/my_new_app.sh -j json/my_config.json
```

### Adding a Python Tool

1. **Create script**: `python/app/my_tool.py`

```python
import argparse
import json
import sys
sys.path.append('../python/')
from cluster import read_root_file_to_clusters

def main(args):
    with open(args.json) as f:
        config = json.load(f)
    
    clusters = read_root_file_to_clusters(config['input_file'])
    # ... process ...

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-j', '--json', required=True)
    args = parser.parse_args()
    main(args)
```

2. **Create wrapper**: `scripts/my_tool.sh`

```bash
#!/bin/bash
REPO_HOME=$(git rev-parse --show-toplevel)
cd ${REPO_HOME}/python/app/
python my_tool.py "$@"
```

### Code Style

**C++**:
- Indent: 4 spaces (no tabs)
- Braces: K&R style (`{` on same line for functions, control flow)
- Naming: `snake_case` for functions/variables, `PascalCase` for classes
- Comments: Doxygen-style for public APIs

**Python**:
- Follow PEP 8
- Type hints encouraged for function signatures
- Docstrings for all public functions

### Git Workflow

```bash
# Create feature branch
git checkout -b username/feature-name

# Make changes, test thoroughly
git add src/app/my_new_app.cpp
git commit -m "Add my_new_app for <purpose>"

# Update documentation
git add docs/code-structure.md
git commit -m "Document my_new_app in code-structure.md"

# Push and create PR
git push origin username/feature-name
# Open PR on GitHub, request review
```

---

## Advanced Topics

### Custom Clustering Algorithms

Implement in `src/clustering/inc/cluster_to_root_libs.h`:

```cpp
std::vector<Cluster> my_custom_clustering(
    std::vector<std::vector<double>>& tps,
    /* custom parameters */
) {
    std::vector<Cluster> clusters;
    
    // Your algorithm here
    // Must return std::vector<Cluster>
    
    return clusters;
}
```

Then call from app:
```cpp
clusters = my_custom_clustering(tps, param1, param2);
write_clusters_to_root(clusters, output_path);
```

### Extending Truth Information

Add new truth fields to `TrueParticle` or `TriggerPrimitive`:

1. Edit `src/objects/inc/TrueParticle.h`
2. Add getter/setter methods
3. Update ROOT I/O in `src/objects/src/TrueParticle.cpp`
4. Modify backtracking app to populate new fields

### Performance Profiling

```bash
# Compile with debug symbols
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j $(nproc)

# Profile with gprof
./src/app/make_clusters -j config.json
gprof ./src/app/make_clusters gmon.out > profile.txt

# Analyze hotspots in profile.txt
```

---

## Troubleshooting

### Common Issues

1. **"Could not find ROOT"**: Ensure ROOT is sourced (`source /path/to/root/bin/thisroot.sh`)
2. **"nlohmann/json.hpp not found"**: Install JSON library (`apt install nlohmann-json3-dev` or download header)
3. **Python import errors**: Check `sys.path` includes `../python/` or set `PYTHONPATH`
4. **Memory overflow**: Reduce `max_files` or increase system RAM
5. **Empty clusters**: Check `tot_cut` and `energy_cut` not too aggressive

### Debug Logging

Enable verbose mode:
```bash
./scripts/make_clusters.sh -j config.json -v
```

Or set debug flag in JSON:
```json
{
  "debug": true,
  "verbose": true
}
```

---

## Further Reading

- **[API_REFERENCE.md](API_REFERENCE.md)**: Detailed function signatures
- **[CONFIGURATION.md](CONFIGURATION.md)**: Complete JSON parameter reference
- **[TOT_CHOICE.md](TOT_CHOICE.md)**: Performance optimization study
- **[MATCHING_CRITERIA_AND_HANDLING.md](MATCHING_CRITERIA_AND_HANDLING.md)**: 3-plane matching details
- **[VOLUME_IMAGES.md](VOLUME_IMAGES.md)**: Volume analysis methodology

---

**Document Version**: 1.0  
**Maintained by**: DUNE SN Online Pointing Group  
**Questions?**: Open an issue on GitHub or consult team documentation
