# Neural Network Dataset Creation for DUNE Clusters

This directory contains tools for creating 32x32 pixel image datasets from DUNE cluster ROOT files for neural network training.

## Tools Available

### Bash Wrappers (Recommended)
- **`scripts/create_nn_dataset.sh`** - Wrapper for dataset creation
- **`scripts/visualize_dataset.sh`** - Wrapper for dataset visualization

### Python Scripts (Direct Use)
- **`python/create_nn_dataset.py`** - Core dataset creation script
- **`python/visualize_dataset.py`** - Core visualization script

The bash wrappers provide:
- Consistent interface with other project scripts
- Automatic environment initialization
- Input validation and helpful error messages
- Clear configuration summary before execution

## Directory Organization

- `src/clustering` → **`src/clusters`** (renamed for better organization)
  - Contains C++ geometry tools for pentagon/triangle/rectangle rendering
  - Display functions in `src/ana/Display.cpp` and `src/ana/Display.h`
  
- **Python implementation** (`python/image_creator.py`)
  - Already has pentagon/triangle/rectangle drawing algorithms
  - Used by dataset creation scripts

## Dataset Creation

### Bash Wrapper (Recommended)

```bash
./scripts/create_nn_dataset.sh \
  -i <cluster_file.root> \
  -o <output.h5> \
  --channel-map APA \
  --only-collection \
  --min-tps 5 \
  --max-clusters 1000 \
  --save-samples 10
```

### Direct Python Usage

```bash
python python/create_nn_dataset.py \
  --input <cluster_file.root> \
  --output <output.h5> \
  --channel-map APA \
  --only-collection \
  --min-tps 5 \
  --max-clusters 1000 \
  --save-samples 10
```

### Parameters

- `--input`, `-i`: Input ROOT file with clusters (required)
- `--output`, `-o`: Output HDF5 file path (required)
- `--channel-map`: Detector type: "APA" (horizontal drift), "CRP" (vertical drift), or "50L"
- `--only-collection`: Use only collection plane (X) - recommended for faster processing
- `--min-tps`: Minimum TPs per cluster (default: 5)
- `--max-clusters`: Maximum clusters to process (for testing)
- `--save-samples`: Number of sample PNG images to save (default: 0)

### Output Format

The HDF5 file contains:

- **`images`**: Shape `(n_clusters, 32, 32, n_planes)`, dtype `float32`
  - Normalized to [0, 1] range
  - `n_planes = 1` if `--only-collection`, else `3` (U, V, X)
  
- **`labels`**: Shape `(n_clusters,)`, dtype `int32`
  - Cluster labels from truth information
  
- **Attributes**:
  - `n_clusters`: Total number of clusters
  - `image_shape`: Shape of individual images
  - `only_collection`: Boolean flag
  - `min_tps`: Minimum TPs threshold
  - `input_file`: Source ROOT file path

## Dataset Visualization

### Bash Wrapper (Recommended)

**Sample Visualization** - Display a grid of sample images:

```bash
./scripts/visualize_dataset.sh dataset.h5 \
  -n 16 \
  -o visualization.png
```

**Statistics Visualization** - Show dataset statistics:

```bash
./scripts/visualize_dataset.sh dataset.h5 \
  --stats \
  -o stats.png
```

### Direct Python Usage

**Sample Visualization:**

```bash
python python/visualize_dataset.py dataset.h5 \
  --n-samples 16 \
  --output visualization.png
```

**Statistics Visualization:**

```bash
python python/visualize_dataset.py dataset.h5 \
  --stats \
  --output stats.png
```

### Parameters

- `dataset`: HDF5 dataset file to visualize (required)
- `--n-samples`, `-n`: Number of samples to display in grid (default: 16)
- `--output`, `-o`: Save to file instead of displaying
- `--stats`: Show statistics instead of samples
- `--no-labels`: Hide labels on sample images

## Image Generation Details

### Pentagon/Triangle/Rectangle Modes

The image creation uses the existing Python implementation in `image_creator.py`, which supports three rendering modes:

1. **Pentagon Mode** (most accurate)
   - 5-vertex polygon shape matching ADC integral
   - Best representation of true waveform shape
   
2. **Triangle Mode**
   - Simplified triangular waveform
   - Linear rise and fall
   
3. **Rectangle Mode**
   - Uniform intensity across time window
   - Fastest but least accurate

Currently uses the pentagon algorithm for best accuracy.

### Fixed Size (32x32)

Images are **always** 32x32 pixels regardless of cluster size:

- **Width (32 pixels)**: Represents channels
  - Spans the channel range of the cluster
  - Margins: 2 pixels on each side
  
- **Height (32 pixels)**: Represents time
  - Spans the time range of the cluster
  - Margins: 2 pixels top and bottom

Clusters are scaled/compressed to fit this fixed size, ensuring:
- Consistent input shape for neural networks
- Memory efficiency
- Fast processing

### Normalization

All pixel values are normalized to [0, 1] range per image:
```python
if img.max() > 0:
    img = img / img.max()
```

## Example Workflow

### 1. Create Dataset

**Using bash wrapper (recommended):**

```bash
./scripts/create_nn_dataset.sh \
  -i /path/to/clusters.root \
  -o data/my_dataset.h5 \
  --channel-map APA \
  --only-collection \
  --min-tps 5 \
  --save-samples 10
```

**Or using Python directly:**

```bash
python python/create_nn_dataset.py \
  --input /path/to/clusters.root \
  --output data/my_dataset.h5 \
  --channel-map APA \
  --only-collection \
  --min-tps 5 \
  --save-samples 10
```

Output:
```
Loading channel map...
Loading clusters from /path/to/clusters.root...
Found 1234 clusters (from 5678 total)
Creating 32x32 dataset from 1234 clusters...
Processing cluster 0/1234
Processing cluster 100/1234
...
Dataset successfully created!
Dataset shape: (1234, 32, 32, 1)
Labels shape: (1234,)
Non-zero images: 1180/1234
Image size: 32x32 pixels
Output file: data/my_dataset.h5
```

### 2. Visualize Samples

```bash
./scripts/visualize_dataset.sh data/my_dataset.h5 \
  -n 16 \
  -o data/samples.png
```

### 3. Check Statistics

```bash
./scripts/visualize_dataset.sh data/my_dataset.h5 \
  --stats \
  -o data/stats.png
```

## Testing

A synthetic test dataset is provided for testing:

```bash
# Create synthetic test dataset
python3 << 'EOF'
import numpy as np
import h5py
from pathlib import Path

n_samples = 20
dataset = np.random.rand(n_samples, 32, 32, 1).astype(np.float32)
labels = np.random.randint(0, 3, n_samples)

Path('data').mkdir(exist_ok=True)
with h5py.File('data/test_dataset_32x32.h5', 'w') as f:
    f.create_dataset('images', data=dataset, compression='gzip')
    f.create_dataset('labels', data=labels, compression='gzip')
    f.attrs['n_clusters'] = n_samples
    f.attrs['image_shape'] = str(dataset.shape[1:])
