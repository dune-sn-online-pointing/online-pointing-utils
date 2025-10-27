# Cluster Array Metadata Format

## Overview

Each generated cluster is stored in batched `.npz` files containing both image arrays and metadata for neural network training. Each batch contains up to 1000 clusters.

## File Structure

Clusters are stored in batched files:
```
clusters_planeX_batch0000.npz    # Batch 0: clusters 0-999 (images: 1000×128×16, metadata: 1000×12)
clusters_planeX_batch0001.npz    # Batch 1: clusters 1000-1999
...
```

## File Sizes (per batch of 1000 clusters)

- **Image data**: 8,192,000 bytes (1000×128×16 float32 array)
- **Metadata data**: 48,000 bytes (1000×12 float32 values)
- **NPZ overhead**: ~3,200 bytes (numpy container format)
- **Total**: ~8.24 MB per batch
- **Overhead**: Only 0.04% for NPZ container!

## NPZ File Contents

Each `.npz` file contains two named arrays:

```python
data = np.load('clusters_planeU_batch0000.npz')
images = data['images']      # N×128×16 float32 array (N ≤ 1000)
metadata = data['metadata']  # N×12 float32 array
```

## Metadata Format

The metadata is stored as a compact numpy array of 12 float32 values per cluster:

```python
metadata = np.array([
    is_marley,            # [0]: 1.0 if Marley, 0.0 otherwise
    is_main_track,        # [1]: 1.0 if main track, 0.0 otherwise
    true_pos_x,           # [2]: True X position [cm]
    true_pos_y,           # [3]: True Y position [cm]
    true_pos_z,           # [4]: True Z position [cm]
    true_dir_x,           # [5]: True direction X (normalized)
    true_dir_y,           # [6]: True direction Y (normalized)
    true_dir_z,           # [7]: True direction Z (normalized)
    true_nu_energy,       # [8]: True neutrino energy [MeV]
    true_particle_energy, # [9]: True particle energy [MeV]
    plane_id,             # [10]: Plane ID (0=U, 1=V, 2=X)
    is_es_interaction     # [11]: 1.0 if ES interaction, 0.0 if CC
], dtype=np.float32)
```

**Note**: Boolean values are stored as float32 (0.0/1.0) for efficiency. Neural networks expect float inputs, and storing all metadata as float32 avoids mixed dtypes and simplifies data loading.

## Loading Metadata

```python
import numpy as np

# Load batch file
data = np.load('clusters_planeU_batch0000.npz')

# Extract images and metadata
images = data['images']    # Shape: (N, 128, 16) where N ≤ 1000
metadata = data['metadata']  # Shape: (N, 12)

# Process each cluster in the batch
for i in range(len(images)):
    img = images[i]
    meta = metadata[i]
    
    # Decode metadata
    is_marley = bool(meta[0])
    is_main_track = bool(meta[1])
    true_pos = meta[2:5]  # [x, y, z]
    true_dir = meta[5:8]  # [dx, dy, dz]
    true_nu_energy = meta[8]
    true_particle_energy = meta[9]
    plane_id = int(meta[10])
    is_es_interaction = bool(meta[11])
    
    plane_map = {0: 'U', 1: 'V', 2: 'X'}
    plane_name = plane_map[plane_id]
    interaction_type = "ES" if is_es_interaction else "CC"
    
    print(f"Cluster {i}: {plane_name} plane, {interaction_type} interaction")

data.close()  # Close file when done
```

## Helper Function

Use the provided `load_batched_cluster_data.py` utility:

```python
from scripts.load_batched_cluster_data import load_batch, decode_metadata

# Load a batch
images, metadata = load_batch('clusters_planeU_batch0000.npz')

# Decode metadata for a specific cluster
meta_dict = decode_metadata(metadata[0])

# Access metadata
print(f"Marley: {meta_dict['is_marley']}")
print(f"Main track: {meta_dict['is_main_track']}")
print(f"Position: {meta_dict['true_pos']}")
print(f"Energy: {meta_dict['true_particle_energy']} MeV")
print(f"Interaction: {meta_dict['interaction_type']}")  # "CC" or "ES"
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

### is_es_interaction (bool)
- `True`: Elastic Scattering (ES) interaction - neutrino scatters elastically (primary SN signal)
- `False`: Charged Current (CC) interaction - produces charged lepton
- **Source**: `true_interaction` branch from ROOT file ("ES" or "CC")
- **Note**: ES interactions are the dominant signal channel for supernova neutrino detection

## Batch Loading for Training

```python
import numpy as np
from pathlib import Path
from scripts.load_batched_cluster_data import load_dataset

# Load all clusters from a directory
images, metadata = load_dataset('data/prod_es/images_es_valid_bg_tick3_ch2_min2_tot2_e2p0')

# images shape: (N, 128, 16) - N clusters, 128 time ticks, 16 channels
# metadata shape: (N, 12) - N clusters, 12 metadata values

# Extract specific metadata fields for training
is_marley = metadata[:, 0].astype(bool)
is_main_track = metadata[:, 1].astype(bool)
true_positions = metadata[:, 2:5]
true_directions = metadata[:, 5:8]
true_nu_energies = metadata[:, 8]
true_particle_energies = metadata[:, 9]
plane_ids = metadata[:, 10].astype(int)
is_es = metadata[:, 11].astype(bool)

# Filter specific types
marley_images = images[is_marley]
es_images = images[is_es]      # ES interactions
cc_images = images[~is_es]     # CC interactions

# Combine filters
marley_es = images[is_marley & is_es]
marley_cc = images[is_marley & ~is_es]
```

## Using the Helper Script

```bash
# Show dataset statistics
python scripts/load_batched_cluster_data.py data/prod_es/images_es_valid_bg_tick3_ch2_min2_tot2_e2p0

# Get info about a specific batch file
python scripts/load_batched_cluster_data.py data/prod_es/images_es_valid_bg_tick3_ch2_min2_tot2_e2p0/clusters_planeU_batch0000.npz
```

Example output:
```
Dataset Statistics:
  Directory: data/prod_es/images_es_valid_bg_tick3_ch2_min2_tot2_e2p0
  Total batches: 3
  Total clusters: 2783
  Total size: 22.94 MB
  
  Plane distribution:
    U: 257 (9.2%)
    V: 308 (11.1%)
    X: 2218 (79.7%)
  
  Interaction types:
    ES: 2783 (100.0%)
    CC: 0 (0.0%)
  
  Marley clusters: 540 (19.4%)
  Main track clusters: 300 (10.8%)
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
