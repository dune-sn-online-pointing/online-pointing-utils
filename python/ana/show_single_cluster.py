#!/usr/bin/env python3
"""
Load a single cluster .npy file and save a PNG for inspection.
Usage:
    python3 python/ana/show_single_cluster.py <input.npy> <output.png>
"""
import sys
from pathlib import Path
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def main():
    if len(sys.argv) < 3:
        print("Usage: show_single_cluster.py <input.npy> <output.png>")
        sys.exit(1)
    in_path = Path(sys.argv[1])
    out_path = Path(sys.argv[2])

    if not in_path.exists():
        print(f"Input file not found: {in_path}")
        sys.exit(2)

    arr = np.load(in_path)
    print(f"Loaded: {in_path.name}")
    print(f"  shape: {arr.shape}, dtype: {arr.dtype}")
    print(f"  nonzero: {np.count_nonzero(arr)} / {arr.size}")
    print(f"  min/max: {arr.min():.6f} / {arr.max():.6f}")

    # Draw with channel on X and time on Y as in display.cpp
    fig, ax = plt.subplots(figsize=(6,6))
    im = ax.imshow(arr, cmap='viridis', origin='lower', aspect='auto', vmin=0, vmax=1)
    ax.set_xlabel('Channel (contiguous index)')
    ax.set_ylabel('Time (tick)')
    ax.set_title(in_path.stem)
    cbar = plt.colorbar(im, ax=ax)
    cbar.set_label('Normalized ADC (pentagon interpolation)')
    plt.tight_layout()
    plt.savefig(out_path, dpi=150)
    print(f"Saved PNG: {out_path}")

if __name__ == '__main__':
    main()
