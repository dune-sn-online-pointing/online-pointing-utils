#!/usr/bin/env python3
"""
Batch cluster array generator for neural network input

Generates 16x128 numpy arrays from cluster ROOT files:
- X axis: Channel (detector channels)
- Y axis: Time (detector ticks)
- Pixel values: Raw ADC intensity (physical units, NOT normalized)
- Pentagon interpolation for ADC values over time

IMPORTANT: Arrays contain actual ADC counts with physical meaning!
Use these raw values for training to preserve energy information.
"""

import sys
import os
import json
from pathlib import Path
import argparse
import uproot
import numpy as np


def get_clusters_folder(json_config):
    """
    Compose clusters folder name from JSON configuration.
    Matches the logic in src/lib/utils.cpp::getClustersFolder()
    
    Args:
        json_config: Dict with clustering parameters or path to JSON file
        
    Returns:
        str: Full path to clusters folder
    """
    # Load JSON if path provided
    if isinstance(json_config, (str, Path)):
        with open(json_config, 'r') as f:
            j = json.load(f)
    else:
        j = json_config
    
    # Extract parameters with defaults matching C++ code
    cluster_prefix = j.get("clusters_folder_prefix", "clusters")
    tick_limit = j.get("tick_limit", 0)
    channel_limit = j.get("channel_limit", 0)
    min_tps_to_cluster = j.get("min_tps_to_cluster", 0)
    tot_cut = j.get("tot_cut", 0)
    energy_cut = float(j.get("energy_cut", 0.0))
    outfolder = j.get("clusters_folder", ".").rstrip('/')
    
    def sanitize(value):
        """
        Sanitize numeric value for filesystem.
        Keeps at most 1 digit after decimal, replaces '.' with 'p'
        Matches C++ logic: std::to_string() then sanitize
        """
        s = str(value)
        
        # For integers or .0 floats, remove decimal point entirely
        if isinstance(value, int) or (isinstance(value, float) and value == int(value)):
            s = str(int(value))
            return s
        
        # Keep at most one digit after decimal point
        if '.' in s:
            parts = s.split('.')
            if len(parts[1]) > 1:
                s = f"{parts[0]}.{parts[1][0]}"
        
        # Replace '.' with 'p' for filesystem safety
        s = s.replace('.', 'p')
        return s
    
    # Build subfolder name matching C++ format
    clusters_subfolder = (
        f"clusters_{cluster_prefix}"
        f"_tick{sanitize(tick_limit)}"
        f"_ch{sanitize(channel_limit)}"
        f"_min{sanitize(min_tps_to_cluster)}"
        f"_tot{sanitize(tot_cut)}"
        f"_e{sanitize(energy_cut)}"
    )
    
    clusters_folder_path = f"{outfolder}/{clusters_subfolder}"
    return clusters_folder_path


def calculate_pentagon_params(time_start, time_peak, time_end, adc_peak, adc_integral, offset=0.5):
    """
    Calculate pentagon parameters (matching Display.cpp logic)
    Pentagon has a threshold baseline (plateau) at the bottom
    Returns: (time_int_rise, time_int_fall, h_int_rise, h_int_fall, threshold, valid)
    """
    if time_end <= time_start:
        return (0, 0, 0, 0, 0, False)
    
    # Total duration
    T = time_end - time_start
    
    # Calculate threshold (baseline/plateau height)
    # This is the "plateau" you mentioned - pentagon sits on this baseline
    threshold = offset / T if T > 0 else 0.0
    
    # Peak position relative to start (fraction)
    t_peak_rel = (time_peak - time_start) / T if T > 0 else 0.5
    t_peak_rel = max(0.0, min(1.0, t_peak_rel))
    
    # Intermediate point positions (fraction of total duration)
    t1 = offset * t_peak_rel  # Rising intermediate point
    t2 = t_peak_rel + offset * (1.0 - t_peak_rel)  # Falling intermediate point
    
    # Convert back to absolute times
    time_int_rise = time_start + t1 * T
    time_int_fall = time_start + t2 * T
    
    # Calculate heights at intermediate points
    # Pentagon vertices: (time_start, threshold), (time_int_rise, h_int_rise), 
    #                    (time_peak, adc_peak), (time_int_fall, h_int_fall), (time_end, threshold)
    # These rise from the threshold baseline
    h_int_rise = threshold + 0.6 * (adc_peak - threshold)
    h_int_fall = threshold + 0.6 * (adc_peak - threshold)
    
    return (time_int_rise, time_int_fall, h_int_rise, h_int_fall, threshold, True)


def draw_cluster_to_array(channels, times, adc_integrals, sot_values, stopeak_values, img_width=16, img_height=128):
    """
    Draw cluster to numpy array with actual ADC values (NOT normalized)
    
    - X axis: Channel (consecutive, no artificial spacing)
    - Y axis: Time (consecutive ticks)
    - Pixel values: Raw ADC intensity (physical units, NOT normalized to [0,1])
    - Pentagon interpolation for ADC values over time
    - Padding added around cluster if smaller than image size
    - Image size: 16 (channels) x 128 (time ticks) to match typical cluster topology
    
    IMPORTANT: Pixel values represent actual ADC counts and have physical meaning!
    """
    if len(channels) == 0:
        return np.zeros((img_height, img_width), dtype=np.float32)
    
    channels = np.array(channels, dtype=np.int32)
    times = np.array(times, dtype=np.float64)
    adc_integrals = np.array(adc_integrals, dtype=np.float64)
    sot_values = np.array(sot_values, dtype=np.int32) if len(sot_values) > 0 else np.ones(len(times), dtype=np.int32) * 10
    stopeak_values = np.array(stopeak_values, dtype=np.int32) if len(stopeak_values) > 0 else (sot_values // 2)
    
    # Get actual bounds (no artificial scaling)
    ch_min, ch_max = channels.min(), channels.max()
    time_min = times.min()
    time_ends = times + sot_values
    time_max = time_ends.max()
    
    # Calculate actual cluster size in detector units
    n_channels_actual = ch_max - ch_min + 1
    n_ticks_actual = int(time_max - time_min + 1)
    
    # Map channels to consecutive indices (0, 1, 2, ...)
    unique_channels = np.sort(np.unique(channels))
    ch_to_idx = {ch: idx for idx, ch in enumerate(unique_channels)}
    n_channels = len(unique_channels)
    
    # Calculate padding to center the cluster in the image
    pad_x = max(0, (img_width - n_channels) // 2)
    pad_y = max(0, (img_height - n_ticks_actual) // 2)
    
    # Create empty image (Y=time, X=channel like in display.cpp)
    img = np.zeros((img_height, img_width), dtype=np.float32)
    
    # Draw each TP
    for i in range(len(channels)):
        ch = channels[i]
        ch_idx = ch_to_idx[ch]
        time_start = times[i]
        sot = sot_values[i]
        time_end = time_start + sot
        stopeak = stopeak_values[i]
        time_peak = time_start + stopeak
        adc_peak = adc_integrals[i] / max(sot, 1) * 2  # Approximate peak from integral
        adc_integral = adc_integrals[i]
        
        # Fill pentagon with padding offsets
        for t in range(int(time_start), int(time_end) + 1):
            # Calculate position with padding
            x = ch_idx + pad_x
            y = int(t - time_min) + pad_y
            
            if x < 0 or x >= img_width or y < 0 or y >= img_height:
                continue
            
            # Calculate ADC intensity at this time using pentagon interpolation
            # Pentagon includes a baseline threshold (the "plateau")
            params = calculate_pentagon_params(time_start, time_peak, time_end, adc_peak, adc_integral, offset=0.5)
            if not params[5]:  # Check valid flag (now at index 5)
                continue
            
            time_int_rise, time_int_fall, h_int_rise, h_int_fall, threshold, _ = params
            intensity = threshold  # Start from threshold baseline
            
            if t < time_int_rise:
                # Rising from threshold to h_int_rise
                span = time_int_rise - time_start
                if span > 0:
                    frac = (t - time_start) / span
                    intensity = threshold + frac * (h_int_rise - threshold)
            elif t < time_peak:
                # Rising from h_int_rise to peak
                span = time_peak - time_int_rise
                if span > 0:
                    frac = (t - time_int_rise) / span
                    intensity = h_int_rise + frac * (adc_peak - h_int_rise)
                else:
                    intensity = adc_peak
            elif t == time_peak:
                intensity = adc_peak
            elif t <= time_int_fall:
                # Falling from peak to h_int_fall
                span = time_int_fall - time_peak
                if span > 0:
                    frac = (t - time_peak) / span
                    intensity = adc_peak - frac * (adc_peak - h_int_fall)
                else:
                    intensity = h_int_fall
            else:
                # Falling from h_int_fall back to threshold
                span = time_end - time_int_fall
                if span > 0:
                    frac = (t - time_int_fall) / span
                    intensity = h_int_fall - frac * (h_int_fall - threshold)
            
            img[y, x] = max(img[y, x], intensity)
    
    # Keep raw ADC values - DO NOT normalize!
    # The pixel values represent actual ADC intensities and have physical meaning
    return img

def generate_images(cluster_file, output_dir, draw_mode='pentagon', repo_root=None):
    """Generate images for all clusters in a ROOT file"""
    
    cluster_file = Path(cluster_file)
    output_dir = Path(output_dir)
    output_dir.mkdir(exist_ok=True)
    
    print(f"Processing: {cluster_file.name}")
    
    # Open ROOT file
    try:
        file = uproot.open(cluster_file)
    except Exception as e:
        print(f"  Error opening file: {e}")
        return 0
    
    # Get clusters directory
    if 'clusters' not in file:
        print(f"  No 'clusters' directory found")
        return 0
    
    clusters_dir = file['clusters']
    
    # Find all cluster trees (one per plane: U, V, X)
    tree_names = [k.split(';')[0] for k in clusters_dir.keys() if 'clusters_tree_' in k]
    
    if not tree_names:
        print(f"  No cluster trees found")
        return 0
    
    print(f"  Found {len(tree_names)} planes with clusters")
    
    n_generated = 0
    plane_map = {'U': 0, 'V': 1, 'X': 2}  # Plane names to numbers
    
    for tree_name in tree_names:
        # Extract plane from tree name (e.g., clusters_tree_U -> U)
        plane_letter = tree_name.split('_')[-1]
        plane_number = plane_map.get(plane_letter, 0)
        
        tree = clusters_dir[tree_name]
        n_entries = tree.num_entries
        
        if n_entries == 0:
            continue
        
        print(f"  Processing plane {plane_letter}: {n_entries} clusters...")
        
        # Load all data at once for efficiency
        branches = [
            'tp_detector_channel', 'tp_time_start', 'tp_adc_integral',
            'tp_samples_over_threshold', 'tp_samples_to_peak',
            'marley_tp_fraction', 'total_charge', 'true_label'
        ]
        data = tree.arrays(branches, library='np')
        
        for i in range(n_entries):
            try:
                # Get TP arrays for this cluster
                channels = data['tp_detector_channel'][i]
                times = data['tp_time_start'][i] / 32  # Convert to TPC ticks
                adc_integrals = data['tp_adc_integral'][i]
                sot_values = data['tp_samples_over_threshold'][i]
                stopeak_values = data['tp_samples_to_peak'][i]
                
                if len(channels) == 0:
                    continue
                
                # Generate 16x128 numpy array with RAW ADC values (not normalized)
                # X=channel (16 pixels), Y=time (128 pixels)
                # Pixel values = actual ADC intensity with physical meaning
                img_array = draw_cluster_to_array(
                    channels, times, adc_integrals, 
                    sot_values, stopeak_values,
                    img_width=16, img_height=128
                )
                
                # Save as .npy file (values are RAW ADC counts)
                output_file = output_dir / f"cluster_plane{plane_letter}_{i:04d}.npy"
                np.save(output_file, img_array)
                
                n_generated += 1
                
                # Progress indicator
                if (n_generated % 100) == 0 or (i + 1) == n_entries:
                    print(f"    Generated {n_generated} arrays total...", end='\r')
                    
            except Exception as e:
                print(f"\n  Warning: Failed to generate array for cluster {i} on plane {plane_letter}: {e}")
                continue
    
    print(f"\n  Successfully generated {n_generated} numpy arrays total")
    return n_generated

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Batch generate cluster numpy arrays (16x128) for NN input')
    parser.add_argument('cluster_files', nargs='*', help='Cluster ROOT files (or use --json)')
    parser.add_argument('--output-dir', help='Output directory for numpy arrays')
    parser.add_argument('--draw-mode', default='pentagon', 
                       choices=['pentagon', 'triangle', 'rectangle'],
                       help='Drawing mode (kept for compatibility, not used)')
    parser.add_argument('--root-dir', help='Repository root directory')
    parser.add_argument('--json', '-j', help='JSON config file (auto-detects clusters folder)')
    parser.add_argument('-v', '--verbose', action='store_true', help='Enable verbose output')
    
    args = parser.parse_args()
    
    # Determine output directory
    if args.json:
        # Use JSON to determine clusters folder
        json_path = Path(args.json)
        if not json_path.exists():
            print(f"Error: JSON file not found: {json_path}")
            sys.exit(1)
        
        clusters_folder = get_clusters_folder(json_path)
        output_dir = args.output_dir if args.output_dir else clusters_folder
        
        # Find cluster files in the computed folder
        cluster_files = list(Path(clusters_folder).glob("clusters_*.root"))
        if not cluster_files:
            print(f"Error: No clusters_*.root files found in {clusters_folder}")
            sys.exit(1)
        
        cluster_files = [str(f) for f in cluster_files]
        
        if args.verbose:
            print(f"Using JSON config: {json_path}")
            print(f"Computed clusters folder: {clusters_folder}")
            print(f"Output directory: {output_dir}")
            print(f"Found {len(cluster_files)} cluster file(s)")
        
        print(f"Using JSON config: {json_path}")
        print(f"Computed clusters folder: {clusters_folder}")
        print(f"Found {len(cluster_files)} cluster file(s)")
        
    else:
        # Use command-line arguments
        if not args.cluster_files:
            print("Error: Must provide cluster_files or --json")
            parser.print_help()
            sys.exit(1)
        
        if not args.output_dir:
            print("Error: --output-dir required when not using --json")
            parser.print_help()
            sys.exit(1)
        
        cluster_files = args.cluster_files
        output_dir = args.output_dir
    
    # Set repo root (optional)
    repo_root = Path(args.root_dir) if args.root_dir else None
    
    # Generate arrays
    total_generated = 0
    for cluster_file in cluster_files:
        n = generate_images(cluster_file, output_dir, args.draw_mode, repo_root)
        total_generated += n
    
    print(f"\nTotal numpy arrays generated: {total_generated}")
    print(f"Arrays saved to: {output_dir}")

