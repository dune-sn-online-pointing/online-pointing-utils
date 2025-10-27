# Folder Structure and CLI Updates - Summary

## Overview
This update introduces improved folder naming conventions, clearer CLI options, and new JSON configuration options for better control over input/output organization.

## Changes Made

### 1. New JSON Configuration Options

#### Folder Naming (replaces generic `outputFolder`)
- **`tpstream_folder`**: Output folder for tpstream files (backtracking step)
- **`tps_folder`**: Output folder for TPs files (backtracking output)
- **`tps_bg_folder`**: Output folder for TPs with backgrounds (add_backgrounds output)
- **`clusters_folder`**: Base folder for clusters (clustering creates subfolder inside)
- **`reports_folder`**: Output folder for reports/plots (analysis output)

**Legacy Support**: `outputFolder` still works for backward compatibility

#### New Processing Options
- **`skip_files`**: Number of files to skip after sorting input file list (default: 0)
  ```json
  "skip_files": 5  // Skip first 5 files in sorted list
  ```

#### Clustering Folder Structure
- **`clusters_folder_prefix`**: Prefix for cluster subfolder name (default: "clusters")
  
**New Folder Pattern**: 
```
clusters_folder/
  └── clusters_<prefix>_tick<X>_ch<Y>_min<Z>_tot<T>_e<E>/
      ├── clusters_0.root
      ├── clusters_1.root
      ├── clusters_2.root
      └── ...
```

**Example**:
```json
{
  "clusters_folder": "/path/to/output/",
  "clusters_folder_prefix": "es_valid",
  "tick_limit": 3,
  "channel_limit": 2,
  "min_tps_to_cluster": 2,
  "tot_cut": 1,
  "energy_cut": 0
}
```

Creates: `/path/to/output/clusters_es_valid_tick3_ch2_min2_tot1_e0/clusters_N.root`

### 2. Updated Bash Script CLI

#### New Convention
- **`-i | --input-file <path>`**: Input file or list (overrides JSON)
- **`-o | --output-folder <path>`**: Output folder (overrides JSON)
- **`-f | --override`**: Force reprocessing (was `-o`)

#### Updated Scripts
All main scripts now follow this convention:
- `scripts/backtrack.sh`
- `scripts/add_backgrounds.sh`
- `scripts/make_clusters.sh`
- `scripts/analyze_clusters.sh`
- `scripts/analyze_tps.sh`
- `scripts/sequence.sh`

#### Example Usage
```bash
# Old way (still works with updated flags)
./scripts/backtrack.sh -j json/backtrack/es_valid.json -o output/ --override

# New way (clearer)
./scripts/backtrack.sh -j json/backtrack/es_valid.json -o output/ -f

# With input override
./scripts/make_clusters.sh -j json/make_clusters/es_valid.json -i input_list.txt -o output/ -f
```

### 3. Code Changes

#### `src/lib/utils.cpp`
- Added `skip_files` support to both `find_input_files()` functions
- Skips specified number of files after sorting

#### `src/app/backtrack_tpstream.cpp`
- Added priority: CLI > `tps_folder` > `outputFolder`
- Maintains backward compatibility

#### `src/app/add_backgrounds.cpp`
- Uses `tps_bg_folder` if available, falls back to `outputFolder`

#### `src/app/make_clusters.cpp`
- Implements new folder structure: creates subfolder with clustering parameters
- Uses `clusters_folder` and `clusters_folder_prefix`
- Generates `clusters_N.root` files inside subfolder

#### `src/app/analyze_tps.cpp` & `src/app/analyze_clusters.cpp`
- Added priority: CLI > `reports_folder` > `outputFolder`

### 4. Updated JSON Files

#### Sample Files Updated
- `json/backtrack/es_valid.json`: Added `tps_folder`, `skip_files`
- `json/make_clusters/es_valid.json`: Added `clusters_folder`, `clusters_folder_prefix`, `skip_files`

#### Template Created
- `json_examples/NEW_FOLDER_STRUCTURE_TEMPLATE.json`: Comprehensive template showing all new options

## Migration Guide

### For Existing Workflows

#### Option 1: Keep Current Structure (No Changes Needed)
Your existing JSONs with `outputFolder` will continue to work as before.

#### Option 2: Migrate to New Structure (Recommended)
Update your JSONs to use specific folder names:

**Before**:
```json
{
  "outputFolder": "/path/to/output/"
}
```

**After**:
```json
{
  "tps_folder": "/path/to/tps/",
  "tps_bg_folder": "/path/to/tps_bg/",
  "clusters_folder": "/path/to/clusters/",
  "clusters_folder_prefix": "dataset_name",
  "reports_folder": "/path/to/reports/",
  "skip_files": 0
}
```

### For Bash Scripts

Update any custom scripts calling our tools:

**Before**:
```bash
./scripts/backtrack.sh -j config.json --override
```

**After**:
```bash
./scripts/backtrack.sh -j config.json -f
# or with output override
./scripts/backtrack.sh -j config.json -o /custom/output/ -f
```

## Benefits

1. **Better Organization**: Separate folders for each processing stage
2. **Clearer CLI**: `-i` for input, `-o` for output, `-f` for force
3. **Structured Clusters**: Parameters encoded in folder name, clean numbered files
4. **Flexible Processing**: `skip_files` allows resuming/testing on subsets
5. **Backward Compatible**: Old JSONs and commands still work

## Testing

All code compiled successfully. To test:

```bash
# Compile
source scripts/compile.sh

# Test with existing JSONs (backward compatibility)
./scripts/sequence.sh -d es_valid --all

# Test with new structure
# 1. Update your JSON with new fields
# 2. Run: ./scripts/sequence.sh -d your_dataset --all -f
```

## Notes

- All apps automatically create output directories if they don't exist
- `skip_files` is applied after file list is sorted (alphabetically)
- Clustering creates subfolder automatically based on parameters
- Legacy `outputFolder` and `outputFilename` still respected for backward compatibility
