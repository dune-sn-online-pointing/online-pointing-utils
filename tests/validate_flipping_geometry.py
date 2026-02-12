#!/usr/bin/env python3
"""
Validate APA flipping by transforming U/V plane images and comparing to X plane.

This script:
1. Finds matched clusters (same match_id) across U, V, X planes
2. Applies geometric transformation to U/V images based on wire angles
3. Compares transformed U/V with X plane to validate flipping correctness

Wire geometry:
- Collection (X): vertical wires, pitch = 0.479 cm
- Induction (U/V): angled wires at ±35.7° from vertical, diagonal pitch = 0.4669 cm
- APA angle = 54.3° (90° - 35.7°)
"""

import sys
import glob
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt
from scipy import ndimage
import uproot

sys.path.insert(0, str(Path(__file__).parent.parent / 'python' / 'app'))
from generate_cluster_arrays import extract_clusters_from_file


def apply_shear_transform(img, angle_deg, pitch_ratio=1.0):
    """
    Apply shear transformation to align induction plane with collection plane.
    
    The induction wires are at ±35.7° from vertical. When a particle creates a 
    signal, it appears at different angles in U/V vs X. We need to shear the 
    channel axis (x) as a function of time axis (y) to align them.
    
    Args:
        img: Input image (time x channel)
        angle_deg: APA angle parameter (54.3° for U, -54.3° for V)
                   This will be converted to wire angle from vertical (±35.7°)
        pitch_ratio: Ratio of induction pitch to collection pitch
        
    Returns:
        Transformed image
    """
    # Convert angle: we receive ±54.3°, convert to wire angle from vertical (±35.7°)
    # For U plane: 54.3° → 35.7° from vertical
    # For V plane: -54.3° → -35.7° from vertical
    actual_angle = 90.0 - abs(angle_deg)  # Always get 35.7°
    if angle_deg < 0:
        actual_angle = -actual_angle  # Apply sign for V plane
    
    angle_rad = np.radians(actual_angle)
    
    # For induction wires at angle θ from vertical:
    # Shear factor to un-tilt: tan(θ)
    shear_factor = np.tan(angle_rad)
    
    # Forward transform (what we want to achieve):
    # new_channel = old_channel - shear_factor * time
    # Transform matrix: [[1, -shear], [0, 1]]
    
    # But affine_transform uses INVERSE transform (maps output coords to input coords)
    # So we need: input_coord = inverse_matrix * output_coord
    # Inverse of [[1, -s], [0, 1]] is [[1, s], [0, 1]]
    
    inverse_transform = np.array([
        [1, shear_factor],
        [0, 1]
    ])
    
    # Calculate output size
    h, w = img.shape
    max_shift = abs(shear_factor * h)
    output_shape = (h, int(w + max_shift + 10))  # Add some padding
    
    # Use simple offset - let the transformation naturally position the content
    offset = [0, 0]
    
    transformed = ndimage.affine_transform(
        img,
        inverse_transform,
        output_shape=output_shape,
        offset=offset,
        order=1,
        mode='constant',
        cval=0.0
    )
    
    # Adjust for wire pitch difference
    if pitch_ratio != 1.0:
        zoom_factors = (1.0, pitch_ratio)
        transformed = ndimage.zoom(transformed, zoom_factors, order=1)
    
    return transformed


def find_matched_clusters(matched_files, min_energy_mev=10.0, min_tps=5):
    """
    Find clusters that have matches across all three planes (U, V, X).
    
    Returns:
        List of (file_path, match_id, apa_id, x_sign, energy)
    """
    matched_clusters = []
    
    for file_path in matched_files:
        try:
            f = uproot.open(file_path)
            
            # Check all three planes
            has_all_planes = all(f'clusters/clusters_tree_{p}' in f for p in ['U', 'V', 'X'])
            if not has_all_planes:
                continue
            
            # Read X plane to get match_ids and geometry
            tree_x = f['clusters/clusters_tree_X']
            data_x = tree_x.arrays([
                'match_id', 'tp_detector', 'tp_detector_channel', 
                'n_tps', 'true_particle_energy'
            ], library='np')
            
            for i in range(len(data_x['match_id'])):
                match_id = int(data_x['match_id'][i])
                if match_id == -1:  # Unmatched
                    continue
                
                n_tps = data_x['n_tps'][i]
                energy = data_x['true_particle_energy'][i]
                
                if n_tps < min_tps or energy < min_energy_mev:
                    continue
                
                # Get APA and x_sign
                detectors = data_x['tp_detector'][i]
                channels = data_x['tp_detector_channel'][i]
                
                if len(channels) == 0:
                    continue
                
                apa_id = int(detectors[0])
                first_ch = channels[0]
                is_top_apa = (apa_id % 2 == 0)
                x_sign = -1 if first_ch < 2080 else 1
                if not is_top_apa:
                    x_sign = -x_sign
                
                # Check if this match_id exists in U and V
                tree_u = f['clusters/clusters_tree_U']
                tree_v = f['clusters/clusters_tree_V']
                
                match_ids_u = tree_u['match_id'].array(library='np')
                match_ids_v = tree_v['match_id'].array(library='np')
                
                if match_id in match_ids_u and match_id in match_ids_v:
                    matched_clusters.append({
                        'file': file_path,
                        'match_id': match_id,
                        'apa_id': apa_id,
                        'x_sign': x_sign,
                        'energy': energy,
                        'n_tps': n_tps
                    })
                    
                    if len(matched_clusters) >= 5:  # Find 5 good examples
                        return matched_clusters
                        
        except Exception as e:
            print(f"Error processing {file_path}: {e}")
            continue
    
    return matched_clusters


def visualize_matched_cluster(file_path, match_id, output_dir='test_apa_flipping_output'):
    """
    Load and visualize a matched cluster across all three planes.
    """
    output_dir = Path(output_dir)
    output_dir.mkdir(exist_ok=True, parents=True)
    
    # Extract all plane data
    result = extract_clusters_from_file(file_path, verbose=False)
    
    # Find the cluster indices for this match_id in each plane
    cluster_data = {}
    
    for plane in ['U', 'V', 'X']:
        if plane not in result:
            continue
        
        metadata = result[plane]['metadata']
        images = result[plane]['images']
        
        # match_id is at index 13 in metadata array
        for idx, meta in enumerate(metadata):
            if int(meta[13]) == match_id:
                cluster_data[plane] = {
                    'image': images[idx],
                    'metadata': meta,
                    'index': idx
                }
                break
    
    if len(cluster_data) < 3:
        print(f"Could not find all three planes for match_id {match_id}")
        return None
    
    # Geometry parameters
    angle_deg = 54.3  # APA angle
    pitch_ratio = 0.4669 / 0.479  # induction / collection
    
    # Transform U and V planes
    img_u_transformed = apply_shear_transform(
        cluster_data['U']['image'], 
        angle_deg, 
        pitch_ratio
    )
    
    img_v_transformed = apply_shear_transform(
        cluster_data['V']['image'], 
        -angle_deg,  # V plane is at opposite angle
        pitch_ratio
    )
    
    # Create visualization
    fig, axes = plt.subplots(2, 3, figsize=(18, 12))
    fig.suptitle(f'Match ID {match_id} - Validation of APA Flipping\n' +
                 f'File: {Path(file_path).name}', fontsize=12)
    
    # Row 1: Original images
    axes[0, 0].imshow(cluster_data['U']['image'], aspect='auto', cmap='viridis', origin='lower')
    axes[0, 0].set_title(f'U Plane (Original)\n{cluster_data["U"]["image"].shape}')
    axes[0, 0].set_ylabel('Time (ticks)')
    axes[0, 0].set_xlabel('Channel')
    
    axes[0, 1].imshow(cluster_data['V']['image'], aspect='auto', cmap='viridis', origin='lower')
    axes[0, 1].set_title(f'V Plane (Original)\n{cluster_data["V"]["image"].shape}')
    axes[0, 1].set_ylabel('Time (ticks)')
    axes[0, 1].set_xlabel('Channel')
    
    axes[0, 2].imshow(cluster_data['X']['image'], aspect='auto', cmap='viridis', origin='lower')
    axes[0, 2].set_title(f'X Plane (Collection)\n{cluster_data["X"]["image"].shape}')
    axes[0, 2].set_ylabel('Time (ticks)')
    axes[0, 2].set_xlabel('Channel')
    
    # Row 2: Transformed images (should look similar to X)
    axes[1, 0].imshow(img_u_transformed, aspect='auto', cmap='viridis', origin='lower')
    axes[1, 0].set_title(f'U Plane (Sheared to X view)\n{img_u_transformed.shape}')
    axes[1, 0].set_ylabel('Time (ticks)')
    axes[1, 0].set_xlabel('Channel (scaled)')
    
    axes[1, 1].imshow(img_v_transformed, aspect='auto', cmap='viridis', origin='lower')
    axes[1, 1].set_title(f'V Plane (Sheared to X view)\n{img_v_transformed.shape}')
    axes[1, 1].set_ylabel('Time (ticks)')
    axes[1, 1].set_xlabel('Channel (scaled)')
    
    axes[1, 2].imshow(cluster_data['X']['image'], aspect='auto', cmap='viridis', origin='lower')
    axes[1, 2].set_title(f'X Plane (Reference)\n{cluster_data["X"]["image"].shape}')
    axes[1, 2].set_ylabel('Time (ticks)')
    axes[1, 2].set_xlabel('Channel')
    
    plt.tight_layout()
    output_file = output_dir / f'geometry_validation_match_{match_id}.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"✓ Saved: {output_file}")
    plt.close()
    
    return cluster_data


def main():
    # Find matched cluster files
    search_patterns = [
        'data2/prod_es/matched_clusters_*/prod*.root',
        'data2/prod_cc/matched_clusters_*/prod*.root',
    ]
    
    matched_files = []
    for pattern in search_patterns:
        matched_files.extend(glob.glob(pattern))
    
    if not matched_files:
        print("ERROR: No matched cluster files found!")
        return 1
    
    print(f"Found {len(matched_files)} matched cluster files")
    print("Searching for fully matched clusters (U, V, X)...")
    
    # Find clusters
    clusters = find_matched_clusters(matched_files[:30], min_energy_mev=10.0, min_tps=5)
    
    print(f"\nFound {len(clusters)} fully matched clusters")
    
    if len(clusters) == 0:
        print("No suitable clusters found!")
        return 1
    
    # Visualize the first few
    for i, cluster in enumerate(clusters[:3]):
        print(f"\n[{i+1}/{min(3, len(clusters))}] Processing match_id {cluster['match_id']}:")
        print(f"  File: {Path(cluster['file']).name}")
        print(f"  APA: {cluster['apa_id']}, x_sign: {cluster['x_sign']:+d}")
        print(f"  Energy: {cluster['energy']:.2f} MeV, TPs: {cluster['n_tps']}")
        
        visualize_matched_cluster(cluster['file'], cluster['match_id'])
    
    print("\n" + "="*80)
    print("Validation complete!")
    print("Check the output images to verify:")
    print("1. U/V transformed images should have similar structure to X plane")
    print("2. Track angles should align after shear transformation")
    print("3. This validates that the flipping logic preserves geometry correctly")
    print("="*80)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
