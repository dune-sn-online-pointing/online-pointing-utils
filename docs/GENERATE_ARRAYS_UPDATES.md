# generate_cluster_arrays.py - Updates Summary

## Changes Made

### 1. JSON Configuration Support for skip_files and max_files

**Before:**
- `--skip-files` and `--max-files` were CLI-only with hardcoded defaults
- `skip_files` defaulted to 0
- No way to configure these in JSON

**After:**
- Reads `skip_files` and `max_files` from JSON configuration if not provided via CLI
- CLI arguments override JSON values
- Matches the pattern used in other C++ tools (make_clusters, analyze_clusters, etc.)

### 2. Override Flag for Batch File Management

**New Feature: `--override` / `-f` flag**

The script now intelligently skips input files if their corresponding output batch files already exist:

**Logic:**
1. Scans output directory for existing `clusters_plane*_batch*.npz` files
2. Determines the highest batch number for each plane (U, V, X)
3. Skips input files that would generate already-existing batches
4. Uses heuristic: 1 input file ≈ 1 batch per plane
5. Can be overridden with `-f` or `--override` flag to force reprocessing

**Benefits:**
- Avoids redundant processing when re-running the script
- Allows incremental processing of large datasets
- Matches the behavior of other tools in the pipeline (make_clusters.cpp pattern)

### 3. Enhanced Progress Messages

**Before:**
- Simple file counting messages
- No indication of where parameters came from

**After:**
- Shows whether skip_files/max_files came from CLI or JSON
- Reports when files are skipped due to existing batches
- Clear message when nothing needs processing
- Instructions to use `--override` to force reprocessing

## Usage Examples

### Basic Usage with JSON (reads skip/max from JSON)
```bash
./scripts/generate_cluster_images.sh -j json/es_valid.json
```

If `json/es_valid.json` contains:
```json
{
  "skip_files": 0,
  "max_files": 100,
  ...
}
```
These values will be used automatically.

### Override JSON with CLI Arguments
```bash
# Override max_files from JSON
python3 scripts/generate_cluster_arrays.py --json json/es_valid.json --max-files 50

# Override skip_files from JSON  
python3 scripts/generate_cluster_arrays.py --json json/es_valid.json --skip-files 10
```

### Force Reprocessing (Ignore Existing Batches)
```bash
# Bash wrapper
./scripts/generate_cluster_images.sh -j json/es_valid.json --override

# Direct Python call
python3 scripts/generate_cluster_arrays.py --json json/es_valid.json -f
```

### Incremental Processing Workflow

**First run** (processes files 0-99):
```bash
./scripts/generate_cluster_images.sh -j json/es_valid.json
# Creates: clusters_planeX_batch0000.npz, batch0001.npz, etc.
```

**Second run** (automatically skips already-processed files):
```bash
./scripts/generate_cluster_images.sh -j json/es_valid.json
# Output: "Found existing batch files (up to batch 5)"
#         "Skipping first 6 input file(s) to avoid regenerating existing batches"
# Only processes new files, creating batch0006.npz, batch0007.npz, etc.
```

**Force reprocessing** (regenerates everything):
```bash
./scripts/generate_cluster_images.sh -j json/es_valid.json -f
# Regenerates all batches from scratch
```

## Implementation Details

### JSON Configuration Fields

The script now reads these fields from JSON:
```json
{
  "skip_files": 0,     // Skip first N cluster ROOT files
  "max_files": 100,    // Process at most N cluster ROOT files
  ...
}
```

Both fields are optional and default to 0 and null (unlimited) respectively.

### Batch File Detection Logic

```python
# For each plane (U, V, X), find existing batch files
existing_batches = {}
for plane in ['U', 'V', 'X']:
    plane_batches = sorted(output_path.glob(f"clusters_plane{plane}_batch*.npz"))
    if plane_batches:
        # Extract batch numbers and find maximum
        batch_nums = [extract_batch_number(f) for f in plane_batches]
        max_batch = max(batch_nums)
        existing_batches[plane] = max_batch + 1  # Number of batches

# Skip input files corresponding to existing batches
if existing_batches and not args.override:
    max_existing_batches = max(existing_batches.values())
    files_to_skip = max_existing_batches  # Heuristic: 1 file per batch
    cluster_files = cluster_files[files_to_skip:]
```

### Priority Order

1. **CLI arguments** (highest priority)
   - `--skip-files 10` overrides JSON
   - `--max-files 50` overrides JSON
   - `--override` forces reprocessing

2. **JSON configuration**
   - Used if CLI argument not provided
   - `"skip_files": 5` in JSON
   - `"max_files": 100` in JSON

3. **Existing batch detection**
   - Automatic skip based on existing files
   - Only if `--override` not used

4. **Defaults**
   - skip_files: 0
   - max_files: None (unlimited)

## Bash Script Changes

### New Arguments

```bash
-f|--override [true|false]    Force reprocessing (optional true/false, defaults to true)
```

### Updated Help

```bash
./scripts/generate_cluster_images.sh -h

Usage: ... [-f|--override] ...
Options:
  ...
  -f|--override               Force reprocessing even if output batch files already exist
  ...

Example:
  ./scripts/generate_cluster_images.sh -j json/es_valid.json -f  # Force reprocess
```

## Error Handling

### No Files to Process
```
All output batch files already exist (found 10 batches)
Nothing to process. Use --override/-f to force reprocessing
```
Script exits with code 0 (not an error, just nothing to do).

### Missing JSON Fields
If JSON doesn't contain `skip_files` or `max_files`:
- Uses defaults: skip_files=0, max_files=None
- No error, graceful fallback

### CLI Override Messages
```
Skipping first 5 files (from CLI)
Processing at most 100 files (from JSON)
```
Clearly indicates source of each parameter.

## Compatibility

- **Backward compatible**: All existing scripts and workflows continue to work
- **JSON optional**: Can still use pure CLI arguments
- **Override optional**: Default behavior is smart skip, but can be disabled

## Testing

### Test Cases

1. **Default behavior** (no existing files):
   ```bash
   ./scripts/generate_cluster_images.sh -j json/test.json
   # Processes all files according to JSON config
   ```

2. **Incremental processing**:
   ```bash
   # Run 1: creates batch0000-batch0009
   ./scripts/generate_cluster_images.sh -j json/test.json
   
   # Run 2: skips first 10 files, continues from batch0010
   ./scripts/generate_cluster_images.sh -j json/test.json
   ```

3. **Force reprocessing**:
   ```bash
   ./scripts/generate_cluster_images.sh -j json/test.json -f
   # Regenerates all batches from scratch
   ```

4. **CLI overrides**:
   ```bash
   # JSON has max_files=100, override to 50
   python3 scripts/generate_cluster_arrays.py \
     --json json/test.json \
     --max-files 50
   ```

## Files Modified

1. **scripts/generate_cluster_arrays.py**
   - Added `--override` / `-f` argument
   - Changed `--skip-files` to optional (no default)
   - Changed `--max-files` to optional (no default)
   - Added JSON reading for skip_files and max_files
   - Added batch file detection logic
   - Enhanced progress messages

2. **scripts/generate_cluster_images.sh**
   - Added `--override` / `-f` argument handling
   - Updated help message
   - Pass override flag to Python script

## Notes

- The heuristic "1 file = 1 batch" may not always be accurate if files have very different numbers of clusters
- Conservative approach: if unsure, use `--override` to force reprocessing
- Existing batch detection looks at all planes (U, V, X) and uses the maximum batch count
