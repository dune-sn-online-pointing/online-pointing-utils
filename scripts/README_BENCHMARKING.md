# Clustering Performance Benchmarking

This directory contains tools for benchmarking clustering performance on APA-split background data.

## Overview

The benchmarking system measures the actual clustering time per APA, excluding the overhead of splitting data. This provides accurate performance metrics that can be extrapolated to 10-second and 100-second time windows for supernova trigger scenarios.

## Tools

### 1. `split_by_apa` (C++)

**Location:** `build/src/app/split_by_apa`

Splits trigger primitive (TP) ROOT files by APA (Anode Plane Assembly). The 1x2x2 detector has 4 APAs with 2560 channels each.

**Usage:**
```bash
./build/src/app/split_by_apa <input_file.root> <output_directory>
```

**Example:**
```bash
./build/src/app/split_by_apa \
    data/prod_cc/bkgs/prodmarley_nue_flat_cc_*_bg_tps.root \
    /tmp/split_output/
```

**Output:**
- Creates 4 files: `apa0_tps.root`, `apa1_tps.root`, `apa2_tps.root`, `apa3_tps.root`
- Each file contains TPs only from that APA's channel range
- Files maintain the `tps/tps` directory structure required by `make_clusters`

**Channel Mapping:**
- APA 0: channels 0-2559
- APA 1: channels 2560-5119  
- APA 2: channels 5120-7679
- APA 3: channels 7680-10239

### 2. `benchmark_clustering.py` (Python)

**Location:** `scripts/benchmark_clustering.py`

Automated benchmarking tool that:
1. Splits input files by APA (timed separately)
2. Runs clustering on each APA independently (THIS is what we benchmark)
3. Generates performance reports with extrapolations

**Usage:**
```bash
python scripts/benchmark_clustering.py \
    --input-files data/prod_cc/bkgs/*_bg_tps.root \
    --output-dir benchmark_results \
    --n-files 5 \
    --tick-limit 3 \
    --channel-limit 2 \
    --min-tps 2
```

**Key Arguments:**
- `-i, --input-files`: One or more background TP files (`*_bg_tps.root`)
- `-o, --output-dir`: Directory for results (default: `benchmark_results`)
- `-e, --make-clusters-exe`: Path to `make_clusters` executable (default: `./build/src/app/make_clusters`)
- `-s, --split-by-apa-exe`: Path to `split_by_apa` executable (default: `./build/src/app/split_by_apa`)
- `-n, --n-files`: Number of files to process (default: all)
- `-t, --tick-limit`: Clustering tick limit (default: 5)
- `-c, --channel-limit`: Clustering channel limit (default: 2)
- `-m, --min-tps`: Minimum TPs per cluster (default: 2)
- `--adc-total`: ADC total threshold (default: 1)
- `--adc-energy`: Energy cut in MeV (default: 0)
- `--report-name`: Output report filename (default: `CLUSTERING_BENCHMARK_REPORT.md`)

## Quick Start

### 1. Build the Tools

```bash
cd /path/to/online-pointing-utils
mkdir -p build && cd build
cmake ..
make split_by_apa make_clusters
cd ..
```

### 2. Run Benchmark on Sample Files

```bash
# Test with 1 file
python scripts/benchmark_clustering.py \
    --input-files data/prod_cc/bkgs/prodmarley_nue_flat_cc_*_bg_tps.root \
    --output-dir benchmark_test \
    --n-files 1 \
    --tick-limit 3 \
    --channel-limit 2 \
    --min-tps 2
```

### 3. View Results

```bash
cat benchmark_test/CLUSTERING_BENCHMARK_REPORT.md
```

## Output Structure

```
benchmark_results/
├── CLUSTERING_BENCHMARK_REPORT.md  # Human-readable report
├── benchmark_results.json           # Machine-readable raw data
├── split/                           # Split TP files by APA
│   ├── apa0/
│   │   └── apa0_tps.root
│   ├── apa1/
│   │   └── apa1_tps.root
│   ├── apa2/
│   │   └── apa2_tps.root
│   └── apa3/
│       └── apa3_tps.root
├── apa0/                            # APA 0 clustering results
│   ├── apa0_config.json
│   └── clusters_*/
├── apa1/                            # APA 1 clustering results
│   ├── apa1_config.json
│   └── clusters_*/
├── apa2/                            # APA 2 clustering results
│   ├── apa2_config.json
│   └── clusters_*/
└── apa3/                            # APA 3 clustering results
    ├── apa3_config.json
    └── clusters_*/
```

## Performance Report

The generated `CLUSTERING_BENCHMARK_REPORT.md` includes:

1. **Configuration Summary**
   - Clustering parameters used
   - Detector configuration (4 APAs, 2560 channels each)

2. **Timing Statistics**
   - Average/min/max runtime per APA
   - Time per event
   - APA splitting overhead (reported separately, NOT included in clustering benchmarks)

3. **Performance Extrapolations**
   - Clustering time for 10-second data window
   - Clustering time for 100-second data window
   - Throughput metrics (real-time factors)

4. **Per-APA Breakdown**
   - Individual performance for each APA
   - Variance analysis

5. **Detailed Results Table**
   - File-by-file, APA-by-APA breakdown
   - Event counts, cluster counts, timing

6. **Reproduction Instructions**
   - Complete commands to reproduce the benchmark
   - System specification checklist

## Example Workflow: Production Benchmark

```bash
# 1. Build tools
cd /path/to/online-pointing-utils
mkdir -p build && cd build
cmake .. && make
cd ..

# 2. Run benchmark on 10 background files
python scripts/benchmark_clustering.py \
    --input-files data/prod_cc/bkgs/*_bg_tps.root \
    --output-dir production_benchmark \
    --n-files 10 \
    --tick-limit 5 \
    --channel-limit 2 \
    --min-tps 2 \
    --adc-total 1 \
    --adc-energy 0

# 3. View summary
cat production_benchmark/CLUSTERING_BENCHMARK_REPORT.md

# 4. Extract specific metrics
python -c "
import json
with open('production_benchmark/benchmark_results.json') as f:
    data = json.load(f)
    runtimes = [apa['runtime'] for file in data for apa in file['apa_results']]
    print(f'Average clustering time: {sum(runtimes)/len(runtimes):.3f}s')
"
```

## Performance Considerations

### What is Measured
- **Clustering time per APA**: The actual time spent clustering TPs
- **Per-event timing**: Average time to cluster one event's worth of TPs
- **Throughput**: How fast clustering can process real detector data

### What is NOT Measured
- **APA splitting time**: Excluded from benchmarks (real system processes one APA at a time)
- **File I/O overhead**: Minimized by pre-splitting files
- **Multi-threading**: Current benchmarks are single-threaded per APA

### Extrapolation Methodology

Given:
- Average clustering time per APA per event: `T_cluster` (ms)
- Event duration in simulation: 3 ms (typical supernova trigger window)
- Time window to analyze: 10s or 100s

Calculations:
```
Events in window = Window (ms) / Event duration (ms)
Total clustering time = Events × T_cluster × N_APAs

Throughput = Window / Total clustering time  (real-time factor)
```

Example:
- T_cluster = 5 ms/event
- 10s window = 10,000 ms → 3,333 events
- Total time (1 APA) = 3,333 × 5 ms = 16.7 seconds
- Total time (4 APAs) = 16.7 × 4 = 66.8 seconds
- Throughput = 10s / 16.7s = 0.6x real-time (per APA)

## Interpreting Results

### Good Performance
- Clustering time < 1 second per APA per event
- Real-time factor > 0.1x for 10s windows
- Low variance across APAs

### Areas for Optimization
- High variance between APAs → check channel distribution
- Slow processing → profile clustering algorithm
- Memory issues → check cluster buffer management

## Troubleshooting

### Issue: "No valid input files found"
**Solution:** Check that input files have the `tps/tps` directory structure. Use `split_by_apa` to create properly formatted files.

### Issue: "All APAs showing 0 events"
**Solution:** Verify input files contain the `event` branch. Check with:
```bash
root -l file.root -e "tps->cd(); tps->Print()"
```

### Issue: Clustering too slow
**Optimization steps:**
1. Increase `adc_total` and `adc_energy` thresholds to reduce TP count
2. Adjust `tick_limit` and `channel_limit` for faster clustering
3. Use profiling tools: `valgrind --tool=callgrind ./build/src/app/make_clusters ...`

## System Requirements

- **ROOT 6.x** with C++17 support
- **CMake 3.14+**
- **Python 3.7+**
- **8GB+ RAM** (for processing large files)
- **Multi-core CPU** recommended (for parallel APA processing in future)

## Future Improvements

- [ ] Parallel APA processing (benchmark all 4 APAs simultaneously)
- [ ] GPU acceleration for clustering
- [ ] Online processing mode (benchmark on streaming data)
- [ ] Memory profiling integration
- [ ] Automated regression testing

## References

- **Clustering Algorithm:** See `src/clusters/Clustering.cpp`
- **TriggerPrimitive Format:** See `docs/TRIGGER_PRIMITIVE_FORMAT.md`
- **DUNE 1x2x2 Detector:** [DUNE Technical Design Report](https://arxiv.org/abs/2103.04797)

## Contact

For questions or issues:
- Open an issue on GitHub: [dune-sn-online-pointing/online-pointing-utils](https://github.com/dune-sn-online-pointing/online-pointing-utils)
- Contact: DUNE Supernova Online Pointing Team
