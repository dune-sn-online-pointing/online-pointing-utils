# Code Structure (current)

Short overview of how the code is organized today. Use this as a map while browsing the tree.

## Top-level layout

- `src/app/`      – C++ executables (pipeline steps and diagnostics)
- `src/lib/`      – shared utilities: parameters, geometry, folder logic, general helpers
- `src/backtracking/` – truth/position handling
- `src/clusters/` – clustering logic and ROOT IO for clusters
- `src/planes-matching/` – 3-plane matching (Pentagon)
- `src/objects/`  – core structs/classes (`TriggerPrimitive`, `Cluster`, etc.)
- `src/io/`       – declarations for file discovery/folder helpers (implemented in `src/lib`)
- `python/`       – Python utilities (core pipeline analysis + shared modules)
- `scripts/`      – orchestration wrappers (build + run steps)
- `parameters/`   – `.dat` parameter files
- `json/`         – example JSON configs + test settings (all other JSONs are gitignored)

## Data flow (current pipeline)

1) `backtrack_tpstream` → `_tps.root` (signal-only with truth)
2) `add_backgrounds` → `tps_bg/*_tps_bg.root`
3) `make_clusters` → `clusters_<prefix>_<conds>/*_clusters.root`
4) `match_clusters` → `matched_clusters_<prefix>_<conds>/*_matched.root`
5) Optional Python products:
  - Cluster image arrays → `cluster_images_<prefix>_<conds>/cluster_plane*.npy`
  - Volume images → `volume_images_<prefix>_<conds>/*.npz`
  - Volume analysis summaries via `python/ana/analyze_volumes.py`

`scripts/sequence.sh` drives these steps and recompiles once at the start.

## Key libraries

- `src/lib/utils.*`
  - File discovery and folder auto-generation (`find_input_files`, `getOutputFolder`, basename tracking)
  - Path helpers (`getTpstreamBaseFolder`, `resolveFolderAgainstTpstream`, `ensureDirectoryExists`)
  - Parameter accessors (`GET_PARAM_*` helpers and geometry/timing accessors)
  - Branch binding helpers for ROOT (`bindBranch`, `SetBranchWithFallback`)

- `src/lib/geometry.*`
  - Geometry constants and helper functions fed by `parameters/geometry.dat`

- `src/lib/ParametersManager.*`
  - Loads `.dat` files, provides `GET_PARAM_*` macros

- `src/clusters/`
  - Cluster creation/IO, main-track tagging, truth fraction preservation

- `src/planes-matching/`
  - Pentagon matching, matched cluster structures

- `src/backtracking/`
  - Truth association and position calculations

## Event displays

- C++ display: `display` (ROOT) via `scripts/display.sh`
- Python display: `python/ana/cluster_display.py` (matplotlib) with settings in `json/display/example.json`

## Notes on Python layout

- Maintained tools: `python/app/create_volumes.py`, `python/app/generate_cluster_arrays.py`, `python/ana/analyze_volumes.py`, `python/ana/view_volume_quick.py`, `python/ana/cluster_display.py`
- Shared modules: `python/lib/cluster.py`, `python/lib/utils.py`

## Testing hook

- `test/run_all_tests.sh` runs the smoke pipeline on `json/test_settings.json` (one file, full chain).

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
- `generate_cluster_arrays.py`: Cluster image array generation (`.npy`)

#### Analysis (`python/ana/`)
- `analyze_volumes.py`: Volume statistics and PDF reports

#### Utilities (`python/`)
- `cluster.py`: `Cluster` class and ROOT file reading
- `utils.py`: Distance calculations, channel maps

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

- **[APPS.md](APPS.md)**: Overview of apps/scripts and typical workflows
- **[API_REFERENCE.md](API_REFERENCE.md)**: Detailed function signatures
- **[CONFIGURATION.md](CONFIGURATION.md)**: Complete JSON parameter reference
- **[MATCHING_CRITERIA_AND_HANDLING.md](MATCHING_CRITERIA_AND_HANDLING.md)**: 3-plane matching details
- **[VOLUME_IMAGES.md](VOLUME_IMAGES.md)**: Volume analysis methodology

---

**Document Version**: 1.0  
**Maintained by**: DUNE SN Online Pointing Group  
**Questions?**: Open an issue on GitHub or consult team documentation
