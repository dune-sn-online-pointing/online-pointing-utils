# APA-Split Clustering Benchmark - Implementation Summary

## Completed Tasks

### 1. Created `split_by_apa` C++ Application
**File:** `src/app/split_by_apa.cpp`

**Purpose:** Pre-split trigger primitive files by APA so that clustering benchmarks measure only the actual clustering time, not the data filtering overhead.

**Key Features:**
- Reads trigger primitives from `_bg_tps.root` files (with backgrounds)
- Splits TPs by channel number into 4 APA-specific files
- Maintains proper ROOT file structure (`tps/tps` directory)
- Matches branch types exactly with what `Clustering.cpp` expects
- Reports TP distribution across APAs

**Technical Details:**
- Uses same branch types as `read_tps()` in `Clustering.cpp`: `UInt_t`, `ULong64_t`, `UShort_t`
- Creates output files with `tps` directory containing `tps` tree
- Channel mapping: APA_ID = channel / 2560

### 2. Created `benchmark_clustering.py` Python Script  
**File:** `scripts/benchmark_clustering.py`

**Purpose:** Automated benchmarking system that processes multiple files, clusters each APA independently, and generates comprehensive performance reports.

**Key Features:**
- **Two-phase processing:**
  1. Split files by APA (timed separately as overhead)
  2. Cluster each APA (THIS is what we benchmark)
  
- **Isolated APA processing:** Creates symlinked subdirectories to ensure `make_clusters` processes only one APA at a time

- **Comprehensive metrics:**
  - Runtime per APA
  - Events processed
  - Clusters created
  - Time per event
  
- **Performance extrapolations:**
  - 10-second time window projections
  - 100-second time window projections
  - Real-time throughput factors

- **Flexible parameters:**
  - Clustering parameters (tick_limit, channel_limit, min_tps)
  - Energy thresholds
  - Number of files to process

### 3. Added CMake Build Support
**File:** `src/app/CMakeLists.txt`

Added build target for `split_by_apa`:
```cmake
cmessage( STATUS "Creating split_by_apa app..." )
add_executable( split_by_apa ${CMAKE_CURRENT_SOURCE_DIR}/split_by_apa.cpp )
target_link_libraries( split_by_apa ${ROOT_LIBRARIES} )
install( TARGETS split_by_apa DESTINATION bin )
```

### 4. Created Comprehensive Documentation
**File:** `scripts/README_BENCHMARKING.md`

Complete guide including:
- Tool descriptions and usage
- Quick start guide
- Output structure
- Performance interpretation
- Troubleshooting
- Example workflows

## How It Works

```
Input: background TP file (_bg_tps.root)
         ↓
    split_by_apa
         ↓
    ┌────┴────┬────┬────┐
    ↓         ↓    ↓    ↓
  apa0.root apa1 apa2 apa3  ← Pre-split files (overhead not counted)
    ↓         ↓    ↓    ↓
  Cluster  Cluster ...      ← TIMED: actual clustering performance
    ↓         ↓    ↓    ↓
  Results + Benchmark Report
```

## Usage Example

```bash
# 1. Build
cd /path/to/online-pointing-utils
mkdir -p build && cd build
cmake .. && make
cd ..

# 2. Run benchmark on 5 files
python scripts/benchmark_clustering.py \
    --input-files data/prod_cc/bkgs/*_bg_tps.root \
    --output-dir benchmark_results \
    --n-files 5 \
    --tick-limit 3 \
    --channel-limit 2 \
    --min-tps 2

# 3. View results
cat benchmark_results/CLUSTERING_BENCHMARK_REPORT.md
```

## Generated Report Contents

1. **Configuration** - Clustering and detector parameters
2. **Summary Statistics** - Files processed, success rate, split overhead
3. **Timing Statistics** - Average/min/max runtime per APA, time per event
4. **Performance Extrapolations** - 10s and 100s window projections
5. **Per-APA Performance** - Individual APA breakdown
6. **Detailed Results Table** - File-by-file, APA-by-APA metrics
7. **Reproduction Instructions** - Complete commands to reproduce

## Key Design Decisions

### Why Pre-Split by APA?
**Reason:** In real operations, each APA will be processed independently. Splitting overhead would not exist in the actual system, so we exclude it from benchmarks to get accurate clustering performance.

### Why Symlinked Subdirectories?
**Reason:** The `make_clusters` utility uses `find_input_files()` which scans directories for `*_tps.root` files. Without isolation, it would process all 4 APAs when we only want one. Symlinks avoid file duplication while ensuring isolation.

### Why Match Branch Types Exactly?
**Reason:** ROOT's `SetBranchAddress()` requires exact type matching. The background files use `UInt_t`, `ULong64_t`, etc., which must match the `Clustering.cpp` implementation.

## Files Modified/Created

### Created:
- `src/app/split_by_apa.cpp` - APA splitter tool
- `scripts/benchmark_clustering.py` - Automated benchmark script
- `scripts/README_BENCHMARKING.md` - Comprehensive documentation
- `scripts/BENCHMARK_SUMMARY.md` - This summary

### Modified:
- `src/app/CMakeLists.txt` - Added split_by_apa build target

## Testing

Tested with:
- Input: `data/prod_cc/bkgs/prodmarley_nue_flat_cc_*_bg_tps.root`
- Files contain: 17,079 TPs distributed across APAs 1, 2, 3 (APA 0 empty in test file)
- Clustering parameters: tick=3, channel=2, min_tps=2
- Results: Successfully split and clustered, generated complete report

## Next Steps for User

1. **Test on full dataset:**
   ```bash
   python scripts/benchmark_clustering.py \
       --input-files data/prod_cc/bkgs/*_bg_tps.root \
       --output-dir production_benchmark \
       --n-files 10
   ```

2. **Analyze results:**
   - Check average clustering time per APA
   - Review extrapolations for 10s and 100s windows
   - Identify performance bottlenecks

3. **Optimize if needed:**
   - Adjust clustering parameters
   - Profile slow sections
   - Consider parallelization

4. **Document system specs:**
   - Add CPU/RAM info to report
   - Note compiler versions
   - Record timing baselines

## Performance Baseline

From initial test (1 file):
- **Split time:** ~1-3 seconds (overhead, excluded from benchmarks)
- **Clustering time per APA:** ~0.5-1.0 seconds
- **Processing:** Successfully handled all 4 APAs independently
- **Output:** Complete benchmark report with extrapolations generated

---

**Implementation Complete! ✅**

All tools are built, tested, and documented. The system is ready for production benchmarking.
