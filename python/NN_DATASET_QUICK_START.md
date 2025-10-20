# Neural Network Dataset - Quick Start Guide

## Quick Commands

### Create Dataset from Clusters
```bash
# Basic usage - only collection plane (recommended for NN training)
./scripts/create_nn_dataset.sh \
  -i /path/to/clusters.root \
  -o data/dataset.h5 \
  --only-collection

# With all options
./scripts/create_nn_dataset.sh \
  -i /path/to/clusters.root \
  -o data/dataset.h5 \
  --channel-map APA \
  --only-collection \
  --min-tps 5 \
  --save-samples 10

# Quick test (100 clusters only)
./scripts/create_nn_dataset.sh \
  -i /path/to/clusters.root \
  -o data/test.h5 \
  --only-collection \
  --max-clusters 100
```

### Visualize Dataset
```bash
# Display 16 samples
./scripts/visualize_dataset.sh data/dataset.h5 -o samples.png

# Display 25 samples
./scripts/visualize_dataset.sh data/dataset.h5 -n 25 -o samples_25.png

# Show statistics
./scripts/visualize_dataset.sh data/dataset.h5 --stats -o stats.png
```

## Common Options

### create_nn_dataset.sh
| Option | Description | Default |
|--------|-------------|---------|
| `-i, --input` | Input ROOT file (required) | - |
| `-o, --output` | Output HDF5 file (required) | - |
| `--channel-map` | Detector: APA, CRP, or 50L | APA |
| `--only-collection` | Use only X plane | false |
| `--min-tps` | Minimum TPs per cluster | 5 |
| `--max-clusters` | Limit clusters (testing) | all |
| `--save-samples` | Save N sample PNGs | 0 |
| `-h, --help` | Show help | - |

### visualize_dataset.sh
| Option | Description | Default |
|--------|-------------|---------|
| `dataset` | HDF5 file (required) | - |
| `-n, --n-samples` | Number of samples | 16 |
| `-o, --output` | Save to file | display |
| `--stats` | Show statistics | false |
| `--no-labels` | Hide labels | false |
| `-h, --help` | Show help | - |

## Output Format

### HDF5 Dataset Structure
```
dataset.h5
├── images     # (n_clusters, 32, 32, 1) float32, range [0,1]
├── labels     # (n_clusters,) int32
└── attributes
    ├── n_clusters
    ├── image_shape
    ├── only_collection
    ├── min_tps
    └── input_file
```

### Image Properties
- **Size**: Always 32×32 pixels
- **Values**: Normalized to [0, 1]
- **Channels**: 1 (X plane) or 3 (U+V+X)
- **Format**: Float32
- **Width**: Represents detector channels
- **Height**: Represents time ticks

## Typical Workflow

```bash
# 1. Create dataset from cluster ROOT file
./scripts/create_nn_dataset.sh \
  -i /eos/user/.../clusters.root \
  -o data/my_dataset.h5 \
  --only-collection \
  --min-tps 5 \
  --save-samples 10

# Output:
# Dataset shape: (1234, 32, 32, 1)
# Non-zero images: 1180/1234
# Image size: 32x32 pixels
# Output file: data/my_dataset.h5
# Sample images saved to: data/my_dataset_samples/

# 2. Visualize samples to verify quality
./scripts/visualize_dataset.sh data/my_dataset.h5 \
  -n 16 \
  -o data/visualization.png

# 3. Check statistics
./scripts/visualize_dataset.sh data/my_dataset.h5 \
  --stats \
  -o data/stats.png

# 4. Use in ML training
python train_model.py --dataset data/my_dataset.h5
```

## Tips

### Performance
- Use `--only-collection` for 3× faster processing
- Use `--max-clusters` for quick tests
- Processing ~1000 clusters takes 1-2 minutes

### Quality Control
- Always check `--save-samples` output first
- Verify non-zero image count is reasonable
- Check label distribution with `--stats`

### Memory
- 10,000 clusters ≈ 40 MB uncompressed
- HDF5 gzip compression saves ~70% space
- 32×32 is very efficient for NNs

### Troubleshooting
- If no images created: lower `--min-tps`
- If images look strange: check channel map type
- If script fails: run with Python directly for error details

## Example: Full Pipeline

```bash
# Create training dataset
./scripts/create_nn_dataset.sh \
  -i /eos/user/data/signal_clusters.root \
  -o data/train_signal.h5 \
  --only-collection \
  --min-tps 5

# Create validation dataset
./scripts/create_nn_dataset.sh \
  -i /eos/user/data/validation_clusters.root \
  -o data/val_signal.h5 \
  --only-collection \
  --min-tps 5

# Verify both datasets
./scripts/visualize_dataset.sh data/train_signal.h5 --stats -o data/train_stats.png
./scripts/visualize_dataset.sh data/val_signal.h5 --stats -o data/val_stats.png

# Check samples
./scripts/visualize_dataset.sh data/train_signal.h5 -n 25 -o data/train_samples.png
./scripts/visualize_dataset.sh data/val_signal.h5 -n 25 -o data/val_samples.png
```

## Need Help?

```bash
# Show detailed help
./scripts/create_nn_dataset.sh --help
./scripts/visualize_dataset.sh --help

# Read full documentation
cat python/NN_DATASET_README.md
```
