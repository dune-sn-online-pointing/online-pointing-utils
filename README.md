# Online-Pointing-Utils

[![DUNE](https://img.shields.io/badge/DUNE-Supernova%20Pointing-blue)](https://www.dunescience.org/)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Python](https://img.shields.io/badge/Python-3.x-blue.svg)](https://www.python.org/)

A comprehensive toolkit for the DUNE Supernova Online Pointing project, providing end-to-end processing of Trigger Primitives (TPs) from raw data through clustering, matching, and analysis for supernova neutrino event reconstruction.

## Overview

This repository contains tools for:
- **Backtracking**: Converting raw TPStreams to signal-only TPs with Monte Carlo truth
- **Clustering**: Grouping TPs into clusters using spatial-temporal proximity algorithms
- **3-Plane Matching**: Combining clusters from U, V, and X detector planes for 3D reconstruction
- **Volume Analysis**: Creating and analyzing detector volumes around main particle tracks
- **Background Addition**: Realistic detector simulation with noise overlay
- **Performance Studies**: Benchmarking and optimization tools for parameter tuning

## Quick Start

### Prerequisites
- **ROOT** (v6.24 or later)
- **CMake** (v3.14 or later)
- **GCC** (v11.4 or later, C++17 support required)
- **Python 3.x** with numpy, matplotlib, scipy
- **nlohmann/json** (included via system install)

### Installation

```bash
# Clone the repository
git clone https://github.com/dune-sn-online-pointing/online-pointing-utils.git
cd online-pointing-utils

# Build the project
cd build
cmake ..
make -j $(nproc)
```

Executables will be in `build/src/app/`. Most workflows use the bash scripts in `scripts/` which handle compilation automatically.

### Basic Workflow

```bash
# 1. Backtrack TPStream files (add truth info, extract signal)
./scripts/backtrack_tpstream.sh -j json/backtrack_cc.json

# 2. Add detector backgrounds to signal TPs
./scripts/add_backgrounds.sh -j json/add_backgrounds_cc.json

# 3. Create clusters from TPs
./scripts/make_clusters.sh -j json/cc_valid.json

# 4. Analyze cluster properties
./scripts/analyze_clusters.sh -j json/analyze_clusters_cc.json

# 5. Create volume images for ML training (Python)
python python/app/create_volumes.py -j json/create_volumes_cc.json

# 6. Analyze volumes and generate reports (Python)
python python/ana/analyze_volumes.py -j json/analyze_volumes_cc.json
```

All operations are configured via JSON files in the `json/` directory.

## Key Features

### Intelligent Clustering
- **Spatial-temporal grouping** with configurable tick and channel limits
- **Time-over-Threshold (ToT) filtering** for noise reduction
- **Energy-based cuts** to remove low-energy backgrounds
- **Main track identification** for primary particle reconstruction

### 3-Plane Matching
- Combines clusters from U, V, and X wire planes
- Pentagon-based geometric matching algorithm
- Truth-validated matching for Monte Carlo studies

### Volume-Based Analysis
- 1m × 1m detector volumes around main tracks
- MARLEY (supernova signal) cluster identification
- Spatial distance analysis from primary vertices
- Multi-page PDF reports with comprehensive statistics

### Performance Benchmarking
- TOT filter optimization (detailed 50-file studies)
- Time vs. storage trade-off analysis
- Cluster purity and efficiency metrics

## Repository Structure

```
online-pointing-utils/
├── src/                      # C++ source code
│   ├── app/                  # Main applications
│   ├── lib/                  # Core libraries
│   ├── clustering/           # Clustering algorithms
│   ├── backtracking/         # Truth matching
│   ├── objects/              # Data structures (Cluster, TP, etc.)
│   └── planesMatching/       # 3-plane reconstruction
├── python/                   # Python tools
│   ├── app/                  # Volume creation, visualization
│   └── ana/                  # Analysis scripts
├── scripts/                  # Bash workflow scripts
├── json/                     # Configuration files
├── parameters/               # Detector geometry & constants
├── docs/                     # Documentation
├── reports/                  # Analysis reports and benchmarks
└── tests/                    # Test scripts and data

See docs/code-structure.md for detailed architecture.
```

## Documentation

- **[Code Structure](docs/code-structure.md)**: Detailed architecture and workflow guide
- **[API Reference](docs/API_REFERENCE.md)**: Core classes and functions
- **[Configuration Guide](docs/CONFIGURATION.md)**: JSON parameter reference
- **[TOT Filter Study](docs/TOT_CHOICE.md)**: Performance optimization results
- **[Volume Images](docs/VOLUME_IMAGES.md)**: Volume analysis methodology

## Configuration

All workflows use JSON configuration files. Key parameters:

```json
{
  "tick_limit": 3,              // Time window for clustering (ticks)
  "channel_limit": 2,           // Channel window for clustering
  "min_tps_to_cluster": 2,      // Minimum TPs to form a cluster
  "tot_cut": 2,                 // Time-over-threshold filter (recommended: 2 or 3)
  "energy_cut": 2.0,            // Energy threshold (MeV)
  "tpstream_folder": "/path/to/data/prod_cc/",
  "max_files": 50               // Process limit (-1 for all)
}
```

**Auto-generated folder structure**: Outputs are organized as subfolders of `tpstream_folder` with condition-encoded names (e.g., `clusters_cc_valid_bg_tick3_ch2_min2_tot2_e2p0/`).

## Key Applications

| Application | Purpose | Documentation |
|------------|---------|---------------|
| `make_clusters` | Create clusters from TPs | [CONFIGURATION.md](docs/CONFIGURATION.md) |
| `backtrack_tpstream` | Add truth info to TPStreams | [API_REFERENCE.md](docs/API_REFERENCE.md) |
| `add_backgrounds` | Overlay detector noise | [BACKGROUND_ENERGY_FIX.md](docs/BACKGROUND_ENERGY_FIX.md) |
| `match_clusters` | 3-plane cluster matching | [MATCHING_CRITERIA_AND_HANDLING.md](docs/MATCHING_CRITERIA_AND_HANDLING.md) |
| `analyze_clusters` | Generate cluster reports | [CLUSTER_MAIN_TRACK_SUMMARY.md](docs/CLUSTER_MAIN_TRACK_SUMMARY.md) |
| `create_volumes.py` | Build volume images (Python) | [VOLUME_IMAGES.md](docs/VOLUME_IMAGES.md) |
| `analyze_volumes.py` | Volume statistics & PDFs (Python) | [VOLUME_IMAGES.md](docs/VOLUME_IMAGES.md) |
| `display` | Interactive visualization | [display_script_usage.md](docs/display_script_usage.md) |

## Recent Benchmarks (November 2025)

**TOT Filter Comparison (50-file CC dataset)**:
- **TOT=2**: Best balance (14:07 runtime, 88 MB output, 42.4% MARLEY fraction) ✅ **RECOMMENDED**
- **TOT=3**: Fastest (≈12:45 estimated, 135 MB output, highest purity)
- **TOT=1**: Slowest (19:19 runtime) ❌ **NOT RECOMMENDED**

See `reports/tot_filter_comparison_complete.txt` for full analysis.

## Python Environment

Python scripts require:
```bash
pip install numpy matplotlib scipy uproot
```

Optional for advanced features:
```bash
pip install pandas seaborn tqdm
```

## Testing

```bash
# Run clustering tests
cd tests
./test_clustering.sh

# Validate backtracking
./test_backtracking.sh
```

## Development Notes

- **Backtracking parameters** are in `parameters/`. Standard TPs have no suffix; test TPs use suffixes (e.g., `_bktr20.root`).
- **Thread safety**: Most applications are single-threaded; use parallel file processing for large datasets.
- **Memory**: Typical RAM usage 400-450 MB per process; scales with file size.

## Contributing

1. Create feature branches from `main`
2. Follow existing code style (C++17, 4-space indentation)
3. Update documentation in `docs/`
4. Test with both ES and CC samples before PR

## License

Part of the DUNE DAQ Application Framework. See COPYING for licensing details.

## Contact

DUNE Supernova Online Pointing Group
- Repository: [dune-sn-online-pointing/online-pointing-utils](https://github.com/dune-sn-online-pointing/online-pointing-utils)
- DUNE Collaboration: [dunescience.org](https://www.dunescience.org/)

---

**Last Updated**: November 2025  
**Branch**: evilla/direct-tp-simide-matching
