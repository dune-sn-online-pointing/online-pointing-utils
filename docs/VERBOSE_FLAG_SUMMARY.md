# Verbose Flag Update Summary

## Overview
All scripts in the `scripts/` directory that run applications have been updated to support the `-v|--verbose` flag. When this flag is provided, it is passed through to the underlying executable.

## Updated Scripts

### Core Application Scripts
1. **analyze_clusters.sh** - Analyzes cluster data
   - Usage: `./scripts/analyze_clusters.sh -j <json> -v`
   
2. **analyze_tps.sh** - Analyzes trigger primitive data
   - Usage: `./scripts/analyze_tps.sh -j <json> -v`
   
3. **backtrack.sh** - Performs backtracking on TP streams
   - Usage: `./scripts/backtrack.sh -j <json> -v`
   
4. **make_clusters.sh** - Creates clusters from input data
   - Usage: `./scripts/make_clusters.sh -j <json> -v`
   
5. **extract_calibration.sh** - Extracts calibration data
   - Usage: `./scripts/extract_calibration.sh <config.json> [outdir] [tot_cut] -v`
   
6. **create_volume_clusters.sh** - Creates volume clusters
   - Usage: `./scripts/create_volume_clusters.sh --json-settings <json> -v`

### Plotting Scripts
7. **plot_avg_times.sh** - Plots average timing data
   - Usage: `./scripts/plot_avg_times.sh -i <input_file> -v`
   
8. **plot_avg_times_run.sh** - Runs average timing plots
   - Usage: `./scripts/plot_avg_times_run.sh -v [input_file]`

### Batch Processing Scripts
9. **margin_scan.sh** - Runs margin parameter scans
   - Usage: `./scripts/margin_scan.sh -v`
   - Note: Passes verbose flag to both backtrack and analyze_tps executables

### Display Script (Already Had Verbose)
10. **display.sh** - Already had verbose mode support
    - Usage: `./scripts/display.sh -i <clusters_file> -v`

## Implementation Details

Each script was updated with:
1. **Help message** - Added `-v|--verbose` option documentation
2. **Variable initialization** - Added `verbose=false` (or `VERBOSE=false`)
3. **Argument parsing** - Added case statement to handle `-v|--verbose` flag
4. **Command execution** - Appends `-v` to the executable command when flag is set

## Usage Examples

```bash
# Analyze clusters with verbose output
./scripts/analyze_clusters.sh -j settings.json -v

# Backtrack with verbose mode
./scripts/backtrack.sh -j backtrack_config.json -v --no-compile

# Run margin scan with verbose output
./scripts/margin_scan.sh -v
```

## Notes

- The verbose flag is optional for all scripts
- The flag is passed directly to the underlying C++ executables
- Scripts that call multiple executables (like margin_scan.sh) pass the flag to all child processes
- The display.sh script already had verbose support before this update

---
Last updated: October 15, 2025
