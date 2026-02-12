#!/usr/bin/env python3
"""
Verify that cluster arrays have consecutive TPs with padding
"""
import numpy as np
import matplotlib.pyplot as plt
import sys
import os

def analyze_cluster(filepath):
    """Analyze cluster array to verify consecutive structure"""
    arr = np.load(filepath)
    
    print(f"\nAnalyzing: {os.path.basename(filepath)}")
    print(f"  Shape: {arr.shape}")
    print(f"  Nonzero pixels: {np.count_nonzero(arr)} / {arr.size}")
    
    # Find bounding box of nonzero pixels
    nonzero = np.argwhere(arr > 0)
    if len(nonzero) > 0:
        y_min, x_min = nonzero.min(axis=0)
        y_max, x_max = nonzero.max(axis=0)
        
        cluster_height = y_max - y_min + 1
        cluster_width = x_max - x_min + 1
        
        print(f"  Cluster bounding box:")
        print(f"    X range: {x_min} to {x_max} (width: {cluster_width} channels)")
        print(f"    Y range: {y_min} to {y_max} (height: {cluster_height} ticks)")
        print(f"  Padding:")
        print(f"    Left: {x_min}, Right: {arr.shape[1] - x_max - 1}")
        print(f"    Top: {y_min}, Bottom: {arr.shape[0] - y_max - 1}")
        
        # Check for gaps in X (channels should be consecutive)
        x_coords = np.unique(nonzero[:, 1])
        expected_x = np.arange(x_min, x_max + 1)
        if np.array_equal(x_coords, expected_x):
            print(f"  ✓ Channels are consecutive (no gaps)")
        else:
            missing = set(expected_x) - set(x_coords)
            print(f"  ✗ Channels have gaps at: {missing}")
        
        # Check for gaps in Y (time should be mostly consecutive)
        y_coords = np.unique(nonzero[:, 0])
        expected_y = np.arange(y_min, y_max + 1)
        if np.array_equal(y_coords, expected_y):
            print(f"  ✓ Time ticks are consecutive (no gaps)")
        else:
            missing = set(expected_y) - set(y_coords)
            if len(missing) < 5:
                print(f"  ~ Time ticks have small gaps at: {missing}")
            else:
                print(f"  ✗ Time ticks have {len(missing)} gaps")
        
        return arr, (y_min, y_max, x_min, x_max)
    else:
        print("  Empty cluster!")
        return arr, None

def visualize_multiple(filepaths, output_path):
    """Visualize multiple clusters in a grid"""
    n = len(filepaths)
    fig, axes = plt.subplots(1, n, figsize=(5*n, 5))
    if n == 1:
        axes = [axes]
    
    for ax, filepath in zip(axes, filepaths):
        arr, bbox = analyze_cluster(filepath)
        
        # Display with grid lines to show pixel boundaries
        im = ax.imshow(arr, cmap='viridis', interpolation='nearest', aspect='auto', origin='lower')
        
        # Add grid
        ax.set_xticks(np.arange(-0.5, arr.shape[1], 1), minor=True)
        ax.set_yticks(np.arange(-0.5, arr.shape[0], 1), minor=True)
        ax.grid(which='minor', color='gray', linestyle='-', linewidth=0.2, alpha=0.3)
        
        ax.set_xlabel('Channel (X)')
        ax.set_ylabel('Time Tick (Y)')
        ax.set_title(os.path.basename(filepath))
        
        plt.colorbar(im, ax=ax, label='Normalized ADC')
        
        # Draw bounding box if present
        if bbox:
            y_min, y_max, x_min, x_max = bbox
            from matplotlib.patches import Rectangle
            rect = Rectangle((x_min - 0.5, y_min - 0.5), 
                           x_max - x_min + 1, y_max - y_min + 1,
                           linewidth=2, edgecolor='red', facecolor='none')
            ax.add_patch(rect)
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"\n✓ Saved visualization to: {output_path}")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python verify_consecutive.py <output.png> <cluster1.npy> [cluster2.npy] ...")
        sys.exit(1)
    
    output_path = sys.argv[1]
    cluster_files = sys.argv[2:]
    
    visualize_multiple(cluster_files, output_path)
