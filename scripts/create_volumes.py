#!/usr/bin/env python3
"""
Create 1m x 1m volume images for channel tagging.
"""

import sys
import json
import argparse
from pathlib import Path
import uproot
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches


def get_conditions_string(config):
    """Build conditions string from clustering parameters (matches C++ logic)"""
    def sanitize(value):
        s = str(value)
        if '.' in s:
            parts = s.split('.')
            if len(parts) > 1:
                s = f"{parts[0]}.{parts[1][:1]}"
        return s.replace('.', 'p')
    
    tick_limit = config.get('tick_limit', 0)
    channel_limit = config.get('channel_limit', 0)
    min_tps = config.get('min_tps_to_cluster', 0)
    tot_cut = config.get('tot_cut', 0)
    energy_cut = float(config.get('energy_cut', 0.0))
    
    return (f"tick{sanitize(tick_limit)}_ch{sanitize(channel_limit)}_"
            f"min{sanitize(min_tps)}_tot{sanitize(tot_cut)}_e{sanitize(energy_cut)}")


#!/usr/bin/env python3
"""
create_volumes.py - Create 1m x 1m volume images for channel tagging

Takes cluster ROOT files and creates volume images centered on main track clusters.
Each image is 1m x 1m in physical size, containing all clusters within that volume.
Only processes plane X (collection plane).

Outputs .npz files with image arrays and metadata including:
- Interaction type
- Neutrino energy  
- Main track particle momentum
- Number of marley vs non-marley clusters in volume
"""

import numpy as np
import uproot
import argparse
import json
import os
import sys
from pathlib import Path

# Add lib directory to path
sys.path.append(str(Path(__file__).parent.parent / 'lib'))

# Geometry parameters (from parameters/geometry.dat and parameters/timing.dat)
WIRE_PITCH_COLLECTION_CM = 0.479    # Wire pitch for collection plane (X)
TIME_TICK_CM = 0.0805                # Time tick in cm
TDC_TO_TPC_CONVERSION = 32           # Conversion factor from TDC to TPC ticks

# Volume size
VOLUME_SIZE_CM = 100.0  # 1 meter x 1 meter

# ADC thresholds for pentagon drawing (from Display.cpp)
THRESHOLD_ADC_X = 60   # Collection plane
THRESHOLD_ADC_U = 70   # Induction plane U
THRESHOLD_ADC_V = 70   # Induction plane V


def _read_conversion_factors():
    """Read ADC->MeV conversion factors from parameters/conversion.dat if available.
    Returns (collection_factor, induction_factor).
    Falls back to defaults matching C++ parameters file.
    """
    default_col = 3600.0
    default_ind = 900.0
    conv_file = Path(__file__).resolve().parent.parent / 'parameters' / 'conversion.dat'
    if not conv_file.exists():
        return default_col, default_ind

    col = default_col
    ind = default_ind
    try:
        with open(conv_file, 'r') as f:
            for line in f:
                line = line.strip()
                if 'conversion.adc_to_energy_factor_collection' in line:
                    parts = line.split('=')
                    if len(parts) > 1:
                        col = float(parts[1].strip().strip('> ').strip())
                if 'conversion.adc_to_energy_factor_induction' in line:
                    parts = line.split('=')
                    if len(parts) > 1:
                        ind = float(parts[1].strip().strip('> ').strip())
    except Exception:
        return default_col, default_ind

    return col, ind


ADC_TO_MEV_COLLECTION, ADC_TO_MEV_INDUCTION = _read_conversion_factors()


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
        """
        if isinstance(value, float):
            s = f"{value:.6f}"
        else:
            s = str(value)
        
        if '.' in s:
            parts = s.split('.')
            if len(parts[1]) > 1:
                s = f"{parts[0]}.{parts[1][0]}"
        
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
    """Get matched_clusters folder from JSON config."""
    if isinstance(json_config, (str, Path)):
        with open(json_config, 'r') as f:
            j = json.load(f)
    else:
        j = json_config
    
    matched_folder = j.get("matched_clusters_folder", "").rstrip('/')
    if matched_folder:
        return matched_folder
    
    clusters_folder = get_clusters_folder(json_config)
    matched_folder = clusters_folder.replace("/clusters_", "/matched_clusters_")
    return matched_folder



def _polygon_area(vertices):
    """Compute polygon area via Shoelace formula."""
    area = 0.0
    n = len(vertices)
    for i in range(n):
        x1, y1 = vertices[i]
        x2, y2 = vertices[(i + 1) % n]
        area += x1 * y2 - x2 * y1
    return abs(area) * 0.5


def calculate_pentagon_params(time_start, time_peak, time_end, adc_peak, adc_integral, threshold_adc, frac=0.5, _depth=0):
    """
    Calculate pentagon parameters mirroring Display.cpp behaviour.
    Returns: (time_int_rise, time_int_fall, h_int_rise, h_int_fall, threshold, valid)
    """
    if time_end <= time_start:
        return (0, 0, 0, 0, 0, False)

    threshold = threshold_adc
    target_area = max(float(adc_integral), 0.0)
    adc_peak = max(float(adc_peak), 0.0)
    time_start = float(time_start)
    time_peak = float(time_peak)
    time_end = float(time_end)

    # Clamp peak into waveform window to avoid negative spans
    if time_peak < time_start:
        time_peak = time_start
    if time_peak > time_end:
        time_peak = time_end

    # First attempt: discrete scan matching polygon area to adc_integral
    best = None
    best_diff = float('inf')
    n_samples = 10
    span_left = time_peak - time_start
    span_right = time_end - time_peak

    if target_area > 0 and adc_peak > threshold and span_left > 0 and span_right > 0:
        for i in range(1, n_samples):
            frac_left = i / n_samples
            t1 = time_start + frac_left * span_left
            if not (time_start < t1 < time_peak):
                continue

            for j in range(1, n_samples):
                frac_right = j / n_samples
                t2 = time_peak + frac_right * span_right
                if not (time_peak < t2 < time_end):
                    continue

                y1 = threshold + frac_left * (adc_peak - threshold)
                y2 = threshold + (1.0 - frac_right) * (adc_peak - threshold)

                if y1 < threshold or y1 > adc_peak or y2 < threshold or y2 > adc_peak:
                    continue

                vertices = [
                    (time_start, threshold),
                    (t1, y1),
                    (time_peak, adc_peak),
                    (t2, y2),
                    (time_end, threshold)
                ]
                area = _polygon_area(vertices)
                diff = abs(area - target_area)
                if diff < best_diff:
                    best_diff = diff
                    best = (t1, t2, y1, y2)

    if best is not None:
        t1, t2, y1, y2 = best
        return (int(round(t1)), int(round(t2)), float(y1), float(y2), threshold, True)

    # Fallback: analytic approach with recursive frac adjustment (matches C++ logic)
    frac = max(min(float(frac), 0.9), 0.1)
    span_total = time_end - time_start
    if span_total <= 0:
        return (0, 0, 0, 0, 0, False)

    rise = max(time_peak - time_start, 0.0)
    fall = max(time_end - time_peak, 0.0)

    ad = rise * frac
    dh = rise - ad
    eb = fall * frac
    he = fall - eb

    time_int_rise = time_start + ad
    time_int_fall = time_peak + eb

    peak_width = dh * (1.0 - frac) + frac * he
    if target_area <= 0:
        intermediate_height = threshold
    else:
        intermediate_height = (2.0 * target_area - adc_peak * peak_width) / span_total

    if intermediate_height < threshold:
        if frac >= 0.9 or _depth > 8:
            intermediate_height = threshold
        else:
            new_frac = 1.0 - 0.5 * (1.0 - frac)
            return calculate_pentagon_params(time_start, time_peak, time_end, adc_peak,
                                             target_area, threshold, new_frac, _depth + 1)

    if intermediate_height > adc_peak:
        if frac <= 0.1 or _depth > 8:
            intermediate_height = adc_peak
        else:
            new_frac = 0.5 * frac
            return calculate_pentagon_params(time_start, time_peak, time_end, adc_peak,
                                             target_area, threshold, new_frac, _depth + 1)

    intermediate_height = max(threshold, min(intermediate_height, adc_peak))

    return (
        int(round(time_int_rise)),
        int(round(time_int_fall)),
        float(intermediate_height),
        float(intermediate_height),
        threshold,
        True
    )


def load_clusters_from_file(cluster_file, plane='X', verbose=False):
    """
    Load clusters from ROOT file for specified plane.
    
    Returns:
        list of dicts, each containing cluster information
    """
    if verbose:
        print(f"Loading clusters from: {cluster_file}")
    
    try:
        f = uproot.open(cluster_file)
    except Exception as e:
        print(f"Error opening file {cluster_file}: {e}")
        return []
    
    all_clusters = []
    
    # Read from both 'clusters' and 'discarded' directories
    # For volumes, we want ALL clusters regardless of energy cut
    for directory in ['clusters', 'discarded']:
        tree_name = f'{directory}/clusters_tree_{plane}'
        if tree_name not in f:
            if verbose:
                print(f"Note: {tree_name} not found in {cluster_file}")
            continue
        
        tree = f[tree_name]
        
        # Load relevant branches
        branches = [
            'event', 'n_tps', 'is_main_cluster',
            'is_es_interaction', 'true_neutrino_energy', 'true_particle_energy',
            'true_mom_x', 'true_mom_y', 'true_mom_z',
            'true_label', 'marley_tp_fraction',
            'tp_detector_channel', 'tp_time_start',
            'tp_adc_integral', 'tp_adc_peak', 
            'tp_samples_over_threshold', 'tp_samples_to_peak',
            'cluster_id'
        ]
        # total_charge is written by clustering (reconstructed charge for the cluster)
        if 'total_charge' in tree.keys():
            branches.append('total_charge')
        
        # Check if match_id exists (for matched_clusters files)
        if 'match_id' in tree.keys():
            branches.extend(['match_id', 'match_type'])
        
        arrays = tree.arrays(branches, library='np')
        n_entries = len(arrays['event'])
        
        for i in range(n_entries):
            # Get TP data
            channels = arrays['tp_detector_channel'][i]
            times_tdc = arrays['tp_time_start'][i]
            adc_integrals = arrays['tp_adc_integral'][i]
            adc_peaks = arrays['tp_adc_peak'][i]
            samples_over_threshold = arrays['tp_samples_over_threshold'][i]
            samples_to_peak = arrays['tp_samples_to_peak'][i]
            total_charge = float(arrays['total_charge'][i]) if 'total_charge' in arrays else 0.0
            
            if len(channels) == 0:
                continue
            
            # Convert times from TDC to TPC ticks
            times_tpc = times_tdc / TDC_TO_TPC_CONVERSION
            
            # Calculate time_end and time_peak from samples_over_threshold and samples_to_peak
            # time_end = time_start + samples_over_threshold
            # time_peak = time_start + samples_to_peak
            times_end_tpc = times_tpc + samples_over_threshold
            times_peak_tpc = times_tpc + samples_to_peak
            
            # Calculate cluster center in detector units
            center_channel = np.mean(channels)
            center_time_tpc = np.mean(times_tpc)
            
            # Check if it's a marley cluster
            # Use marley_tp_fraction as primary indicator since true_label may be 'UNKNOWN' in matched files
            marley_frac = arrays['marley_tp_fraction'][i]
            if hasattr(marley_frac, '__len__') and len(marley_frac) > 0:
                marley_frac = marley_frac[0]
            is_marley = float(marley_frac) > 0
            
            # Also check true_label as fallback
            true_label = arrays['true_label'][i]
            if isinstance(true_label, bytes):
                true_label = true_label.decode('utf-8')
            if 'marley' in true_label.lower():
                is_marley = True
            
            # Compute reconstructed energy from total_charge per-cluster (MeV)
            if isinstance(plane, str) and plane == 'X':
                reco_energy = total_charge / ADC_TO_MEV_COLLECTION
            else:
                reco_energy = total_charge / ADC_TO_MEV_INDUCTION

            cluster_info = {
                'event': int(arrays['event'][i]),
                'n_tps': int(arrays['n_tps'][i]),
                'is_main_cluster': bool(arrays['is_main_cluster'][i]),
                'channels': channels,
                'times_tpc': times_tpc,
                'times_end_tpc': times_end_tpc,
                'times_peak_tpc': times_peak_tpc,
                'adc_integrals': adc_integrals,
                'adc_peaks': adc_peaks,
                'samples_over_threshold': samples_over_threshold,
                'samples_to_peak': samples_to_peak,
                'center_channel': center_channel,
                'center_time_tpc': center_time_tpc,
                'is_es_interaction': bool(arrays['is_es_interaction'][i]),
                'true_neutrino_energy': float(arrays['true_neutrino_energy'][i]),
                'true_particle_energy': float(arrays['true_particle_energy'][i]),
                'true_mom_x': float(arrays['true_mom_x'][i]),
                'true_mom_y': float(arrays['true_mom_y'][i]),
                'true_mom_z': float(arrays['true_mom_z'][i]),
                'is_marley': is_marley,
                'marley_tp_fraction': float(arrays['marley_tp_fraction'][i]),
                'cluster_id': int(arrays['cluster_id'][i]),
                'match_id': int(arrays['match_id'][i]) if 'match_id' in arrays else -1,
                'match_type': int(arrays['match_type'][i]) if 'match_type' in arrays else -1,
                'total_charge': total_charge,
                'reco_energy_mev': float(reco_energy)
            }
            
            all_clusters.append(cluster_info)
    
    if verbose:
        print(f"  Loaded {len(all_clusters)} clusters from plane {plane} (including discarded)")
    
    if verbose and len(all_clusters) > 0:
        n_main = sum(1 for c in all_clusters if c['is_main_cluster'])
        print(f"  Found {n_main} main track clusters")
    
    return all_clusters


def get_clusters_in_volume(all_clusters, center_channel, center_time_tpc, volume_size_cm=100.0, event=None):
    """
    Get all clusters within a volume around the center point.
    
    Args:
        all_clusters: List of cluster dicts
        center_channel: Center channel number
        center_time_tpc: Center time in TPC ticks
        volume_size_cm: Volume size in cm (default 100cm = 1m)
        event: If provided, only include clusters from this event
    
    Returns:
        List of cluster dicts within the volume
    """
    # Convert volume size to detector units
    channel_range = volume_size_cm / WIRE_PITCH_COLLECTION_CM
    time_range_tpc = volume_size_cm / TIME_TICK_CM
    
    # Define bounds
    min_channel = center_channel - channel_range / 2.0
    max_channel = center_channel + channel_range / 2.0
    min_time = center_time_tpc - time_range_tpc / 2.0
    max_time = center_time_tpc + time_range_tpc / 2.0
    
    # Filter clusters
    volume_clusters = []
    for cluster in all_clusters:
        # Filter by event if specified
        if event is not None and cluster['event'] != event:
            continue
        
        if (min_channel <= cluster['center_channel'] <= max_channel and
            min_time <= cluster['center_time_tpc'] <= max_time):
            volume_clusters.append(cluster)
    
    return volume_clusters


def create_volume_image(volume_clusters, center_channel, center_time_tpc, volume_size_cm=100.0, plane='X'):
    """
    Create a 2D image array from clusters in the volume using pentagon interpolation.
    
    Returns:
        numpy array with shape (n_channels, n_time_bins)
    """
    # Get threshold for this plane
    threshold_adc = THRESHOLD_ADC_X if plane == 'X' else (THRESHOLD_ADC_U if plane == 'U' else THRESHOLD_ADC_V)
    
    # Calculate image dimensions in detector units
    n_channels = int(volume_size_cm / WIRE_PITCH_COLLECTION_CM)
    n_time_bins = int(volume_size_cm / TIME_TICK_CM)
    
    # Create empty image
    image = np.zeros((n_channels, n_time_bins), dtype=np.float32)
    
    # Calculate offset to center the volume
    channel_offset = center_channel - n_channels / 2.0
    time_offset = center_time_tpc - n_time_bins / 2.0
    
    # Fill image with TPs from all clusters in volume using pentagon interpolation
    for cluster in volume_clusters:
        channels = cluster['channels']
        times_start = cluster['times_tpc']
        times_end = cluster['times_end_tpc']
        times_peak = cluster['times_peak_tpc']
        adc_integrals = cluster['adc_integrals']
        adc_peaks = cluster['adc_peaks']
        
        # Draw each TP with pentagon interpolation
        for tp_idx in range(len(channels)):
            ch = channels[tp_idx]
            time_start = times_start[tp_idx]
            time_end = times_end[tp_idx]
            time_peak = times_peak[tp_idx]
            adc_peak = adc_peaks[tp_idx]
            adc_integral = adc_integrals[tp_idx] if tp_idx < len(adc_integrals) else 0.0
            
            # Convert channel to image coordinates
            ch_idx = int(ch - channel_offset)
            if not (0 <= ch_idx < n_channels):
                continue
            
            # Calculate pentagon parameters
            time_int_rise, time_int_fall, h_int_rise, h_int_fall, threshold, valid = \
                calculate_pentagon_params(time_start, time_peak, time_end, adc_peak, adc_integral, threshold_adc)
            
            if not valid:
                continue
            
            # Fill pixels for this TP - match C++ Display.cpp logic exactly
            # Pentagon: rises from 0 to h_int_rise, then to peak, then falls to h_int_fall, then to 0
            t_min = int(time_start - time_offset) - 1  # extended range like C++
            t_max = int(time_end - time_offset) + 2
            
            for t_idx in range(max(0, t_min), min(n_time_bins, t_max)):
                t_tpc = t_idx + time_offset
                intensity = 0.0
                
                if t_tpc < time_start:
                    # Extended base before time_start
                    span = time_int_rise - time_start
                    if span > 0:
                        frac = (t_tpc - time_start) / span
                        intensity = frac * h_int_rise
                        if intensity < threshold * 0.5:
                            intensity = 0.0
                elif t_tpc < time_int_rise:
                    # Segment 1: rising from 0 to h_int_rise
                    span = time_int_rise - time_start
                    if span > 0:
                        frac = (t_tpc - time_start) / span
                        intensity = frac * h_int_rise
                elif t_tpc < time_peak:
                    # Segment 2: rising from h_int_rise to peak
                    span = time_peak - time_int_rise
                    if span > 0:
                        frac = (t_tpc - time_int_rise) / span
                        intensity = h_int_rise + frac * (adc_peak - h_int_rise)
                    else:
                        intensity = adc_peak
                elif t_tpc == time_peak:
                    # Peak
                    intensity = adc_peak
                elif t_tpc <= time_int_fall:
                    # Segment 3: falling from peak to h_int_fall
                    span = time_int_fall - time_peak
                    if span > 0:
                        frac = (t_tpc - time_peak) / span
                        intensity = adc_peak - frac * (adc_peak - h_int_fall)
                    else:
                        intensity = h_int_fall
                elif t_tpc < time_end:
                    # Segment 4: falling from h_int_fall to 0
                    span = time_end - time_int_fall
                    if span > 0:
                        frac = (t_tpc - time_int_fall) / span
                        intensity = h_int_fall - frac * h_int_fall
                else:
                    # Extended base after time_end
                    span = time_end - time_int_fall
                    if span > 0:
                        frac = (t_tpc - time_int_fall) / span
                        intensity = h_int_fall - frac * h_int_fall
                        if intensity < threshold * 0.5:
                            intensity = 0.0
                
                # Use max() like C++ to keep highest value per pixel
                if intensity > 0:
                    image[ch_idx, t_idx] = max(image[ch_idx, t_idx], intensity)
    
    return image


def process_cluster_file(cluster_file, output_folder, plane='X', verbose=False):
    """
    Process a single cluster file and create volume images for all main tracks.
    All volumes from one input file are saved into a single output NPZ file.
    
    Returns:
        Number of volumes created
    """
    # Load all clusters
    clusters = load_clusters_from_file(cluster_file, plane=plane, verbose=verbose)
    
    if len(clusters) == 0:
        if verbose:
            print(f"  No clusters found in {cluster_file}")
        return 0
    
    # Get main track clusters
    main_clusters = [c for c in clusters if c['is_main_cluster']]
    
    if len(main_clusters) == 0:
        if verbose:
            print(f"  No main track clusters found")
        return 0
    
    # Create output folder if needed
    os.makedirs(output_folder, exist_ok=True)
    
    # Get base filename for output (e.g., 'clusters_0' from 'clusters_0_clusters.root')
    base_name = Path(cluster_file).stem.replace('_clusters', '')
    
    # Collect all volumes and metadata
    all_images = []
    all_metadata = []
    
    # Process each main track
    for idx, main_cluster in enumerate(main_clusters):
        # Get clusters in volume around this main track (only from same event)
        volume_clusters = get_clusters_in_volume(
            clusters,
            main_cluster['center_channel'],
            main_cluster['center_time_tpc'],
            VOLUME_SIZE_CM,
            event=main_cluster['event']  # Only include clusters from the same event
        )
        
        if len(volume_clusters) == 0:
            continue
        
        # Create volume image
        image = create_volume_image(
            volume_clusters,
            main_cluster['center_channel'],
            main_cluster['center_time_tpc'],
            VOLUME_SIZE_CM,
            plane
        )
        
        # Calculate metadata
        n_marley_clusters = sum(1 for c in volume_clusters if c['is_marley'])
        n_non_marley_clusters = len(volume_clusters) - n_marley_clusters
        
        # Calculate momentum magnitude
        mom_x = main_cluster['true_mom_x']
        mom_y = main_cluster['true_mom_y']
        mom_z = main_cluster['true_mom_z']
        mom_mag = np.sqrt(mom_x**2 + mom_y**2 + mom_z**2)
        
        # If momentum is 0 but we have particle energy, estimate momentum for electrons
        # For relativistic electrons: p ≈ sqrt(E^2 - m_e^2) ≈ E for E >> 0.511 MeV
        if mom_mag == 0 and main_cluster['is_marley']:
            particle_energy_mev = main_cluster['true_particle_energy']
            if particle_energy_mev > 1.0:  # Only if we have meaningful energy
                # Electron mass: 0.511 MeV/c^2
                me_mev = 0.511
                mom_mag = np.sqrt(max(0, particle_energy_mev**2 - me_mev**2))
        
        # Determine energy and interaction type
        # For marley clusters, use true_particle_energy (actual track energy)
        # For background clusters, neutrino_energy will be -1.0
        # In matched files, truth energy may be -1.0 even for MARLEY, so use marley_tp_fraction
        is_es = main_cluster['is_es_interaction']
        if main_cluster['is_marley']:
            # Use reconstructed cluster energy (from total_charge) instead of true energy
            event_energy = main_cluster.get('reco_energy_mev', -1.0)
            # If matched file lost truth info, energy will be -1.0 but we still know it's ES from marley_tp_fraction
            interaction_type = "ES" if is_es or main_cluster['marley_tp_fraction'] > 0 else "CC"
        else:
            # Background clusters
            event_energy = -1.0
            interaction_type = "Background"
        
        metadata = {
            'event': main_cluster['event'],
            'plane': plane,
            'interaction_type': interaction_type,
            'particle_energy': event_energy,  # Electron/particle energy for marley, -1 for background
            'main_track_momentum': mom_mag,
            'main_track_momentum_x': mom_x,
            'main_track_momentum_y': mom_y,
            'main_track_momentum_z': mom_z,
            'n_clusters_in_volume': len(volume_clusters),
            'n_marley_clusters': n_marley_clusters,
            'n_non_marley_clusters': n_non_marley_clusters,
            'center_channel': float(main_cluster['center_channel']),
            'center_time_tpc': float(main_cluster['center_time_tpc']),
            'volume_size_cm': VOLUME_SIZE_CM,
            'image_shape': image.shape,
            'main_cluster_id': main_cluster['cluster_id'],
            'volume_index': idx,  # Add volume index within this file
            'source_root_file': os.path.abspath(cluster_file)
        }
        
        # Add to collections
        all_images.append(image)
        all_metadata.append(metadata)
        
        if verbose:
            interaction_str = "ES" if main_cluster['is_es_interaction'] else "CC" if main_cluster['is_marley'] else "Background"
            print(f"  Volume {idx}: {len(volume_clusters)} clusters "
                  f"({n_marley_clusters} marley, {n_non_marley_clusters} non-marley), "
                  f"event {main_cluster['event']}, {interaction_str}")
    
    # Save all volumes from this file into a single NPZ
    if len(all_images) > 0:
        output_file = os.path.join(output_folder, f"{base_name}_plane{plane}.npz")
        # Save as arrays - images as a list of arrays (variable size), metadata as object array
        np.savez_compressed(output_file, 
                           images=np.array(all_images, dtype=object),  # Object array to handle variable shapes
                           metadata=np.array(all_metadata, dtype=object))
        
        if verbose:
            print(f"  Saved {len(all_images)} volumes to {output_file}")
    
    return len(all_images)


def main():
    parser = argparse.ArgumentParser(description='Create 1m x 1m volume images for channel tagging')
    parser.add_argument('-j', '--json', required=True, help='JSON configuration file')
    parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    parser.add_argument('-o', '--output-folder', type=str, default=None, help='Override output folder ')
    parser.add_argument('--skip', type=int, default=None, help='Override JSON skip_files: skip first N files')
    parser.add_argument('--max', type=int, default=None, help='Override JSON max_files: process at most N files')
    parser.add_argument('-f', '--override', action='store_true', help='Force reprocessing (kept for compatibility)')
    
    args = parser.parse_args()
    
    # Load JSON configuration
    with open(args.json, 'r') as f:
        config = json.load(f)
    
    # Use get_clusters_folder to compute the full path (matching make_clusters logic)
    clusters_folder = get_clusters_folder(config)
    
    # Try to use matched_clusters_folder if it exists
    matched_clusters_folder = get_matched_clusters_folder(config)
    if matched_clusters_folder and Path(matched_clusters_folder).exists():
        matched_files = list(Path(matched_clusters_folder).glob("*_matched.root"))
        if matched_files:
            clusters_folder = matched_clusters_folder
            print(f"Using matched clusters folder: {matched_clusters_folder}")
    
    # Auto-generate volume_images_folder if not explicitly provided
    if args.output_folder is not None:
        output_folder = args.output_folder
    elif 'volume_images_folder' in config and config['volume_images_folder']:
        output_folder = config['volume_images_folder']
    else:
        # Auto-generate from tpstream_folder
        tpstream_folder = config.get('tpstream_folder', '.')
        if tpstream_folder.endswith('/'):
            tpstream_folder = tpstream_folder[:-1]
        prefix = config.get('clusters_folder_prefix', 'volumes')
        conditions = get_conditions_string(config)
        output_folder = f"{tpstream_folder}/volume_images_{prefix}_{conditions}"
    
    plane = config.get('plane', 'X')  # Default to collection plane
    
    # CLI overrides JSON settings
    skip_files = args.skip if args.skip is not None else config.get('skip_files', 0)
    max_files = args.max if args.max is not None else config.get('max_files', None)
    
    print("="*60)
    print("Volume Image Creation for Channel Tagging")
    print("="*60)
    print(f"Clusters folder: {clusters_folder}")
    print(f"Output folder: {output_folder}")
    print(f"Plane: {plane}")
    print(f"Volume size: {VOLUME_SIZE_CM} cm x {VOLUME_SIZE_CM} cm")
    if skip_files > 0:
        print(f"Skipping first {skip_files} files")
    if max_files is not None:
        print(f"Processing at most {max_files} files")
    print("="*60)
    
    # Find all cluster files (look for *_clusters.root pattern)
    cluster_files = sorted(Path(clusters_folder).glob('*_matched.root'))
    if not cluster_files:
        cluster_files = sorted(Path(clusters_folder).glob('*_clusters.root'))
    
    # Apply skip and max
    if skip_files > 0:
        cluster_files = cluster_files[skip_files:]
    if max_files is not None:
        cluster_files = cluster_files[:max_files]
    
    if len(cluster_files) == 0:
        print(f"No cluster files found in {clusters_folder}")
        return 1
    
    print(f"Found {len(cluster_files)} cluster files")
    print()
    
    # Process each file
    total_volumes = 0
    for i, cluster_file in enumerate(cluster_files):
        print(f"[{i+1}/{len(cluster_files)}] Processing: {cluster_file.name}")
        
        n_volumes = process_cluster_file(
            str(cluster_file),
            output_folder,
            plane=plane,
            verbose=args.verbose
        )
        
        total_volumes += n_volumes
        print(f"  Created {n_volumes} volume images")
    
    print()
    print("="*60)
    print(f"DONE: Created {total_volumes} total volume images")
    print(f"Output saved to: {output_folder}")
    print("="*60)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
