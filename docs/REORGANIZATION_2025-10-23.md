# Repository Reorganization Summary

**Date**: October 23, 2025  
**Purpose**: Clean up repository structure and improve organization

## Overview

The repository has been reorganized to improve clarity and maintainability. All files have been moved to appropriate directories based on their purpose.

## Changes Made

### 1. Documentation (`docs/`)
**Moved from root to docs/:**
- `BACKGROUND_ENERGY_FIX.md`
- `CHANGES_SUMMARY.md`
- `ENERGY_CUT_STUDY_RESULTS.md`
- `FOLDER_STRUCTURE_UPDATE.md`
- `IMPLEMENTATION_SUMMARY.md`
- `SIGNAL_TO_BACKGROUND_ANALYSIS.md`
- `TOT_CHOICE.md`
- `TRUTH_EMBEDDING_REFACTOR.md`

### 2. Analysis Results (`results/`)
**Moved from root to results/:**
- `CLUSTERING_ANALYSIS.txt`
- `CORRECTED_RESULTS_SUMMARY.txt`
- `notes.txt`
- `energy_cut_scan_data.txt`
- `tot2_data.txt`
- `es_valid_bg_tick3_ch2_min2_tot1_e0_clusters_0_combined_analysis.pdf`
- `es_valid_bg_tick3_ch2_min2_tot1_e0_clusters_0_combined_analysis_VALID.pdf`

### 3. Plots (`plots/`)
**Moved from root to plots/:**
- `energy_cut_analysis_cc.png`
- `energy_cut_analysis_es.png`
- `energy_cut_analysis_signal_to_background.png`

### 4. Scripts (`scripts/`)
**Moved from root to scripts/:**
- `run_energy_cut_scan.sh`

### 5. Python Reorganization (`python/`)

#### New Structure:
```
python/
├── lib/              # Shared libraries
│   ├── cluster.py
│   └── utils.py
├── ana/              # Analysis scripts
│   ├── analyze_clusters.py
│   ├── analyze_margin_scan.py
│   ├── cluster_analyse.py
│   ├── plot_energy_cut_study.py
│   └── extract_energy_cut_data.py
└── clusters/         # Clustering and visualization
    ├── cluster_display.py
    ├── cluster_to_text.py
    ├── cluster_var_plotter.py
    ├── plot_cluster_info.py
    ├── plot_cluster_positions.py
    ├── plot_cluster_variables.py
    ├── image_creator.py
    ├── clusters_to_dataset.py
    ├── create_nn_dataset.py
    ├── dataset_creator.py
    └── dataset_mixer.py
```

**Changes:**
- Created `lib/` directory for shared modules (`cluster.py`, `utils.py`)
- Moved analysis scripts to `ana/`
- Moved clustering/visualization scripts to `clusters/`
- Updated all import statements to reference `lib/` correctly
- Moved old JSON creators and test files to `tmp/`

### 6. JSON Configurations (`json/`)
**Changes:**
- Moved `json_examples/` to `tmp/`
- Kept organized structure in `json/` with subdirectories per app
- Each subdirectory contains:
  - `example.json` with full documentation
  - Working configurations for production use

### 7. Testing (`test/`)
**New additions:**
- `run_tests.sh` - Comprehensive test script for all apps
- `README.md` - Test documentation
- `test_es_tpstream.root` - Sample ES tpstream file
- `test_bg_tps.root` - Sample background file
- `test_output/` - Directory for test outputs (gitignored)

**Test Coverage:**
- Backtracking (`backtrack_tpstream`)
- Background addition (`add_backgrounds`)
- Clustering (`make_clusters`)
- Cluster analysis (`analyze_clusters`)
- Python analysis scripts (`python/ana/`)

### 8. Temporary Files (`tmp/`)
**Moved to tmp/ for user review:**
- All `*.log` files
- All `*.C` ROOT macros
- All `*.h5` HDF5 files
- Test text files (`test*.txt`)
- Old test JSONs
- Old shell scripts (`es_valid.sh`, `test_es.sh`, `clean_local.sh`, `update_analyze_clusters.sh`)
- `json_examples/` directory
- `remove_dup_plotting.py`
- Python test files and JSON creators
- `debugCl.txt`

## Repository Structure (After Cleanup)

```
online-pointing-utils/
├── CMakeLists.txt           # Main build configuration
├── README.md                # Project documentation
├── build/                   # Build artifacts
├── cmake/                   # CMake modules
├── data/                    # Data files (ROOT files, etc.)
├── docs/                    # All documentation
├── img/                     # Images for documentation
├── json/                    # JSON configurations (organized by app)
├── output/                  # Application outputs
├── parameters/              # Parameter files
├── plots/                   # Generated plots
├── python/                  # Python code (organized)
│   ├── lib/                # Shared libraries
│   ├── ana/                # Analysis scripts
│   └── clusters/           # Clustering/visualization
├── results/                 # Analysis results and reports
├── scripts/                 # Bash scripts
├── src/                     # C++ source code
├── submodules/              # Git submodules
├── test/                    # Automated tests
│   ├── run_tests.sh        # Test runner
│   ├── README.md           # Test documentation
│   ├── test_es_tpstream.root
│   └── test_bg_tps.root
├── tests/                   # Additional test files
└── tmp/                     # Files to review/delete
```

## Python Import Updates

All Python scripts that previously imported from `cluster` or `utils` have been updated to use the new `lib/` structure:

```python
import sys
from pathlib import Path
sys.path.append(str(Path(__file__).parent.parent / 'lib'))
from cluster import *
from utils import *
```

## Testing the Changes

To verify everything still works:

```bash
# Run comprehensive test suite
cd test/
./run_tests.sh --clean

# Run energy cut analysis
cd scripts/
./run_energy_cut_scan.sh

# Python analysis
cd python/ana/
python3 plot_energy_cut_study.py
```

## Next Steps

1. **Review tmp/ directory** - Decide what to delete permanently
2. **Update README.md** - Reflect new structure
3. **Update documentation** - Fix any outdated paths in docs/
4. **Add .gitignore entries** - For test_output/, etc.

## Files Preserved in Root

Only essential files remain in root:
- `CMakeLists.txt` - Build configuration
- `README.md` - Main documentation

## Benefits

1. **Cleaner root directory** - Only build files and main README
2. **Organized Python code** - Clear separation of libraries, analysis, and visualization
3. **Centralized documentation** - All docs in one place
4. **Centralized results** - Easy to find analysis outputs
5. **Automated testing** - Quick verification after changes
6. **Better git hygiene** - Temporary files isolated in tmp/

## Compatibility Notes

- All existing JSON configurations should still work
- Python scripts updated with correct import paths
- Test suite verifies core functionality
- No changes to C++ source code structure
