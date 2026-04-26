# 3D Reconstruction Guide

## Overview

`python/reco/` is a pure-Python module that takes matched U/V/X clusters and produces
3D charge clouds. It does not require a compiled ROOT installation at runtime — it reads
ROOT files through `uproot` and runs the reconstruction entirely in NumPy.

### How it works

1. Each matched triplet (one cluster per plane) carries TP arrays: channel, time-start,
   time-over-threshold, ADC values.
2. U/V/X wire intersections are computed using the detector geometry in
   `space_transformations.py`, generating a list of 3D candidate voxels per tick.
3. A greedy Orthogonal Matching Pursuit (OMP) selects the sparse set of candidates that
   best explains the measured charge on each plane, or a dense version distributes charge
   proportionally across all hypotheses.

The result is a set of `(x, y, z, amplitude)` voxels — one small point cloud per
matched triplet.

---

## Prerequisites

```bash
pip install uproot numpy matplotlib
```

No ROOT, no shared libraries. The module runs anywhere Python 3 is available.

---

## Script overview

| Script | What it does |
|--------|-------------|
| `reco_display.py` | Interactive event-by-event viewer: U/V/X waveforms + 3D voxel cloud |
| `get_3d_clouds.py` | Batch processor: ROOT files → single compressed `.npz` dataset |
| `plot_3d_clouds.py` | Lightweight NPZ viewer (no ROOT, no reconstruction) |
| `load_matched_clusters.py` | Library: reads ROOT files into Python objects |
| `reco_3d.py` | Library: core reconstruction (OMP + dense) |
| `space_transformations.py` | Library: detector geometry, wire ↔ coordinate conversions |

---

## Interactive viewer

View matched clusters from a single file one event at a time. Close the window to
advance; Ctrl+C to exit.

```bash
# From the repo root
python3 python/reco/reco_display.py \
    --clusters-file /path/to/matched_clusters.root

# Or resolve the file automatically from a pipeline JSON
python3 python/reco/reco_display.py \
    -j json/test_01.json

# View first 5 files from a directory
python3 python/reco/reco_display.py \
    --clusters-dir /path/to/matched_clusters/ \
    -n 5
```

Each window shows:
- Top row: U, V, X waveform histograms with TP shapes drawn as pentagons (or triangles /
  rectangles, see `--draw-mode`). Truth wire positions are overlaid when available.
- Bottom right: 3D voxel cloud coloured by charge amplitude, with truth point and
  momentum arrow.

### Useful options

```bash
--draw-mode pentagon|triangle|rectangle   # TP shape in waveform panels (default: pentagon)
--thr-u 70 --thr-v 70 --thr-x 60         # ADC thresholds for colour scaling
--show-all-hypotheses                     # Overlay all UVX-consistent candidates (faint)
--expand-dt 2                             # Widen TP timing window by ±2 ticks
--omp-iters 50                            # Increase OMP iterations for dense events
-v                                        # Print per-event reconstruction stats
```

---

## Batch cloud generation

Process many files without a GUI. Outputs a single compressed `.npz` file containing
one cloud per matched triplet across all input files.

```bash
python3 python/reco/get_3d_clouds.py \
    --clusters-dir /path/to/matched_clusters/ \
    -n 20 \
    --out output/clouds_test.npz \
    -v

# Or from a single file
python3 python/reco/get_3d_clouds.py \
    --clusters-file /path/to/matched_clusters.root \
    --out output/clouds_test.npz

# Also save all hypothesis candidates (larger file, useful for debugging)
python3 python/reco/get_3d_clouds.py \
    --clusters-file /path/to/matched_clusters.root \
    --out output/clouds_test.npz \
    --save-all-hypotheses
```

### NPZ output schema

```
event_id        (M,)      int64    event index for each group
match_id        (M,)      int64    match_id for each group
collection_adc  (M,)      float64  total X-plane ADC (charge proxy)
truth_xyz       (M, 3)    float64  true (x,y,z) vertex; NaN if unavailable
truth_mom_xyz   (M, 3)    float64  true (px,py,pz); NaN if unavailable
cloud_data      (N, 4)    float32  concatenated voxels: (x, y, z, amplitude)
cloud_offsets   (M+1,)    int64    slice group i: cloud_data[offsets[i]:offsets[i+1]]

# Optional (with --save-all-hypotheses):
hyp_data        (K, 3)    float32  all UVX-consistent candidates: (x, y, z)
hyp_offsets     (M+1,)    int64    slice group i: hyp_data[offsets[i]:offsets[i+1]]
```

**Reading a cloud in Python:**

```python
import numpy as np

data = np.load("output/clouds_test.npz")
offsets = data["cloud_offsets"]
clouds  = data["cloud_data"]      # shape (N, 4): x, y, z, amplitude

# Extract cloud for group i
def get_cloud(i):
    return clouds[offsets[i]:offsets[i+1]]   # shape (n_voxels, 4)

# Iterate all groups
for i in range(len(data["event_id"])):
    cloud = get_cloud(i)
    truth = data["truth_xyz"][i]
    print(f"Event {data['event_id'][i]}, match {data['match_id'][i]}: "
          f"{len(cloud)} voxels, truth=({truth[0]:.1f}, {truth[1]:.1f}, {truth[2]:.1f}) cm")
```

---

## Viewing an NPZ file

Once you have a `.npz` from `get_3d_clouds.py`, you can browse it without ROOT:

```bash
python3 python/reco/plot_3d_clouds.py output/clouds_test.npz

# Start from group 10
python3 python/reco/plot_3d_clouds.py output/clouds_test.npz --start 10

# Show only a specific event
python3 python/reco/plot_3d_clouds.py output/clouds_test.npz --only-event 42

# Keep only the brightest 60% of voxels (cleaner view for busy events)
python3 python/reco/plot_3d_clouds.py output/clouds_test.npz --top-frac 0.6
```

---

## JSON config integration

All three entry scripts accept `-j <config.json>`. When supplied, the matched_clusters
folder is resolved from the pipeline JSON using the same naming convention as
`make_clusters` and `match_clusters`.

The relevant JSON keys are the same ones used by the rest of the pipeline:

```json
{
    "signal_folder":        "/path/to/data/",
    "products_prefix":      "es_test",
    "tick_limit":           3,
    "channel_limit":        2,
    "min_tps_to_cluster":   2,
    "tot_cut":              3,
    "energy_cut":           2.0
}
```

This resolves to the matched_clusters folder:
`/path/to/data/es_test_matched_clusters_tick3_ch2_min2_tot3_e2p0/`

You can also override the folder explicitly:

```json
{
    "matched_clusters_folder": "/absolute/path/to/matched_clusters_folder/"
}
```

---

## Key reconstruction parameters

| Parameter | Default | Effect |
|-----------|---------|--------|
| `--omp-iters` | 30 | Max OMP iterations. Increase for events with many overlapping tracks. |
| `--omp-max-reuse` | 1 | Max times the same (tick, wire) bin can be selected. |
| `--omp-stop-l1` | 1e-6 | Stop OMP when L1 residual drops below this. |
| `--expand-dt` | 1 | Timing tolerance (±ticks) for placing TP charge. Increase for noisier data. |
| `--use-peak-tick` | off | Place all TP charge at the peak tick only (sharper but coarser). |
| `--tdc-factor` | 32.0 | TDC→TPC tick conversion factor. Change only if your data uses a different clock ratio. |
| `--no-tdc-to-tpc` | off | Skip TDC→TPC conversion entirely. |

---

## Module API

Import the libraries directly for scripted analysis:

```python
import sys
from pathlib import Path
sys.path.insert(0, str(Path("python/reco")))

from load_matched_clusters import MatchedClustersLoader
from reco_3d import reconstruct_cloud_dense

# Load matched clusters
loader = MatchedClustersLoader("matched_clusters.root", verbose=True)
groups = loader.get_matched_groups()   # list of MatchedGroup

for group in groups:
    u = group.u_cluster   # ClusterItem or None
    v = group.v_cluster
    x = group.x_cluster

    if u is None or v is None or x is None:
        continue

    # Reconstruct 3D cloud
    result = reconstruct_cloud_dense(u, v, x)
    # result.candidates: list of (x, y, z) tuples
    # result.amplitudes: corresponding charge weights
    # result.residual_norm: reconstruction residual

    print(f"Event {group.event_id}, match {group.match_id}: "
          f"{len(result.candidates)} voxels")
```

**`ClusterItem` attributes:**

```python
item.plane          # 'U', 'V', or 'X'
item.event_id       # int
item.cluster_id     # int
item.match_id       # int (-1 if unmatched)

# TP arrays (one entry per TP in the cluster)
item.ch             # np.array, detector channels
item.tstart         # np.array, time start (TPC ticks after conversion)
item.sot            # np.array, samples over threshold (duration)
item.stopeak        # np.array, samples to peak
item.adc_peak       # np.array
item.adc_integral   # np.array

# Truth (NaN/None if unavailable)
item.true_pos       # (x, y, z) in cm
item.true_mom       # (px, py, pz) in MeV/c
```
