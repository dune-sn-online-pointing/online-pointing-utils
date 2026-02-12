# Folder Structure Implementation

## Overview

The pipeline now uses an **auto-generation** folder structure that organizes all outputs as subfolders of the main `tpstream_folder`.

## Structure

Given `tpstream_folder` (e.g., `/path/to/prod_es/`):

```
prod_es/                                           # tpstream_folder
├── *_tpstream.root                                # Raw TP streams (input)
├── *_tps.root                                    # Backtracked TPs (signal only)
├── tps_bg/                                        # Auto-generated
│   └── *_tps_bg.root                             # Background-added TPs
├── clusters_<prefix>_<conditions>/                # Auto-generated
│   └── *_clusters.root                           # Clusters (basename preserved)
├── matched_clusters_<prefix>_<conditions>/        # Auto-generated
│   └── *_matched.root                            # Matched clusters (3-plane)
├── cluster_images_<prefix>_<conditions>/          # Auto-generated
│   └── *.npz                                     # Cluster image arrays
├── volume_images_<prefix>_<conditions>/           # Auto-generated
│   └── *.png                                     # Volume visualization images
└── reports/                                       # Auto-generated
    └── *.pdf                                     # Analysis reports
```

## Conditions String

Format: `tick{N}_ch{N}_min{N}_tot{N}_e{X}`

Example: `tick3_ch2_min2_tot1_e0p0`

Parameters:
- `tick_limit`: Time clustering window
- `channel_limit`: Channel clustering window
- `min_tps_to_cluster`: Minimum TPs per cluster
- `tot_cut`: Time-over-threshold cut
- `energy_cut`: Energy threshold (decimal point → 'p')

## Priority Logic

For each folder type:

1. **Explicit JSON parameter** (if provided) → use as-is
2. **Auto-generate** from `tpstream_folder` → create subfolder

### JSON Parameters

| Folder Type | JSON Key | Auto-Generated Path |
|------------|----------|---------------------|
| TPs with BG | `tps_folder` or `tps_bg_folder` | `{tpstream_folder}/tps_bg/` |
| Clusters | `clusters_folder` | `{tpstream_folder}/clusters_{prefix}_{conditions}/` |
| Matched Clusters | `matched_clusters_folder` | `{tpstream_folder}/matched_clusters_{prefix}_{conditions}/` |
| Cluster Images | `cluster_images_folder` | `{tpstream_folder}/cluster_images_{prefix}_{conditions}/` |
| Volume Images | `volumes_folder` | `{tpstream_folder}/volume_images_{prefix}_{conditions}/` |
| Reports | `reports_folder` | `{tpstream_folder}/reports/` |

## Filename Convention

**Key Feature**: Basenames are preserved throughout the pipeline.

### Example Flow

1. Input: `prodmarley_nue_flat_es_..._tpstream.root`
2. Backtracking: `prodmarley_nue_flat_es_..._tps.root`
3. Add BG: `prodmarley_nue_flat_es_..._tps_bg.root`
4. Clustering: `prodmarley_nue_flat_es_..._clusters.root`
5. Matching: `prodmarley_nue_flat_es_..._matched.root`

This maintains full traceability: matched clusters → clusters → TPs → tpstream.

## Implementation

### C++ Functions

In `src/lib/utils.cpp` and `utils.h`:

```cpp
std::string getConditionsString(const nlohmann::json& j);
std::string getOutputFolder(const nlohmann::json& j, 
                           const std::string& folder_type, 
                           const std::string& json_key);
```

Usage in tools:
```cpp
// Auto-generate or use explicit path
std::string clusters_path = getOutputFolder(j, "clusters", "clusters_folder");
std::string matched_path = getOutputFolder(j, "matched_clusters", "matched_clusters_folder");
```

### Python Functions

In Python scripts:

```python
def get_conditions_string(config):
    """Build conditions string from clustering parameters"""
    # Returns: "tick3_ch2_min2_tot1_e0p0"

def get_output_folder(config, folder_type, json_key):
    """Auto-generate output folder or use explicit path"""
    # Mirrors C++ logic
```

## Backward Compatibility

✅ **Explicit paths still work**: If you provide explicit paths in JSON, they are used as-is.

✅ **CLI overrides**: Command-line arguments override JSON settings.

❌ **Old progressive numbering**: Removed - files now keep original basenames.

## Benefits

1. **Organization**: All outputs grouped under main folder
2. **Traceability**: Filename preservation tracks provenance
3. **Flexibility**: Works locally and on lxplus/EOS
4. **Clean**: No more scattered output folders
5. **Auto-magic**: Minimal configuration needed

## Examples

### Minimal JSON (auto-generation)

```json
{
    "tpstream_folder": "/data/prod_es",
    "tps_folder": "/data/prod_es",
    "clusters_folder_prefix": "es_valid",
    "tick_limit": 3,
    "channel_limit": 2,
    "min_tps_to_cluster": 2,
    "tot_cut": 1,
    "energy_cut": 0.0
}
```

Result:
- Clusters: `/data/prod_es/clusters_es_valid_tick3_ch2_min2_tot1_e0p0/`
- Matched: `/data/prod_es/matched_clusters_es_valid_tick3_ch2_min2_tot1_e0p0/`
- Images: `/data/prod_es/cluster_images_es_valid_tick3_ch2_min2_tot1_e0p0/`
- Volumes: `/data/prod_es/volume_images_es_valid_tick3_ch2_min2_tot1_e0p0/`

### Explicit Paths (override)

```json
{
    "tpstream_folder": "/data/prod_es",
    "clusters_folder": "/custom/path/my_clusters",
    "matched_clusters_folder": "/custom/path/my_matched"
}
```

Result: Uses your custom paths instead of auto-generation.

## Testing

Tested successfully with:
- Local paths: `/home/.../data/prod_es/`
- EOS paths: `/eos/user/e/evilla/dune/sn-tps/production_es/`
- Single file: `make_clusters -j config.json -f --max 1` ✅
- Batch mode: `sequence.sh -s es_valid --all` ✅
