# Volume Image Generation for Channel Tagging

Clean Python implementation for creating 1m × 1m volume images centered on main track clusters.

## Overview

This pipeline creates volume images suitable for channel tagging ML tasks. Unlike individual cluster images, these volumes:
- Are centered on main track clusters (using `is_main_cluster` flag)
- Have fixed physical size: **1 meter × 1 meter**
- Contain all clusters within the volume (not just the main track)
- Process only **plane X** (collection plane)
- Separate detectors and TPC sides automatically

## Files Created

### Scripts
- **`scripts/create_volumes.py`** - Main Python script for volume generation
- **`scripts/create_volumes.sh`** - Bash wrapper with JSON interface
- **`scripts/display_volumes.py`** - Visualization utility

### Configuration
- **`json/create_volumes_test.json`** - Example configuration

## Usage

### 1. Create Volumes

```bash
./scripts/create_volumes.sh -j json/create_volumes_test.json -v
```

JSON configuration:
```json
{
  "clusters_folder": "./clusters_clusters_tick3_ch2_min2_tot1_e0p0/",
  "volumes_folder": "/path/to/output/volumes_planeX/",
  "plane": "X",
  "max_files": 10
}
```

### 2. Display Volumes

```bash
# Display grid of multiple volumes
python3 scripts/display_volumes.py data/volumes_planeX/*.npz --grid

# Display single volume
python3 scripts/display_volumes.py data/volumes_planeX/clusters_0_planeX_volume000.npz

# Print detailed information
python3 scripts/display_volumes.py data/volumes_planeX/*.npz --info

# Save figure
python3 scripts/display_volumes.py data/volumes_planeX/*.npz --grid --save output.png
```

## Output Format

### NPZ Files

Each volume is saved as a compressed numpy file (`.npz`) containing:

**Image**: `(n_channels, n_time_bins)` array
- Dimensions: ~208 × 1242 pixels
- Physical size: 100 cm × 100 cm
- Data type: float32
- Values: Occupancy (number of TPs at each pixel)

**Metadata**: Dictionary with:
- `event`: Event number
- `plane`: Wire plane ('X', 'U', or 'V')
- `interaction_type`: True interaction type (e.g., 'CCQE', 'ES')
- `neutrino_energy`: True neutrino energy (MeV)
- `main_track_momentum`: Main track momentum magnitude (MeV/c)
- `main_track_momentum_x/y/z`: Momentum components
- `n_clusters_in_volume`: Total clusters in volume
- `n_marley_clusters`: Number of MARLEY (signal) clusters
- `n_non_marley_clusters`: Number of non-MARLEY (background) clusters
- `center_channel`: Center channel number
- `center_time_tpc`: Center time in TPC ticks
- `volume_size_cm`: Volume size in cm
- `image_shape`: Image array dimensions

## Geometry Conversion

From detector units to physical units:

```
Physical size = 100 cm × 100 cm

Collection plane (X):
- Wire pitch: 0.479 cm/channel
- Channels in 1m: 100 / 0.479 ≈ 209 channels

Time dimension:
- Time tick: 0.0805 cm/tick  
- TPC ticks in 1m: 100 / 0.0805 ≈ 1242 ticks
```

## Example Output

```
Found 1 cluster files
[1/1] Processing: clusters_0.root
  Loaded 38 clusters from plane X
  Found 10 main track clusters
  Created volume 0: 5 clusters (5 marley, 0 non-marley), event 1, UNKNOWN
  Created volume 1: 5 clusters (5 marley, 0 non-marley), event 2, UNKNOWN
  ...
  Created 10 volume images

DONE: Created 10 total volume images
```

## Implementation Details

### Volume Selection Algorithm

1. Load all clusters for specified plane
2. Filter for main track clusters (`is_main_cluster == True`)
3. For each main track:
   - Calculate center position (mean of TP channels and times)
   - Define 1m × 1m bounding box around center
   - Collect ALL clusters whose centers fall within bounds
   - Create image from all TPs in collected clusters

### Key Features

- **No true position used** - Only detector coordinates (channel, time)
- **Automatic plane separation** - Reads correct tree from ROOT file
- **Event separation** - Each event's main tracks processed independently
- **Metadata preservation** - All truth info from main track saved
- **Compressed output** - Uses `np.savez_compressed` for efficient storage

## Testing

Successfully tested on fresh cluster file:
- Input: `clusters_0.root` (38 clusters, 10 main tracks)
- Output: 10 volume NPZ files
- Image size: 208 × 1242 pixels each
- Metadata: Complete and accurate

## Notes

- Currently processes plane X only (can be extended to U/V)
- Uses simple occupancy (count of TPs) - can be enhanced with ADC values
- Interaction type shows "UNKNOWN" for test data (normal for CC data without ES labels)
- Momentum components are 0 for test data (depends on upstream backtracking)
