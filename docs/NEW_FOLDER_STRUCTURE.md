# New Folder Structure Logic

## Proposed Structure

Given `tpstream_folder` as the base directory (e.g., `/path/to/prod_es/`):

```
prod_es/                                    # tpstream_folder (input)
├── *_tpstream.root                         # Raw TP streams
├── *_tps.root                             # Backtracked TPs (signal only)
├── tps_bg/                                # Background-added TPs
│   └── *_tps_bg.root
├── clusters_<prefix>_<conditions>/        # Clusters output
│   └── *_clusters.root                    # Keeps original basename
├── cluster_images_<prefix>_<conditions>/  # Cluster visualizations
│   └── *.png
├── volume_images_<prefix>_<conditions>/   # Volume images
│   └── *.png
└── reports/                               # Analysis reports
    └── *.pdf
```

## Logic Priority

For each folder type:
1. **Explicit JSON parameter** (if provided) → use as-is
2. **CLI override** (if provided) → use as-is
3. **Auto-generate** from `tpstream_folder` → create subfolder

## Parameters

### Input Parameters
- `tpstream_folder`: Base directory containing raw tpstream files

### Output Parameters (with auto-generation)
- `sig_folder`: Signal TPs → defaults to `{tpstream_folder}/` (same level)
- `tps_bg_folder` or `tps_folder`: Background-added TPs → defaults to `{tpstream_folder}/tps_bg/`
- `clusters_folder`: Clusters → defaults to `{tpstream_folder}/clusters_{prefix}_{conditions}/`
- `cluster_images_folder`: Cluster images → defaults to `{tpstream_folder}/cluster_images_{prefix}_{conditions}/`
- `volume_images_folder` or `volumes_folder`: Volume images → defaults to `{tpstream_folder}/volume_images_{prefix}_{conditions}/`
- `reports_folder`: Reports → defaults to `{tpstream_folder}/reports/`

### Conditions String
Built from clustering parameters:
- `tick{tick_limit}_ch{channel_limit}_min{min_tps}_tot{tot_cut}_e{energy_cut}`

Example: `tick3_ch2_min2_tot1_e0p0`

## Implementation Plan

1. Create `getOutputFolder()` utility function with parameters:
   - `base_folder` (tpstream_folder)
   - `folder_type` ("tps_bg", "clusters", "cluster_images", "volume_images", "reports")
   - `json_config` (to check for explicit paths)
   - `prefix` (clusters_folder_prefix)
   - `conditions` (clustering parameters)

2. Update each tool:
   - `backtrack_tpstream` → outputs to `sig_folder` (same as tpstream_folder)
   - `add_backgrounds` → outputs to `tps_bg_folder`
   - `make_clusters` → outputs to `clusters_folder`
   - `generate_cluster_arrays` → outputs to `cluster_images_folder`
   - `create_volumes` → outputs to `volume_images_folder`
   - Analysis tools → output to `reports_folder`

## Filename Convention

### Current Issue
Clusters currently use progressive numbering that doesn't match input files.

### New Convention
Keep the original basename from input, change suffix:
- Input: `prodmarley_nue_flat_es_...._tps_bg.root`
- Output: `prodmarley_nue_flat_es_...._clusters.root`

This maintains traceability: cluster file → TPs file → tpstream file.

## Migration Notes

- Backward compatible: if explicit paths provided in JSON, use those
- Auto-generation only when paths not specified
- Makes sense for both local and lxplus/EOS usage
