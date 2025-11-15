# Display Script Usage Examples

## Overview

The `display.sh` script provides a convenient wrapper around the `cluster_display` application with support for multiple drawing modes.

## Basic Usage

```bash
./scripts/display.sh --input-clusters <file.root> [options]
```

## Draw Mode Options

The script now supports three drawing modes via the `--draw-mode` flag:

### 1. Pentagon Mode (Default)
```bash
./scripts/display.sh \
  -j json/display/example.json \
  --draw-mode pentagon
```

**Features:**
- Most accurate waveform representation
- Uses grid search for optimal pentagon vertices
- Extended range fills 1-2 pixels beyond boundaries
- Best for detailed analysis

### 2. Rectangle Mode
```bash
./scripts/display.sh \
  -j json/display/example.json \
  --draw-mode rectangle
```

**Features:**
- Uniform intensity across all samples
- Intensity = adc_integral / samples_over_threshold
- Fastest rendering
- Good for quick overview

### 3. Triangle Mode
```bash
./scripts/display.sh \
  -j json/display/example.json \
  --draw-mode triangle
```

**Features:**
- Linear rise and fall from threshold to peak
- Simple approximation
- Good compromise between accuracy and speed

## Complete Examples

### Example 1: View clusters with pentagon mode
```bash
./scripts/display.sh \
  --input-clusters data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root \
  --mode clusters \
  --draw-mode pentagon
```

### Example 2: View events with rectangle mode (MARLEY only)
```bash
./scripts/display.sh \
  --input-clusters data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root \
  --mode events \
  --draw-mode rectangle \
  --only-marley
```

### Example 3: Using JSON settings (pentagon is default)
```bash
./scripts/display.sh -j json/display/example.json
```

### Example 4: Triangle mode with verbose output
```bash
./scripts/display.sh \
  -j json/display/example.json \
  --draw-mode triangle \
  -v
```

### Example 5: Quick view without recompilation
```bash
./scripts/display.sh \
  -j json/display/example.json \
  --draw-mode rectangle \
  --no-compile
```

## All Available Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `--input-clusters` | file path | - | Input clusters ROOT file (required unless via JSON) |
| `-j, --json-settings` | file path | - | JSON configuration file (relative to json/) |
| `--mode` | clusters, events | clusters | Display mode |
| `--draw-mode` | triangle, pentagon, rectangle | **pentagon** | Drawing/rendering mode |
| `--only-marley` | flag | false | In events mode, show only MARLEY clusters |
| `-v, --verbose-mode` | flag | false | Enable verbose output |
| `--no-compile` | flag | false | Skip recompilation |
| `--clean-compile` | flag | false | Clean and recompile |
| `-h, --help` | flag | - | Show help message |

## JSON Configuration

You can also specify the draw mode in your JSON settings file:

```json
{
  "clusters_file": "data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root",
  "mode": "clusters",
  "draw_mode": "pentagon",
  "only_marley": false
}
```

Command-line arguments override JSON settings.

## Performance Comparison

For typical clusters with 10-20 TPs:

| Mode | Relative Speed | Use Case |
|------|----------------|----------|
| Rectangle | 100% (fastest) | Quick overview, many clusters |
| Triangle | ~95% | Good balance |
| Pentagon | ~85% | Detailed analysis, publications |

## Tips

1. **Start with rectangle mode** for quick navigation through many clusters
2. **Use pentagon mode** when you need to analyze specific clusters in detail
3. **Triangle mode** is a good middle ground for most use cases
4. **Verbose mode** (`-v`) helps debug when clusters don't appear as expected
5. **Use JSON settings** to save your preferred configuration

## Troubleshooting

### Missing samples at boundaries
- Use pentagon mode with extended range (default behavior)
- Check that your TP data has valid samples_over_threshold values

### Display too slow
- Switch to rectangle or triangle mode
- Use `--no-compile` to skip recompilation

### Colors look wrong
- The script uses the Viridis palette (perceptually uniform)
- Color scale is in ADC units based on the selected drawing mode

## Examples with Real Data

### Analyze specific event from debug output
```bash
# Event 2, Plane V, Cluster 3/7 mentioned in your printout
./scripts/display.sh \
  -j json/display/example.json \
  --draw-mode pentagon
# Then use "Next" button to navigate to cluster 3
```

### Compare different modes on same data
```bash
# Pentagon mode
./scripts/display.sh -j json/display/example.json --draw-mode pentagon

# Rectangle mode for comparison
./scripts/display.sh -j json/display/example.json --draw-mode rectangle --no-compile

# Triangle mode
./scripts/display.sh -j json/display/example.json --draw-mode triangle --no-compile
```

## Default Behavior

If you don't specify `--draw-mode`, the script defaults to **pentagon** mode, which provides the most accurate visualization with extended boundaries to show all samples including first and last ticks.
