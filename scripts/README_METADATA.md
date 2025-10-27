# Cluster Array Metadata Format

## Overview

Each generated cluster is stored as a single `.npz` file containing both the image array and metadata for neural network training.

## File Structure

For each cluster, one file is generated:
```
cluster_planeX_0123.npz          # Contains both image (16×128) and metadata (11 values)
```

## File Sizes

- **Image data**: 8,192 bytes (16×128 float32 array)
- **Metadata data**: 44 bytes (11 float32 values)
- **NPZ overhead**: ~512 bytes (numpy container format)
- **Total**: ~8.5 KB per cluster
- **Overhead**: Only 5.9% for NPZ container!

## NPZ File Contents

Each `.npz` file contains two named arrays:

```python
data = np.load('cluster_planeU_0123.npz')
image = data['image']      # 128×16 float32 array
metadata = data['metadata']  # 11 float32 values
```

## Metadata Format

The metadata is stored as a compact numpy array of 11 float32 values:

```python
metadata = np.array([
    is_marley,           # [0]: 1.0 if Marley, 0.0 otherwise
    is_main_track,       # [1]: 1.0 if main track, 0.0 otherwise
    true_pos_x,          # [2]: True X position [cm]
    true_pos_y,          # [3]: True Y position [cm]
    true_pos_z,          # [4]: True Z position [cm]
    true_dir_x,          # [5]: True direction X (normalized)
    true_dir_y,          # [6]: True direction Y (normalized)
    true_dir_z,          # [7]: True direction Z (normalized)
    true_nu_energy,      # [8]: True neutrino energy [MeV]
    true_particle_energy,# [9]: True particle energy [MeV]
    plane_id             # [10]: Plane ID (0=U, 1=V, 2=X)
], dtype=np.float32)
```

## Loading Metadata

```python
import numpy as np

# Load cluster file
data = np.load('cluster_planeU_0123.npz')

# Extract image and metadata
img = data['image']
meta = data['metadata']

# Decode metadata
is_marley = bool(meta[0])
is_main_track = bool(meta[1])
true_pos = meta[2:5]  # [x, y, z]
true_dir = meta[5:8]  # [dx, dy, dz]
true_nu_energy = meta[8]
true_particle_energy = meta[9]
plane_id = int(meta[10])

plane_map = {0: 'U', 1: 'V', 2: 'X'}
plane_name = plane_map[plane_id]

data.close()  # Close file when done
```

## Helper Function

Use the provided `load_cluster_data.py` utility:

```python
from scripts.load_cluster_data import load_cluster

# Load cluster with decoded metadata
img, metadata_dict = load_cluster('cluster_planeU_0123.npz')

# Access metadata
print(f"Marley: {metadata_dict['is_marley']}")
print(f"Main track: {metadata_dict['is_main_track']}")
print(f"Position: {metadata_dict['true_pos']}")
print(f"Energy: {metadata_dict['true_particle_energy']} MeV")
```

## Metadata Fields Explained

### is_marley (bool)
- `True`: Cluster is from Marley generator (supernova signal)
- `False`: Cluster is from other generators (backgrounds, noise, atmospheric, etc.)
- **Criterion**: marley_tp_fraction > 0.5 (more than 50% of TPs from Marley)

### is_main_track (bool)
- `True`: This cluster represents the main particle track
- `False`: Secondary cluster or fragment
- **Source**: `is_main_cluster` branch from ROOT file

### true_pos (3D position)
- True interaction/vertex position in detector coordinates
- Units: centimeters [cm]
- Format: [x, y, z]

### true_dir (3D direction)
- True particle direction (normalized vector)
- Dimensionless
- Format: [dx, dy, dz]

### true_nu_energy (float)
- True neutrino energy
- Units: MeV
- Value: -1.0 if not available

### true_particle_energy (float)
- True energy of the particle creating this cluster
- Units: MeV

### plane_id (int)
- Detector readout plane identifier
- 0 = U plane (induction)
- 1 = V plane (induction)
- 2 = X plane (collection)

## Batch Loading for Training

```python
import numpy as np
from pathlib import Path

def load_dataset(data_dir):
    """Load all clusters with metadata for training"""
    data_dir = Path(data_dir)
    
    images = []
    metadata = []
    
    for npz_file in sorted(data_dir.glob('cluster_plane*_[0-9]*.npz')):
        data = np.load(npz_file)
        images.append(data['image'])
        metadata.append(data['metadata'])
        data.close()
    
    return np.array(images), np.array(metadata)

# Usage
images, metadata = load_dataset('output/clusters')

# Extract specific metadata fields for training
is_marley = metadata[:, 0].astype(bool)
is_main_track = metadata[:, 1].astype(bool)
true_positions = metadata[:, 2:5]
true_directions = metadata[:, 5:8]

# Filter only Marley events
marley_images = images[is_marley]
marley_positions = true_positions[is_marley]
```

## Using the Helper Script

```bash
# Show dataset statistics
python scripts/load_cluster_data.py data/prod_es/images_es_valid_bg_tick3_ch2_min2_tot1_e0

# Inspect a specific cluster
python scripts/load_cluster_data.py data/prod_es/images_es_valid_bg_tick3_ch2_min2_tot1_e0 cluster_planeU_0002.npz
```

Example output:
```
Dataset Statistics:
  Total files: 19552
  Total size: 163.12 MB
  Avg file size: 8.54 KB

  Plane distribution:
    U: 2557 (13.1%)
    V: 3108 (15.9%)
    X: 13887 (71.0%)

  Marley clusters: 540 (2.8%)
  Main track clusters: 300 (1.5%)
```

## Storage Efficiency

The `.npz` format combines both image and metadata in a single file:
- **Compact**: 8,192 bytes image + 44 bytes metadata + 512 bytes NPZ overhead = 8,748 bytes
- **Fast loading**: Single file I/O operation
- **Small overhead**: Only 5.9% container overhead
- **Clean organization**: One file per cluster (no separate metadata files)

## Why Use NPZ Format?

Advantages over separate files:
1. **Single file**: Easier to manage, copy, and organize
2. **Atomic operations**: No risk of image/metadata mismatch
3. **Named access**: `data['image']` and `data['metadata']` are self-documenting
4. **Minimal overhead**: Only ~512 bytes extra vs separate .npy files
5. **Standard format**: Well-supported by numpy and deep learning frameworks
