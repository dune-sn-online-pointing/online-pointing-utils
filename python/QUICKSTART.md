# Python Cluster Display - Quick Start Guide

## Installation

```bash
cd /home/virgolaema/dune/online-pointing-utils

# Install Python dependencies
pip3 install --user -r python/requirements.txt

# Or install system-wide (requires sudo)
sudo pip3 install -r python/requirements.txt
```

## Quick Start

### Using the shell wrapper (recommended)

```bash
# Pentagon mode (default)
./scripts/cluster_display_py.sh --clusters-file data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root

# Triangle mode
./scripts/cluster_display_py.sh --clusters-file data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root --draw-mode triangle

# Rectangle mode
./scripts/cluster_display_py.sh --clusters-file data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root --draw-mode rectangle
```

### Direct Python invocation

```bash
python3 python/cluster_display.py --clusters-file <file.root> --draw-mode pentagon
```

## Usage Examples

### 1. Compare with C++ Version

**C++ version:**
```bash
./scripts/display.sh -j json/display/example.json --draw-mode pentagon
```

**Python version (equivalent):**
```bash
./scripts/cluster_display_py.sh -j json/display/example.json --draw-mode pentagon
```

### 2. Browse Individual Clusters

```bash
# Pentagon mode - most accurate
./scripts/cluster_display_py.sh \
  --clusters-file data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root \
  --mode clusters \
  --draw-mode pentagon
```

Use the Prev/Next buttons in the matplotlib window to navigate.

### 3. View Aggregated Events

```bash
# Show all clusters per event
./scripts/cluster_display_py.sh \
  --clusters-file data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root \
  --mode events

# Show only MARLEY clusters per event
./scripts/cluster_display_py.sh \
  --clusters-file data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root \
  --mode events \
  --only-marley
```

### 4. Different Drawing Modes

**Pentagon (default)** - Most accurate, extended boundaries:
```bash
./scripts/cluster_display_py.sh --clusters-file <file.root> --draw-mode pentagon
```

**Triangle** - Linear approximation:
```bash
./scripts/cluster_display_py.sh --clusters-file <file.root> --draw-mode triangle
```

**Rectangle** - Uniform intensity, fastest:
```bash
./scripts/cluster_display_py.sh --clusters-file <file.root> --draw-mode rectangle
```

## Features

### Interactive Navigation
- **Prev Button**: Navigate to previous cluster/event
- **Next Button**: Navigate to next cluster/event
- **Keyboard**: Arrow keys work when plot window is focused
- **Close**: Click X to exit

### Visualization Details
- **Colormap**: Viridis (perceptually uniform)
- **Threshold**: Set per plane in `parameters/display.dat`
- **Physical Units**: Secondary Y-axis shows drift distance in cm
- **Channel Labels**: X-axis shows actual detector channel numbers

### Drawing Modes

| Mode | Algorithm | Best For |
|------|-----------|----------|
| **Pentagon** | Grid search (10×10) + Shoelace area | Detailed analysis, publications |
| **Triangle** | Linear rise/fall | Good balance, standard use |
| **Rectangle** | Uniform intensity | Quick overview, many clusters |

## Parameters

The Python version reads the same parameter files as the C++ version:

```
parameters/
  ├── display.dat      # Thresholds and drawing settings
  ├── timing.dat       # Time conversions and drift velocity
  └── geometry.dat     # Wire pitch and detector dimensions
```

To use alternative parameters:
```bash
./scripts/cluster_display_py.sh \
  --clusters-file <file.root> \
  --params-dir /path/to/custom/parameters
```

## Troubleshooting

### Import Error: No module named 'uproot'

Install dependencies:
```bash
pip3 install --user uproot matplotlib numpy
```

### No clusters displayed

Check that your ROOT file contains MARLEY clusters:
```bash
# Use C++ version to verify
./build/src/app/cluster_display --clusters-file <file.root>
```

Or try events mode:
```bash
./scripts/cluster_display_py.sh --clusters-file <file.root> --mode events
```

### Display window doesn't appear

Make sure you have a display available:
```bash
echo $DISPLAY
```

If empty, you need X11 forwarding or a local display.

### Slow rendering

Use rectangle mode for faster visualization:
```bash
./scripts/cluster_display_py.sh --clusters-file <file.root> --draw-mode rectangle
```

## Comparison with C++ Version

### What's the Same
✅ Drawing algorithms (pentagon, triangle, rectangle)  
✅ Parameter loading from .dat files  
✅ ROOT file structure and branches  
✅ MARLEY filtering logic  
✅ Viridis colormap  
✅ Physical units (cm)  

### What's Different
📝 matplotlib instead of ROOT graphics  
📝 Button widgets instead of TButton  
📝 Slightly different axis layout  
📝 Python-native data structures  

## Performance

For typical datasets:
- **Loading**: ~1-2 seconds for ~1500 clusters
- **Navigation**: <0.1 seconds per cluster
- **Pentagon mode**: <0.2 seconds per cluster
- **Triangle mode**: <0.15 seconds per cluster
- **Rectangle mode**: <0.1 seconds per cluster

## Advanced Usage

### Custom Parameters in JSON

Create a JSON file with custom settings:

```json
{
  "clusters_file": "data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root",
  "mode": "clusters",
  "draw_mode": "pentagon",
  "only_marley": false
}
```

Then use:
```bash
./scripts/cluster_display_py.sh -j my_config.json
```

### Programmatic Usage

You can also import and use the classes directly:

```python
from cluster_display import ClusterViewer

viewer = ClusterViewer(
    clusters_file="data/clusters.root",
    mode="clusters",
    draw_mode="pentagon"
)
viewer.run()
```

## Getting Help

```bash
# Command-line help
./scripts/cluster_display_py.sh --help

# Or
python3 python/cluster_display.py --help
```

For more details, see `python/README.md`.
