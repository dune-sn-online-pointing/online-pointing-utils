# Output Filename Prefix Feature

## Summary
Added functionality to `analyze_tps.cpp` and `analyze_clusters.cpp` to read output filename prefix from JSON configuration, matching the behavior already present in `make_clusters.cpp`.

## Changes Made

### 1. analyze_tps.cpp
**Location:** Lines 148-162 (approximately)

**Previous behavior:** Output filename was generated based on the first input file's basename
- Example: `input_file_tps.root` → `input_file_tp_analysis_report.pdf`

**New behavior:** Output filename uses prefix from JSON configuration
- Reads `outputFilename` field from JSON
- Falls back to JSON filename stem if `outputFilename` not specified
- Example with `outputFilename: "bkg"` → `bkg_tp_analysis_report.pdf`

**Code added:**
```cpp
// Determine output file prefix: JSON outputFilename > JSON filename stem
std::string file_prefix;
try {
    file_prefix = j.at("outputFilename").get<std::string>();
} catch (...) {
    std::filesystem::path json_path(json);
    file_prefix = json_path.stem().string();
}

// Generate output filename
std::string pdf_output;
if (!outFolder.empty()) {
    pdf_output = outFolder + "/" + file_prefix + "_tp_analysis_report.pdf";
} else {
    pdf_output = file_prefix + "_tp_analysis_report.pdf";
}
```

### 2. analyze_clusters.cpp
**Location:** Lines 131-139 (approximately)

**Previous behavior:** Output filename based only on input clusters file
- Example: `input_clusters.root` → `input_clusters_report.pdf`

**New behavior:** Output filename includes prefix from JSON configuration
- Reads `outputFilename` field from JSON
- Falls back to JSON filename stem if `outputFilename` not specified  
- Example with `outputFilename: "cc"` → `cc_input_clusters_report.pdf`

**Code added:**
```cpp
// Determine output file prefix: JSON outputFilename > JSON filename stem
std::string file_prefix;
try {
    file_prefix = j.at("outputFilename").get<std::string>();
} catch (...) {
    std::filesystem::path json_path(json);
    file_prefix = json_path.stem().string();
}
```

**Modified line:**
```cpp
// Before:
std::string pdf = outDir + "/" + base + "_report.pdf";

// After:
std::string pdf = outDir + "/" + file_prefix + "_" + base + "_report.pdf";
```

### 3. JSON Configuration Files

Updated the following JSON files to include `outputFilename` field:

#### analyze_tps JSON files:
- `json/analyze_tps/bkg_analyze.json` → `"outputFilename": "bkg"`
- `json/analyze_tps/cc_analyze.json` → `"outputFilename": "cc"`
- `json/analyze_tps/es_analyze.json` → `"outputFilename": "es"`

#### analyze_clusters JSON files:
- `json/analyze_clusters/prod_cc_analyze_clusters.json` → `"outputFilename": "cc"`
- `json/analyze_clusters/prod_es_analyze_clusters.json` → `"outputFilename": "es"`

## Benefits

1. **Consistency:** All three applications (make_clusters, analyze_tps, analyze_clusters) now use the same pattern for output naming
2. **Clarity:** Output files clearly indicate their data type (bkg, cc, es, etc.)
3. **Batch Processing:** Easier to distinguish outputs when processing multiple datasets
4. **Backward Compatible:** Falls back to JSON filename if `outputFilename` not specified

## Example Usage

### analyze_tps
```json
{
    "inputFolder": "/path/to/tps/",
    "outputFolder": "/path/to/output/",
    "outputFilename": "bkg",
    "tot_cut": 0
}
```
Output: `bkg_tp_analysis_report.pdf`

### analyze_clusters
```json
{
    "inputListFile": "data/clusters_files.txt",
    "output_folder": "output/analysis",
    "outputFilename": "cc"
}
```
For input file `event123_clusters.root`, output: `cc_event123_clusters_report.pdf`

## Testing

Both applications compiled successfully with the changes:
```bash
cd build
make -j4
```

Both `analyze_tps` and `analyze_clusters` targets built without errors.
