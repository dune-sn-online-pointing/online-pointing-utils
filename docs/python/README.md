# Python Cluster Display

Python port of the C++ `cluster_display` application for interactive visualization of MARLEY trigger primitive (TP) clusters.

## Features

- **Interactive Navigation**: Browse through clusters with Prev/Next buttons
- **Multiple Drawing Modes**:
  - **Pentagon**: Most accurate waveform representation with extended boundaries
  - **Triangle**: Linear rise/fall approximation
  - **Rectangle**: Uniform intensity distribution
- **Two Display Modes**:
  - **Clusters**: Individual cluster visualization
  - **Events**: Event-level aggregation
- **Viridis Colormap**: Perceptually uniform color scale
- **Physical Units**: Secondary axes showing drift distance in cm
- **MARLEY Filtering**: Option to show only MARLEY clusters

## Installation

```bash
# Install dependencies
pip install -r requirements.txt

# Or install individually
pip install uproot matplotlib numpy scipy
```

**Requirements**:
- `uproot>=5.0.0` - ROOT file I/O without ROOT dependency
- `matplotlib>=3.5.0` - Interactive plotting
- `numpy>=1.20.0` - Array operations
- `scipy>=1.7.0` - Pentagon algorithm optimization

## Usage

### Basic Usage

```bash
# Display clusters with pentagon mode (default)
python cluster_display.py --clusters-file data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root

# Use triangle drawing mode
python cluster_display.py --clusters-file <file.root> --draw-mode triangle

# Use rectangle mode for quick overview
python cluster_display.py --clusters-file <file.root> --draw-mode rectangle
```

### Event Mode

```bash
# Display events (aggregated clusters)
python cluster_display.py --clusters-file <file.root> --mode events

# Show only MARLEY clusters in event mode
python cluster_display.py --clusters-file <file.root> --mode events --only-marley
```

### Using JSON Configuration

```bash
# Load settings from JSON
python cluster_display.py -j json/display/example.json
```

Example JSON format:
```json
{
  "clusters_file": "data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root",
  "mode": "clusters",
  "draw_mode": "pentagon",
  "only_marley": false
}
```

## Command-Line Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `--clusters-file` | file path | *required* | Input clusters ROOT file |
| `--mode` | clusters, events | clusters | Display mode |
| `--draw-mode` | triangle, pentagon, rectangle | pentagon | Drawing/rendering mode |
| `--only-marley` | flag | false | In events mode, show only MARLEY clusters |
| `-j, --json` | file path | - | JSON configuration file |
| `--params-dir` | directory | parameters | Directory with .dat parameter files |
| `-v, --verbose` | flag | false | Verbose output |

## Drawing Modes Comparison

### Pentagon Mode (Default - **Recommended**)
- **Most accurate** waveform representation with plateau baseline
- Uses scipy optimization to fit pentagon shape above threshold
- Threshold plateau: Rectangular baseline from 0 to plane-specific threshold
- Signal pentagon: Optimized to match `adc_integral` with minimal error
- Extended range: fills 1-2 pixels beyond nominal boundaries for smooth visualization
- Best for detailed analysis and publications
- See `PENTAGON_ALGORITHM.md` for technical details

### Triangle Mode
- **Linear approximation** with rise and fall edges
- Simpler calculation, faster rendering
- Good balance between accuracy and performance
- Suitable for quick analysis

### Rectangle Mode
- **Uniform intensity** across all samples
- Intensity = `adc_integral / samples_over_threshold`
- Fastest rendering mode
- Good for quick overview of many clusters

## Parameters

The application reads parameters from `.dat` files in the `parameters/` directory:

- `display.dat`: ADC thresholds
- `timing.dat`: Time tick conversions
- `geometry.dat`: Wire pitch and detector dimensions

Key parameters:
```
display.threshold_adc_u = 70.0    # ADC threshold for U plane
display.threshold_adc_v = 70.0    # ADC threshold for V plane
display.threshold_adc_x = 60.0    # ADC threshold for X plane
timing.time_tick_cm = 0.0805      # Time tick in cm
geometry.wire_pitch_collection_cm = 0.479  # Wire pitch (collection)
```

## Navigation

- **Prev Button**: Go to previous cluster/event
- **Next Button**: Go to next cluster/event
- **Close Window**: Exit application

## Differences from C++ Version

### Similarities
✅ Identical drawing algorithms (pentagon, triangle, rectangle)  
✅ Same parameter loading from .dat files  
✅ Same ROOT file reading structure  
✅ Same MARLEY filtering logic  
✅ Viridis colormap  
✅ Secondary axes in cm  

### Differences
- Uses `matplotlib` instead of ROOT's TCanvas/TH2F
- Button navigation via matplotlib widgets instead of ROOT TButton
- Time unit conversion handled in Python (TDC → TPC ticks)
- Slightly different axis positioning due to matplotlib layout

## Examples

### Compare Drawing Modes

```bash
# Pentagon mode - most accurate
python cluster_display.py --clusters-file <file.root> --draw-mode pentagon

# Triangle mode - good balance
python cluster_display.py --clusters-file <file.root> --draw-mode triangle

# Rectangle mode - fastest
python cluster_display.py --clusters-file <file.root> --draw-mode rectangle
```

### Analyze Specific Event

```bash
# Navigate through clusters to find event of interest
python cluster_display.py --clusters-file <file.root> --draw-mode pentagon
# Use Next button to navigate to desired cluster
```

## Troubleshooting

### No clusters displayed
- Check that the ROOT file contains MARLEY clusters
- Verify the file has the `clusters/` directory structure
- Try `--mode events` to see aggregated data

### Colors look wrong
- The application uses the Viridis colormap (perceptually uniform)
- Color scale is in ADC units based on the drawing mode
- Adjust thresholds in `parameters/display.dat`

### Missing parameters
- Ensure `parameters/` directory exists with .dat files
- Use `--params-dir` to specify alternative location
- Check parameter file syntax: `< key = value >`

## Performance

For typical clusters with 10-20 TPs:
- **Rectangle**: ~100% (fastest)
- **Triangle**: ~95%
- **Pentagon**: ~85% (most accurate)

All modes are fast enough for interactive use.

## Development

The code is structured in classes:

- `Parameters`: Load and manage .dat parameter files
- `ClusterItem`: Represents a single cluster or event
- `DrawingAlgorithms`: Pentagon/triangle/rectangle filling algorithms
- `ClusterViewer`: Main application with matplotlib UI

## Equivalent C++ Command

This Python application replaces:
```bash
./build/src/app/cluster_display \
  --clusters-file <file.root> \
  --draw-mode pentagon \
  --mode clusters
```

With:
```bash
python python/ana/cluster_display.py \
  --clusters-file <file.root> \
  --draw-mode pentagon \
  --mode clusters
```

## License

Same as the parent `online-pointing-utils` project.
