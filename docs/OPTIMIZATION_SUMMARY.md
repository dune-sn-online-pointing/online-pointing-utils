# add_backgrounds Performance Optimization

## Problem
The `add_backgrounds` step was taking too long, primarily due to slow file discovery on EOS.

### Original Performance (from job 17837035.192)
- Finding 4520 signal files: **~150 seconds** (2.5 minutes)
- Finding 346 background files: **~141 seconds** (2.4 minutes)
- Total startup time before processing: **~5 minutes**

## Solution
Optimized the `find_input_files()` function in `src/lib/utils.cpp` to use the system `find` command instead of C++ `std::filesystem::directory_iterator`.

### Key Changes
1. Replaced `std::filesystem::directory_iterator` with `popen()` + system `find` command
2. Added fallback to original method if `find` command fails
3. Pattern matching optimized with glob patterns in find command

### Code Modified
- **File**: `src/lib/utils.cpp`
- **Function**: `find_input_files()` - Priority 2 section (pattern_folder handling)
- **Lines**: ~499-527

## Results

### Optimized Performance (tested 2025-10-30)
- Finding 4520 signal files: **~11 seconds** (down from 150s)
- Finding 346 background files: **<1 second** (down from 141s)
- Total startup time: **~12 seconds**

### Speedup
- Signal file finding: **13.6x faster**
- Background file finding: **>140x faster**
- Overall startup: **~25x faster**

## Testing
Run the test script to verify performance:
```bash
cd /afs/cern.ch/work/e/evilla/private/dune/online-pointing-utils
./test_fast_find.sh
```

## Compilation
The optimized code has been compiled and is ready to use:
```bash
./scripts/compile.sh -p /afs/cern.ch/work/e/evilla/private/dune/online-pointing-utils
```

## Usage
The optimized executable is at:
```
./build/src/app/add_backgrounds
```

Use it exactly as before - no changes to command-line interface or behavior, only performance improvement.

## Backup
Original version saved as: `src/lib/utils_optimized.cpp` (actually this is a copy before changes, misnomer)

## Implementation Details

### Why is `find` faster than `std::filesystem`?
1. **Native EOS integration**: The `find` command is optimized for the underlying filesystem
2. **Reduced syscalls**: `find` batches operations more efficiently
3. **No repeated stat() calls**: C++ iterator calls stat() for each file, `find` optimizes this
4. **Better caching**: System utilities leverage kernel-level directory caching

### Safety Features
- Fallback to original method if `popen()` fails
- All original pattern matching logic preserved
- Error handling maintained
- Verbose/debug logging retained

## Date
Optimization implemented: October 30, 2025
