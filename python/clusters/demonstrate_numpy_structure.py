#!/usr/bin/env python3
"""
Demonstrate numpy cluster array structure with detailed examples
"""
import numpy as np
import sys
import os

def demonstrate_structure(npy_file):
    """Show detailed structure of a cluster numpy array"""
    
    print("=" * 70)
    print("CLUSTER ARRAY STRUCTURE DEMONSTRATION")
    print("=" * 70)
    print()
    
    # Load array
    arr = np.load(npy_file)
    filename = os.path.basename(npy_file)
    
    print(f"File: {filename}")
    print()
    
    # Basic properties
    print("BASIC PROPERTIES")
    print("-" * 70)
    print(f"  Shape:       {arr.shape}")
    print(f"               ↳ (height, width) = (time_ticks, channels)")
    print(f"  Dimensions:  Y-axis (time): {arr.shape[0]} ticks")
    print(f"               X-axis (channels): {arr.shape[1]} channels")
    print(f"  Data type:   {arr.dtype}")
    print(f"  Memory size: {arr.nbytes} bytes (~{arr.nbytes/1024:.1f} KB)")
    print()
    
    # Value statistics
    print("VALUE STATISTICS")
    print("-" * 70)
    nonzero_mask = arr > 0
    n_nonzero = np.count_nonzero(arr)
    n_total = arr.size
    
    print(f"  Value range:  [{arr.min():.6f}, {arr.max():.6f}]")
    print(f"  Nonzero:      {n_nonzero} / {n_total} pixels ({100*n_nonzero/n_total:.1f}%)")
    print(f"  Sparsity:     {100*(1 - n_nonzero/n_total):.1f}% zeros")
    
    if n_nonzero > 0:
        print(f"  Nonzero mean: {arr[nonzero_mask].mean():.6f}")
        print(f"  Nonzero std:  {arr[nonzero_mask].std():.6f}")
    print()
    
    # Spatial extent (bounding box)
    print("CLUSTER BOUNDING BOX")
    print("-" * 70)
    nonzero_coords = np.argwhere(arr > 0)
    
    if len(nonzero_coords) > 0:
        y_min, x_min = nonzero_coords.min(axis=0)
        y_max, x_max = nonzero_coords.max(axis=0)
        
        cluster_height = y_max - y_min + 1
        cluster_width = x_max - x_min + 1
        
        print(f"  X (channels): {x_min} to {x_max} (width: {cluster_width})")
        print(f"  Y (time):     {y_min} to {y_max} (height: {cluster_height})")
        print()
        
        # Padding analysis
        print("PADDING")
        print("-" * 70)
        pad_left = x_min
        pad_right = arr.shape[1] - x_max - 1
        pad_top = y_min
        pad_bottom = arr.shape[0] - y_max - 1
        
        print(f"  Left:    {pad_left} pixels")
        print(f"  Right:   {pad_right} pixels")
        print(f"  Top:     {pad_top} pixels")
        print(f"  Bottom:  {pad_bottom} pixels")
        
        # Check if centered
        x_centered = abs(pad_left - pad_right) <= 1
        y_centered = abs(pad_top - pad_bottom) <= 1
        
        print(f"  Horizontally centered: {'Yes' if x_centered else 'No'}")
        print(f"  Vertically centered:   {'Yes' if y_centered else 'No'}")
        print()
        
        # Show cross-sections
        print("CROSS-SECTIONS (showing pixel values)")
        print("-" * 70)
        
        # Find a representative row (middle of cluster)
        mid_y = (y_min + y_max) // 2
        row = arr[mid_y, :]
        
        print(f"  Time slice at y={mid_y} (middle of cluster):")
        print(f"    Channels: ", end="")
        for x in range(arr.shape[1]):
            print(f"{x:5d}", end=" ")
        print()
        print(f"    Values:   ", end="")
        for x in range(arr.shape[1]):
            val = row[x]
            if val > 0:
                print(f"{val:5.3f}", end=" ")
            else:
                print("  ---", end=" ")
        print()
        print()
        
        # Find a representative column (middle of cluster)
        mid_x = (x_min + x_max) // 2
        col = arr[:, mid_x]
        
        print(f"  Channel slice at x={mid_x} (middle of cluster):")
        print(f"    Showing first 10 nonzero values:")
        nonzero_times = np.where(col > 0)[0]
        for i, t in enumerate(nonzero_times[:10]):
            print(f"      Time {t:3d}: {col[t]:.6f}")
        if len(nonzero_times) > 10:
            print(f"      ... ({len(nonzero_times) - 10} more nonzero values)")
        print()
        
        # Connectivity check
        print("STRUCTURE VALIDATION")
        print("-" * 70)
        
        # Check for gaps in occupied channels
        occupied_channels = np.unique(nonzero_coords[:, 1])
        expected_channels = np.arange(x_min, x_max + 1)
        channel_gaps = set(expected_channels) - set(occupied_channels)
        
        if len(channel_gaps) == 0:
            print(f"  ✓ Channels are consecutive (no gaps)")
        else:
            print(f"  ⚠ Channels have {len(channel_gaps)} gaps: {sorted(channel_gaps)}")
        
        # Check for gaps in occupied time ticks
        occupied_times = np.unique(nonzero_coords[:, 0])
        expected_times = np.arange(y_min, y_max + 1)
        time_gaps = set(expected_times) - set(occupied_times)
        
        if len(time_gaps) == 0:
            print(f"  ✓ Time ticks are consecutive (no gaps)")
        else:
            if len(time_gaps) < 10:
                print(f"  ~ Time ticks have small gaps at: {sorted(time_gaps)}")
            else:
                print(f"  ⚠ Time ticks have {len(time_gaps)} gaps")
        
        # Check value normalization
        if arr.max() <= 1.0 and arr.min() >= 0.0:
            print(f"  ✓ Values properly normalized to [0, 1]")
        else:
            print(f"  ⚠ Values outside [0, 1] range")
        
        print()
    else:
        print("  Empty cluster (no nonzero pixels)")
        print()
    
    # Access examples
    print("PYTHON ACCESS EXAMPLES")
    print("-" * 70)
    print(f"  # Load")
    print(f"  arr = np.load('{filename}')")
    print()
    print(f"  # Access pixel at channel=5, time=50")
    print(f"  value = arr[50, 5]  # arr[time, channel]")
    print()
    print(f"  # Get all values for channel 3")
    print(f"  channel_3 = arr[:, 3]")
    print()
    print(f"  # Get all values at time tick 100")
    print(f"  time_100 = arr[100, :]")
    print()
    print(f"  # Extract cluster region (without padding)")
    if len(nonzero_coords) > 0:
        print(f"  cluster_only = arr[{y_min}:{y_max+1}, {x_min}:{x_max+1}]")
        print(f"  # Result shape: ({cluster_height}, {cluster_width})")
    print()
    
    print("=" * 70)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python demonstrate_numpy_structure.py <cluster.npy>")
        sys.exit(1)
    
    npy_file = sys.argv[1]
    if not os.path.exists(npy_file):
        print(f"Error: File not found: {npy_file}")
        sys.exit(1)
    
    demonstrate_structure(npy_file)
