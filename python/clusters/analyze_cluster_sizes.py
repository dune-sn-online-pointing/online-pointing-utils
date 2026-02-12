#!/usr/bin/env python3
"""
Analyze cluster sizes to determine optimal image dimensions
"""
import uproot
import numpy as np
import sys
import os
import glob

def analyze_cluster_sizes(root_files):
    """Analyze cluster dimensions across all planes"""
    all_widths = []  # channel extent
    all_heights = []  # time extent
    
    for root_file in root_files:
        print(f"Analyzing {os.path.basename(root_file)}...")
        
        with uproot.open(root_file) as f:
            for plane in ['U', 'V', 'X']:
                tree_name = f'clusters/clusters_tree_{plane}'
                if tree_name not in f:
                    continue
                
                tree = f[tree_name]
                n_entries = tree.num_entries
                
                if n_entries == 0:
                    continue
                
                # Read TP data
                channels = tree['tp_detector_channel'].array(library='np')
                times = tree['tp_time_start'].array(library='np')
                sot = tree['tp_samples_over_threshold'].array(library='np')
                
                # Analyze each cluster
                for i in range(len(channels)):
                    if len(channels[i]) == 0:
                        continue
                    
                    ch_arr = np.array(channels[i])
                    time_arr = np.array(times[i])
                    sot_arr = np.array(sot[i])
                    
                    # Calculate extents
                    width = ch_arr.max() - ch_arr.min() + 1
                    time_end = time_arr + sot_arr
                    height = time_end.max() - time_arr.min() + 1
                    
                    all_widths.append(width)
                    all_heights.append(height)
    
    return np.array(all_widths), np.array(all_heights)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python analyze_cluster_sizes.py <cluster_folder>")
        sys.exit(1)
    
    folder = sys.argv[1]
    root_files = sorted(glob.glob(os.path.join(folder, 'clusters_*.root')))
    
    if not root_files:
        print(f"No cluster files found in {folder}")
        sys.exit(1)
    
    print(f"Found {len(root_files)} ROOT files")
    print("=" * 60)
    
    widths, heights = analyze_cluster_sizes(root_files)
    
    print("\n" + "=" * 60)
    print("CLUSTER SIZE STATISTICS")
    print("=" * 60)
    
    print(f"\nTotal clusters analyzed: {len(widths)}")
    
    print(f"\nChannel extent (width):")
    print(f"  Mean: {np.mean(widths):.1f}")
    print(f"  Median: {np.median(widths):.1f}")
    print(f"  Std: {np.std(widths):.1f}")
    print(f"  Min/Max: {widths.min()} / {widths.max()}")
    print(f"  Percentiles:")
    for p in [50, 75, 90, 95, 99]:
        print(f"    {p}%: {np.percentile(widths, p):.1f}")
    
    print(f"\nTime extent (height):")
    print(f"  Mean: {np.mean(heights):.1f}")
    print(f"  Median: {np.median(heights):.1f}")
    print(f"  Std: {np.std(heights):.1f}")
    print(f"  Min/Max: {heights.min()} / {heights.max()}")
    print(f"  Percentiles:")
    for p in [50, 75, 90, 95, 99]:
        print(f"    {p}%: {np.percentile(heights, p):.1f}")
    
    print(f"\nAspect ratio (height/width):")
    aspect_ratios = heights / np.maximum(widths, 1)
    print(f"  Mean: {np.mean(aspect_ratios):.2f}")
    print(f"  Median: {np.median(aspect_ratios):.2f}")
    
    # Recommendations
    print("\n" + "=" * 60)
    print("RECOMMENDATIONS")
    print("=" * 60)
    
    w95 = np.percentile(widths, 95)
    h95 = np.percentile(heights, 95)
    w99 = np.percentile(widths, 99)
    h99 = np.percentile(heights, 99)
    
    print(f"\nTo capture 95% of clusters without cropping:")
    print(f"  Width: {w95:.0f}, Height: {h95:.0f}")
    print(f"  Square: {max(w95, h95):.0f} x {max(w95, h95):.0f}")
    
    print(f"\nTo capture 99% of clusters without cropping:")
    print(f"  Width: {w99:.0f}, Height: {h99:.0f}")
    print(f"  Square: {max(w99, h99):.0f} x {max(w99, h99):.0f}")
    
    # Suggest standard sizes
    max_95 = max(w95, h95)
    max_99 = max(w99, h99)
    
    standard_sizes = [32, 48, 64, 96, 128]
    
    print(f"\nSuggested standard image sizes:")
    for size in standard_sizes:
        pct_captured = 100 * np.sum((widths <= size) & (heights <= size)) / len(widths)
        print(f"  {size}x{size}: captures {pct_captured:.1f}% of clusters")
    
    # Padding vs cropping analysis
    print(f"\nIf clusters exceed image size:")
    print(f"  - Option 1: Crop (may lose edge information)")
    print(f"  - Option 2: Downsample (may lose spatial resolution)")
    print(f"  - Recommendation: Choose size to minimize cropping (~95-99% captured)")
