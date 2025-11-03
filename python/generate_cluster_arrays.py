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
import re


def load_display_parameters(repo_root=None):
    """
    Load display parameters from parameters/display.dat
    Returns dict with threshold_adc_x, threshold_adc_u, threshold_adc_v
    """
    if repo_root is None:
        repo_root = Path(__file__).parent.parent
    else:
        repo_root = Path(repo_root)
    
    display_dat = repo_root / "parameters" / "display.dat"
    
    if not display_dat.exists():
        # Return defaults if file not found
        return {
            'threshold_adc_x': 60.0,
            'threshold_adc_u': 70.0,
            'threshold_adc_v': 70.0
        }
    
    params = {}
    with open(display_dat, 'r') as f:
        for line in f:
            line = line.strip()
            # Parse lines like: < display.threshold_adc_x = 60 >
            match = re.match(r'<\s*display\.(\w+)\s*=\s*([^>]+?)\s*>', line)
            if match:
                key = match.group(1)
                value_str = match.group(2).strip().strip('"')
                # Try to convert to float, fallback to string
                try:
                    value = float(value_str)
                except ValueError:
                    value = value_str
                params[key] = value
    
    # Ensure we have all required parameters with defaults
    params.setdefault('threshold_adc_x', 60.0)
    params.setdefault('threshold_adc_u', 70.0)
    params.setdefault('threshold_adc_v', 70.0)
    
    return params


def load_conversion_factors(repo_root=None):
    """
    Load ADC to energy conversion factors from parameters/conversion.dat
    Returns dict with adc_to_mev_collection and adc_to_mev_induction
    """
    if repo_root is None:
        repo_root = Path(__file__).parent.parent
    else:
        repo_root = Path(repo_root)
    
    conversion_dat = repo_root / "parameters" / "conversion.dat"
    
    if not conversion_dat.exists():
        # Return defaults if file not found (typical DUNE values)
        return {
            'adc_to_mev_collection': 3600.0,  # ADC per MeV for collection (X) plane
            'adc_to_mev_induction': 900.0      # ADC per MeV for induction (U, V) planes
        }
    
    params = {}
    with open(conversion_dat, 'r') as f:
        for line in f:
            line = line.strip()
            # Parse lines like: < conversion.adc_to_energy_factor_collection = 3600.0 >
            match_collection = re.match(r'<\s*conversion\.adc_to_energy_factor_collection\s*=\s*([^>]+?)\s*>', line)
            match_induction = re.match(r'<\s*conversion\.adc_to_energy_factor_induction\s*=\s*([^>]+?)\s*>', line)
            
            if match_collection:
                params['adc_to_mev_collection'] = float(match_collection.group(1).strip())
            elif match_induction:
                params['adc_to_mev_induction'] = float(match_induction.group(1).strip())
    
    # Ensure we have both parameters with defaults
    params.setdefault('adc_to_mev_collection', 3600.0)
    params.setdefault('adc_to_mev_induction', 900.0)
    
    return params


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
    
    # Auto-generate from tpstream_folder if clusters_folder not specified
    outfolder = j.get("clusters_folder", "")
    if not outfolder:
        outfolder = j.get("tpstream_folder", ".")
        if outfolder.endswith('/'):
            outfolder = outfolder[:-1]
    else:
        outfolder = outfolder.rstrip('/')
    
    def sanitize(value):
        """
        Sanitize numeric value for filesystem.
        Keeps at most 1 digit after decimal, replaces '.' with 'p'
        Matches C++ logic: std::to_string() then sanitize
        
        IMPORTANT: C++ std::to_string always includes decimals for floats,
        so we must match that behavior. E.g., 2.0 -> "2p0" not "2"
        """
        # Convert to string (for floats, Python shows minimal representation)
        # but we need to match C++ which always shows decimal for float types
        if isinstance(value, float):
            # Format with at least 1 decimal place to match C++ std::to_string behavior
            s = f"{value:.6f}"  # C++ typically gives 6 decimals
        else:
            s = str(value)
        
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


def get_matched_clusters_folder(json_config):
    """Get the matched_clusters folder path from JSON config."""
    if isinstance(json_config, (str, Path)):
        with open(json_config, 'r') as f:
            j = json.load(f)
    else:
        j = json_config
    
    # Get matched_clusters_folder from JSON
    if 'matched_clusters_folder' in j and j['matched_clusters_folder']:
        return j['matched_clusters_folder']
    
    # Auto-generate from clusters_folder
    clusters_folder = get_clusters_folder(json_config)
    matched_folder = clusters_folder.replace('/clusters_', '/matched_clusters_')
    return matched_folder


def get_images_folder(json_config):
    """
    Compose images folder name from JSON configuration.
    Mirrors get_clusters_folder but for images output.
    
    Args:
        json_config: Dict with clustering parameters or path to JSON file
        
    Returns:
        str: Full path to images folder
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
    
    # Auto-generate from tpstream_folder if clusters_folder not specified
    outfolder = j.get("clusters_folder", "")
    if not outfolder:
        outfolder = j.get("tpstream_folder", ".")
        if outfolder.endswith('/'):
            outfolder = outfolder[:-1]
    else:
        outfolder = outfolder.rstrip('/')
    
    def sanitize(value):
        """
        Sanitize numeric value for filesystem.
        Keeps at most 1 digit after decimal, replaces '.' with 'p'
        Matches C++ logic: std::to_string() then sanitize
        
        IMPORTANT: C++ std::to_string always includes decimals for floats,
        so we must match that behavior. E.g., 2.0 -> "2p0" not "2"
        """
        # Convert to string (for floats, Python shows minimal representation)
        # but we need to match C++ which always shows decimal for float types
        if isinstance(value, float):
            # Format with at least 1 decimal place to match C++ std::to_string behavior
            s = f"{value:.6f}"  # C++ typically gives 6 decimals
        else:
            s = str(value)
        
        # Keep at most one digit after decimal point
        if '.' in s:
            parts = s.split('.')
            if len(parts[1]) > 1:
                s = f"{parts[0]}.{parts[1][0]}"
        
        # Replace '.' with 'p' for filesystem safety
        s = s.replace('.', 'p')
        return s
    
    # Build subfolder name matching C++ format
    images_subfolder = (
        f"images_{cluster_prefix}"
        f"_tick{sanitize(tick_limit)}"
        f"_ch{sanitize(channel_limit)}"
        f"_min{sanitize(min_tps_to_cluster)}"
        f"_tot{sanitize(tot_cut)}"
        f"_e{sanitize(energy_cut)}"
    )
    
    images_folder_path = f"{outfolder}/{images_subfolder}"
    return images_folder_path


def calculate_pentagon_params(time_start, time_peak, time_end, adc_peak, adc_integral, threshold_adc):
    """
    Calculate pentagon parameters (matching Display.cpp logic)
    Pentagon has a threshold baseline (plateau) at the bottom
    Returns: (time_int_rise, time_int_fall, h_int_rise, h_int_fall, threshold, valid)
    
    Args:
        threshold_adc: The ADC threshold for the plane (60 for X, 70 for U/V) - this is the plateau height
    """
    if time_end <= time_start:
        return (0, 0, 0, 0, 0, False)
    
    # Total duration
    T = time_end - time_start
    
    # The threshold/plateau is the ADC threshold for the plane
    # X plane (collection): 60 ADC
    # U/V planes (induction): 70 ADC
    threshold = threshold_adc
    
    # Peak position relative to start (fraction)
    t_peak_rel = (time_peak - time_start) / T if T > 0 else 0.5
    t_peak_rel = max(0.0, min(1.0, t_peak_rel))
    
    # Intermediate point positions (fraction of total duration)
    frac = 0.5  # Fixed fraction for intermediate vertices
    t1 = frac * t_peak_rel  # Rising intermediate point
    t2 = t_peak_rel + frac * (1.0 - t_peak_rel)  # Falling intermediate point
    
    # Convert back to absolute times - INTEGERS for time (x-axis)
    time_int_rise = int(round(time_start + t1 * T))
    time_int_fall = int(round(time_start + t2 * T))
    
    # Calculate heights at intermediate points - keep as float (ADC values)
    # Pentagon vertices: (time_start, threshold), (time_int_rise, h_int_rise), 
    #                    (time_peak, adc_peak), (time_int_fall, h_int_fall), (time_end, threshold)
    # These rise from the threshold baseline
    h_int_rise = threshold + 0.6 * (adc_peak - threshold)
    h_int_fall = threshold + 0.6 * (adc_peak - threshold)
    
    return (time_int_rise, time_int_fall, h_int_rise, h_int_fall, threshold, True)


def draw_cluster_to_array(channels, times, adc_integrals, adc_peaks, sot_values, stopeak_values,
                          plane_threshold, img_width=16, img_height=128):
    """
    Draw cluster to numpy array with actual ADC values (NOT normalized)
    
    - X axis: Channel (consecutive, no artificial spacing)
    - Y axis: Time (consecutive ticks)
    - Pixel values: Raw ADC intensity (physical units, NOT normalized to [0,1])
    - Pentagon interpolation for ADC values over time
    - Padding added around cluster if smaller than image size
    - Image size: 16 (channels) x 128 (time ticks) to match typical cluster topology
    
    IMPORTANT: Pixel values represent actual ADC counts and have physical meaning!
    
    Args:
        adc_peaks: Actual ADC peak values from detector data (NOT estimated)
        plane_threshold: The ADC threshold for this plane (60 for X, 70 for U/V) - the plateau baseline
    """
    if len(channels) == 0:
        return np.zeros((img_height, img_width), dtype=np.float32)
    
    channels = np.array(channels, dtype=np.int32)
    times = np.array(times, dtype=np.float64)
    adc_integrals = np.array(adc_integrals, dtype=np.float64)
    adc_peaks = np.array(adc_peaks, dtype=np.float64)  # Use actual peaks from data
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
        adc_peak = adc_peaks[i]  # Use actual peak from detector data!
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
            # Use plane-specific threshold: X=60, U=70, V=70 ADC
            params = calculate_pentagon_params(time_start, time_peak, time_end, 
                                               adc_peak, adc_integral, plane_threshold)
            if not params[5]:  # Check valid flag (now at index 5)
                continue
            
            time_int_rise, time_int_fall, h_int_rise, h_int_fall, threshold, _ = params
            intensity = threshold  # Default to threshold baseline
            
            # CRITICAL: First and last samples should be at threshold (the baseline)
            if t == time_start or t == time_end:
                intensity = threshold
            elif t < time_int_rise:
                # Rising from threshold to h_int_rise
                span = time_int_rise - time_start
                if span > 0:
                    frac = (t - time_start) / span
                    intensity = threshold + frac * (h_int_rise - threshold)
                else:
                    intensity = threshold
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

def extract_clusters_from_file(cluster_file, repo_root=None, verbose=False):
    """
    Extract all clusters from a single ROOT file, organized by plane.
    
    Returns:
        dict: {plane_letter: {'images': [...], 'metadata': [...]}}
    """
    cluster_file = Path(cluster_file)
    
    # Load display parameters
    display_params = load_display_parameters(repo_root)
    
    # Load conversion factors
    conversion_factors = load_conversion_factors(repo_root)
    
    threshold_map = {
        'U': display_params['threshold_adc_u'],
        'V': display_params['threshold_adc_v'],
        'X': display_params['threshold_adc_x']
    }
    
    result = {}
    
    # Open ROOT file
    try:
        file = uproot.open(cluster_file)
    except Exception as e:
        if verbose:
            print(f"  Error opening file: {e}")
        return result
    
    # Access clusters directory
    try:
        clusters_dir = file['clusters']
    except KeyError:
        if verbose:
            print(f"  No 'clusters' directory found")
        return result
    
    # Find cluster trees
    tree_names = [k for k in clusters_dir.keys() if 'clusters_tree_' in k]
    if not tree_names:
        if verbose:
            print(f"  No cluster trees found")
        return result
    
    plane_map = {'U': 0, 'V': 1, 'X': 2}
    
    for tree_name in tree_names:
        # Extract plane letter (handle ROOT versioning like 'clusters_tree_U;1')
        plane_letter = tree_name.split('_')[-1].split(';')[0]  # Get 'U' from 'U;1'
        plane_number = plane_map.get(plane_letter, 0)
        plane_threshold = threshold_map.get(plane_letter, 60.0)
        
        tree = clusters_dir[tree_name]
        n_entries = tree.num_entries
        
        if n_entries == 0:
            continue
        
        # Read required branches
        branches = [
            'event',
            'tp_detector_channel', 'tp_time_start', 'tp_adc_integral',
            'tp_samples_over_threshold', 'tp_samples_to_peak', 'tp_adc_peak',
            'marley_tp_fraction', 'is_main_cluster',
            'true_pos_x', 'true_pos_y', 'true_pos_z',
            'true_dir_x', 'true_dir_y', 'true_dir_z',
            'true_mom_x', 'true_mom_y', 'true_mom_z',
            'true_neutrino_energy', 'true_particle_energy', 'is_es_interaction'
        ]
        data = tree.arrays(branches, library='np')
        
        images = []
        metadata = []
        
        for i in range(n_entries):
            try:
                # Get TP arrays
                channels = data['tp_detector_channel'][i]
                times = data['tp_time_start'][i] / 32
                adc_integrals = data['tp_adc_integral'][i]
                adc_peaks = data['tp_adc_peak'][i]
                sot_values = data['tp_samples_over_threshold'][i]
                stopeak_values = data['tp_samples_to_peak'][i]
                
                if len(channels) == 0:
                    continue
                
                # Extract metadata
                is_marley = data['marley_tp_fraction'][i] > 0.5
                is_main_track = bool(data['is_main_cluster'][i])
                
                true_pos = np.array([
                    data['true_pos_x'][i],
                    data['true_pos_y'][i],
                    data['true_pos_z'][i]
                ], dtype=np.float32)
                
                true_particle_mom = np.array([
                    data['true_mom_x'][i],
                    data['true_mom_y'][i],
                    data['true_mom_z'][i]
                ], dtype=np.float32)
                
                true_nu_energy = float(data['true_neutrino_energy'][i])
                true_particle_energy = float(data['true_particle_energy'][i])
                
                # Read boolean directly
                is_es_interaction = 1.0 if bool(data['is_es_interaction'][i]) else 0.0
                
                # Generate image
                img_array = draw_cluster_to_array(
                    channels, times, adc_integrals, adc_peaks,
                    sot_values, stopeak_values,
                    plane_threshold=plane_threshold,
                    img_width=16, img_height=128
                )
                
                # Calculate cluster energy from ADC sum (instead of using MC truth)
                # Use plane-specific conversion factor (ADC per MeV)
                total_adc = float(np.sum(img_array))
                conversion_factor = conversion_factors['adc_to_mev_collection'] if plane_letter == 'X' else conversion_factors['adc_to_mev_induction']
                cluster_energy_mev = total_adc / conversion_factor  # MeV
                
                # Prepare metadata
                # Note: cluster_energy is now ADC-derived, not MC truth neutrino energy
                metadata_array = np.array([
                    int(data['event'][i]),
                    int(is_marley),
                    int(is_main_track),
                    is_es_interaction,
                    true_pos[0], true_pos[1], true_pos[2],
                    true_particle_mom[0], true_particle_mom[1], true_particle_mom[2],
                    np.float32(cluster_energy_mev),
                    np.float32(true_particle_energy),
                    plane_number
                ], dtype=np.float32)
                
                images.append(img_array)
                metadata.append(metadata_array)
                
            except Exception as e:
                if verbose:
                    print(f"  Warning: Failed cluster {i} on plane {plane_letter}: {e}")
                continue
        
        if len(images) > 0:
            result[plane_letter] = {
                'images': images,
                'metadata': metadata
            }
            if verbose:
                print(f"    Plane {plane_letter}: {len(images)} clusters")
    
    return result

def generate_images(cluster_file, output_dir, draw_mode='pentagon', repo_root=None, verbose=False):
    """
    Generate images for all clusters in a ROOT file and save one NPZ per plane.
    
    Args:
        cluster_file: Path to ROOT file with clusters
        output_dir: Directory to save NPZ files
        draw_mode: Drawing mode for cluster visualization (default: 'pentagon')
        repo_root: Optional repository root path
        verbose: Enable verbose output (default: False)
        
    Returns:
        dict: Statistics about generated arrays per plane {plane: count}
    """
    
    cluster_file = Path(cluster_file)
    output_dir = Path(output_dir)
    output_dir.mkdir(exist_ok=True, parents=True)
    
    # Extract base name without path and _clusters.root suffix
    input_basename = cluster_file.stem.replace('_clusters', '')
    
    # Load display parameters from parameters/display.dat
    display_params = load_display_parameters(repo_root)
    
    # Load conversion factors from parameters/conversion.dat
    conversion_factors = load_conversion_factors(repo_root)
    
    # Thresholds per plane (U, V, X/Collection)
    threshold_map = {
        'U': display_params['threshold_adc_u'],
        'V': display_params['threshold_adc_v'],
        'X': display_params['threshold_adc_x']
    }
    
    if verbose:
        print(f"Processing: {cluster_file.name}")
        print(f"  Thresholds: U={threshold_map['U']}, V={threshold_map['V']}, X={threshold_map['X']}")
    
    # Open ROOT file
    try:
        file = uproot.open(cluster_file)
    except Exception as e:
        print(f"  Error opening file: {e}")
        return {}
    
    # Get clusters directory
    if 'clusters' not in file:
        print(f"  No 'clusters' directory found")
        return {}
    
    clusters_dir = file['clusters']
    
    # Find all cluster trees (one per plane: U, V, X)
    tree_names = [k.split(';')[0] for k in clusters_dir.keys() if 'clusters_tree_' in k]
    
    if not tree_names:
        print(f"  No cluster trees found")
        return {}

    if verbose:
        print(f"  Found {len(tree_names)} planes with clusters")

    plane_stats = {}  # Track generated clusters per plane
    plane_map = {'U': 0, 'V': 1, 'X': 2}  # Plane names to numbers
    
    for tree_name in tree_names:
        # Extract plane from tree name (e.g., clusters_tree_U -> U)
        plane_letter = tree_name.split('_')[-1]
        plane_number = plane_map.get(plane_letter, 0)
        
        tree = clusters_dir[tree_name]
        n_entries = tree.num_entries
        
        if n_entries == 0:
            continue
        
        if verbose:
            print(f"  Processing plane {plane_letter}: {n_entries} clusters...")
        
        # Get threshold for this plane
        plane_threshold = threshold_map.get(plane_letter, 60.0)
        if verbose:
            print(f"    Using threshold = {plane_threshold} ADC for plane {plane_letter}")
        
        # Load all data at once for efficiency
        branches = [
            'event',
            'tp_detector_channel', 'tp_time_start', 'tp_adc_integral',
            'tp_samples_over_threshold', 'tp_samples_to_peak',
            'tp_adc_peak',  # Use actual ADC peak from data!
            # Cluster-level metadata
            'marley_tp_fraction', 'is_main_cluster',
            'true_pos_x', 'true_pos_y', 'true_pos_z',
            'true_dir_x', 'true_dir_y', 'true_dir_z',  # Particle direction (normalized)
            'true_mom_x', 'true_mom_y', 'true_mom_z',  # Particle momentum [GeV/c]
            'true_neutrino_energy', 'true_particle_energy',
            'is_es_interaction'  # Interaction type: boolean (True=ES, False=CC)
        ]
        data = tree.arrays(branches, library='np')
        
        # Collect all images and metadata for this plane
        plane_images = []
        plane_metadata = []
        n_generated_plane = 0  # Track for this plane
        
        for i in range(n_entries):
            try:
                # Get TP arrays for this cluster
                channels = data['tp_detector_channel'][i]
                times = data['tp_time_start'][i] / 32  # Convert to TPC ticks
                adc_integrals = data['tp_adc_integral'][i]
                adc_peaks = data['tp_adc_peak'][i]  # Use actual peak values from data
                sot_values = data['tp_samples_over_threshold'][i]
                stopeak_values = data['tp_samples_to_peak'][i]
                
                if len(channels) == 0:
                    continue
                
                # Extract cluster metadata
                is_marley = data['marley_tp_fraction'][i] > 0.5  # Marley if >50% of TPs are from Marley
                is_main_track = bool(data['is_main_cluster'][i])
                
                # True position (3D)
                true_pos = np.array([
                    data['true_pos_x'][i],
                    data['true_pos_y'][i],
                    data['true_pos_z'][i]
                ], dtype=np.float32)
                
                # Particle direction (3D, normalized)
                true_dir = np.array([
                    data['true_dir_x'][i],
                    data['true_dir_y'][i],
                    data['true_dir_z'][i]
                ], dtype=np.float32)
                
                # Particle momentum (3D) [GeV/c]
                true_particle_mom = np.array([
                    data['true_mom_x'][i],
                    data['true_mom_y'][i],
                    data['true_mom_z'][i]
                ], dtype=np.float32)
                
                # Energy information
                true_nu_energy = float(data['true_neutrino_energy'][i])
                true_particle_energy = float(data['true_particle_energy'][i])
                
                # Interaction type: ES (1) or CC (0)
                is_es_interaction = 1.0 if bool(data['is_es_interaction'][i]) else 0.0
                
                # Generate 16x128 numpy array with RAW ADC values (not normalized)
                # X=channel (16 pixels), Y=time (128 pixels)
                # Pixel values = actual ADC intensity with physical meaning
                # Use plane-specific threshold (60 for X, 70 for U/V)
                img_array = draw_cluster_to_array(
                    channels, times, adc_integrals, adc_peaks,
                    sot_values, stopeak_values,
                    plane_threshold=plane_threshold,
                    img_width=16, img_height=128
                )
                
                # Calculate cluster energy from ADC sum (instead of using MC truth)
                # Use plane-specific conversion factor (ADC per MeV)
                total_adc = float(np.sum(img_array))
                conversion_factor = conversion_factors['adc_to_mev_collection'] if plane_letter == 'X' else conversion_factors['adc_to_mev_induction']
                cluster_energy_mev = total_adc / conversion_factor  # MeV
                
                # Prepare metadata as compact array
                # Format: [event, is_marley, is_main_track, is_es_interaction, pos(3),
                #          particle_mom(3), cluster_energy, particle_energy, plane_id]
                # All stored as float32 for efficiency (0.0/1.0 for booleans)
                # Note: cluster_energy is now ADC-derived, not MC truth neutrino energy
                plane_id = {'U': 0, 'V': 1, 'X': 2}.get(plane_letter, 0)
                metadata_array = np.array([
                    int(data['event'][i]),
                    int(is_marley),
                    int(is_main_track),
                    is_es_interaction,
                    true_pos[0], true_pos[1], true_pos[2],
                    true_particle_mom[0], true_particle_mom[1], true_particle_mom[2],
                    np.float32(cluster_energy_mev),
                    np.float32(true_particle_energy),
                    plane_id
                ], dtype=np.float32)                # Add to plane collection
                plane_images.append(img_array)
                plane_metadata.append(metadata_array)
                n_generated_plane += 1
                
                # Progress indicator
                if verbose:
                    if (n_generated_plane % 100) == 0 or (i + 1) == n_entries:
                        print(f"    Generated {n_generated_plane} arrays total...", end='\r')
                    
            except Exception as e:
                print(f"\n  Warning: Failed to generate array for cluster {i} on plane {plane_letter}: {e}")
                continue
        
        # Save all clusters for this plane to a single file
        if len(plane_images) > 0:
            output_file = output_dir / f"{input_basename}_plane{plane_letter}.npz"
            np.savez(output_file, 
                    images=np.array(plane_images, dtype=np.float32),
                    metadata=np.array(plane_metadata, dtype=np.float32))
            if verbose:
                print(f"\n    Saved {len(plane_images)} clusters to {output_file.name}")
        
        plane_stats[plane_letter] = n_generated_plane

    if verbose:
        total_generated = sum(plane_stats.values())
        print(f"\n  Successfully generated {total_generated} numpy arrays total")
    return plane_stats

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate cluster numpy arrays (16x128) for NN input with 1:1 file mapping')
    parser.add_argument('cluster_files', nargs='*', help='Cluster ROOT files (or use --json)')
    parser.add_argument('--output-dir', help='Output directory for numpy arrays')
    parser.add_argument('--draw-mode', default='pentagon', 
                       choices=['pentagon', 'triangle', 'rectangle'],
                       help='Drawing mode (kept for compatibility, not used)')
    parser.add_argument('--root-dir', help='Repository root directory')
    parser.add_argument('--json', '-j', help='JSON config file (auto-detects clusters folder)')
    parser.add_argument('--skip-files', type=int, help='Skip first N cluster files (overrides JSON)')
    parser.add_argument('--max-files', type=int, help='Process at most N cluster files (overrides JSON)')
    parser.add_argument('-f', '--override', action='store_true', help='Force reprocessing even if output files already exist')
    parser.add_argument('-v', '--verbose', action='store_true', help='Enable verbose output')
    
    args = parser.parse_args()
    
    # Determine output directory
    if args.json:
        # Use JSON to determine clusters folder and images folder
        json_path = Path(args.json)
        if not json_path.exists():
            print(f"Error: JSON file not found: {json_path}")
            sys.exit(1)
        
        # Load JSON configuration
        with open(json_path, 'r') as f:
            json_config = json.load(f)
        
        # Try to use matched clusters folder if it exists
        matched_clusters_folder = get_matched_clusters_folder(json_path)
        if matched_clusters_folder and Path(matched_clusters_folder).exists():
            matched_files = list(Path(matched_clusters_folder).glob("*_matched.root"))
            if matched_files:
                print(f"Using matched clusters folder: {matched_clusters_folder}")
                clusters_folder = matched_clusters_folder
                file_pattern = "*_matched.root"
            else:
                clusters_folder = get_clusters_folder(json_path)
                file_pattern = "*_clusters.root"
        else:
            clusters_folder = get_clusters_folder(json_path)
            file_pattern = "*_clusters.root"
        
        images_folder = get_images_folder(json_path)
        output_dir = args.output_dir if args.output_dir else images_folder
        
        # Find cluster files in the computed folder
        cluster_files = list(Path(clusters_folder).glob(file_pattern))
        if not cluster_files:
            print(f"Error: No *_clusters.root files found in {clusters_folder}")
            sys.exit(1)
        
        # Sort for consistent ordering
        cluster_files = sorted(cluster_files)
        
        # Read skip_files and max_files from JSON if not provided via CLI
        skip_files = args.skip_files if args.skip_files is not None else json_config.get('skip_files', 0)
        max_files = args.max_files if args.max_files is not None else json_config.get('max_files', None)
        
        # Apply skip and max filters
        if skip_files > 0:
            cluster_files = cluster_files[skip_files:]
            print(f"Skipping first {skip_files} files (from {'CLI' if args.skip_files is not None else 'JSON'})")
        
        if max_files is not None:
            cluster_files = cluster_files[:max_files]
            print(f"Processing at most {max_files} files (from {'CLI' if args.max_files is not None else 'JSON'})")
        
        cluster_files = [str(f) for f in cluster_files]
        
        print(f"Using JSON config: {json_path}")
        print(f"Computed clusters folder: {clusters_folder}")
        print(f"Computed images folder: {images_folder}")
        print(f"Output directory: {output_dir}")
        print(f"Processing {len(cluster_files)} cluster file(s)")
        
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
        
        # Apply skip and max filters from CLI
        skip_files = args.skip_files if args.skip_files is not None else 0
        max_files = args.max_files
        
        if skip_files > 0:
            cluster_files = cluster_files[skip_files:]
            print(f"Skipping first {skip_files} files")
        
        if max_files is not None:
            cluster_files = cluster_files[:max_files]
            print(f"Processing at most {max_files} files")
    
    # Check for existing output files and handle override mode
    output_path = Path(output_dir)
    if args.override:
        # Override mode: delete existing NPZ files
        existing_files = list(output_path.glob('*_plane*.npz'))
        if existing_files:
            print(f"\nOverride mode: Deleting {len(existing_files)} existing file(s)")
            for f in existing_files:
                f.unlink()
    else:
        # Check for existing files to potentially skip
        existing_files = list(output_path.glob('*_plane*.npz'))
        if existing_files:
            print(f"\nFound {len(existing_files)} existing output file(s)")
            print(f"Processing will skip files that already have outputs")
            print(f"Use --override/-f to force reprocessing all files from scratch\n")
    
    # Ensure output directory exists
    output_path.mkdir(exist_ok=True, parents=True)
    
    # Set repo root (optional)
    repo_root = Path(args.root_dir) if args.root_dir else None
    
    # Process files one at a time (1:1 mapping)
    total_generated = 0
    plane_totals = {}
    processed_count = 0
    skipped_count = 0
    
    for cluster_file in cluster_files:
        cluster_file = Path(cluster_file)
        input_basename = cluster_file.stem.replace('_clusters', '')
        
        # Check if output already exists (unless override mode)
        if not args.override:
            output_exists = any(output_path.glob(f"{input_basename}_plane*.npz"))
            if output_exists:
                if args.verbose:
                    print(f"\nSkipping {cluster_file.name} (output already exists)")
                skipped_count += 1
                continue
        
        if args.verbose:
            print(f"\nProcessing file {processed_count + 1}/{len(cluster_files)}: {cluster_file.name}")
        
        # Generate images for this file
        plane_stats = generate_images(
            cluster_file=cluster_file,
            output_dir=output_path,
            draw_mode='pentagon',
            repo_root=repo_root,
            verbose=args.verbose
        )
        
        # Update totals
        for plane, count in plane_stats.items():
            plane_totals[plane] = plane_totals.get(plane, 0) + count
            total_generated += count
        
        processed_count += 1
    
    print(f"\nProcessing complete:")
    print(f"  Files processed: {processed_count}")
    if skipped_count > 0:
        print(f"  Files skipped (already exist): {skipped_count}")
    print(f"  Total clusters converted: {total_generated}")
    if plane_totals:
        print(f"  Per plane: {dict(plane_totals)}")
    print(f"Output directory: {output_dir}")

