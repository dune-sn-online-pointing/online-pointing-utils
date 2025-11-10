#!/usr/bin/env python3
"""
Test APA flipping logic by finding and visualizing clusters from different APA configurations.

Requirements:
- Find 4 clusters with similar particle momentum direction (normalized)
- At least 5 TPs each
- One from each category:
  1. Top APA (0 or 2), x < 0 (collection channels 0-399)
  2. Top APA (0 or 2), x > 0 (collection channels 400-799)
  3. Bottom APA (1 or 3), x < 0 (collection channels 400-799)
  4. Bottom APA (1 or 3), x > 0 (collection channels 0-399)
"""

import sys
import glob
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt
import uproot

sys.path.insert(0, str(Path(__file__).parent / 'app'))
from generate_cluster_arrays import extract_clusters_from_file


def normalize_vector(v):
    """Normalize a 3D vector."""
    mag = np.linalg.norm(v)
    return v / mag if mag > 0 else v


def find_test_clusters(matched_files, target_momentum_dir=None, min_tps=5, min_energy_mev=10.0, skip_clusters=None):
    """
    Find 4 clusters matching the test criteria.
    
    Args:
        matched_files: List of ROOT files to search
        target_momentum_dir: Optional target momentum direction for similarity filtering
        min_tps: Minimum number of TPs per cluster
        min_energy_mev: Minimum energy in MeV (for decent tracks)
        skip_clusters: Dict of {category: [(file_path, cluster_idx), ...]} to skip
    
    Returns:
        dict: {category: (file_path, cluster_idx, momentum, apa_id, x_sign, n_tps, energy)}
    """
    if skip_clusters is None:
        skip_clusters = {}
    
    categories = {
        'top_neg': None,   # Top APA, x < 0
        'top_pos': None,   # Top APA, x > 0
        'bottom_neg': None,  # Bottom APA, x < 0
        'bottom_pos': None   # Bottom APA, x > 0
    }
    
    for file_path in matched_files:
        try:
            f = uproot.open(file_path)
            tree = f['clusters/clusters_tree_X']
            
            # Read necessary branches
            data = tree.arrays([
                'tp_detector', 'tp_detector_channel', 'n_tps',
                'true_mom_x', 'true_mom_y', 'true_mom_z', 'true_particle_energy'
            ], library='np')
            
            for i in range(len(data['tp_detector'])):
                n_tps = data['n_tps'][i]
                if n_tps < min_tps:
                    continue
                
                # Check energy threshold
                energy = data['true_particle_energy'][i]
                if energy < min_energy_mev:
                    continue
                
                # Get APA and channel info
                detectors = data['tp_detector'][i]
                channels = data['tp_detector_channel'][i]
                
                if len(detectors) == 0:
                    continue
                
                apa_id = int(detectors[0])
                is_top_apa = (apa_id % 2 == 0)
                
                # Get collection channel (1760-2559 → 0-799)
                first_ch = channels[0]
                if first_ch < 1760:  # Not collection plane
                    continue
                    
                coll_ch = first_ch - 1760
                
                # Determine x_sign based on APA type and channel
                if is_top_apa:
                    x_sign = -1 if coll_ch < 400 else 1
                else:
                    x_sign = 1 if coll_ch < 400 else -1
                
                # Get momentum direction
                mom = np.array([
                    data['true_mom_x'][i],
                    data['true_mom_y'][i],
                    data['true_mom_z'][i]
                ])
                mom_norm = normalize_vector(mom)
                
                # Check momentum direction filter (all components in [0.35, 0.65])
                if not (0.35 <= abs(mom_norm[0]) <= 0.65 and 
                        0.35 <= abs(mom_norm[1]) <= 0.65 and 
                        0.35 <= abs(mom_norm[2]) <= 0.65):
                    continue
                
                # Determine category
                if is_top_apa:
                    category = 'top_neg' if x_sign < 0 else 'top_pos'
                else:
                    category = 'bottom_neg' if x_sign < 0 else 'bottom_pos'
                
                # Check if this cluster should be skipped
                if category in skip_clusters:
                    if (file_path, i) in skip_clusters[category]:
                        continue
                
                # Check if we already have this category
                if categories[category] is not None:
                    continue
                
                # Save this cluster
                categories[category] = (file_path, i, mom_norm, apa_id, x_sign, int(n_tps), float(energy))
                
                # If we found all 4, we can return early
                if all(v is not None for v in categories.values()):
                    return categories
                    
        except Exception as e:
            print(f"Error processing {file_path}: {e}")
            continue
    
    return categories


def visualize_test_clusters(categories, output_dir='test_apa_flipping_output'):
    """
    Visualize the 4 test clusters (X plane only) to verify flipping is correct.
    """
    output_dir = Path(output_dir)
    output_dir.mkdir(exist_ok=True, parents=True)
    
    fig, axes = plt.subplots(4, 1, figsize=(8, 16))
    fig.suptitle('APA Flipping Test: X Plane Clusters (E > 10 MeV, |p_i| ∈ [0.35, 0.65])', fontsize=16)
    
    category_names = {
        'top_neg': 'Top APA, x<0 (ch 0-399)',
        'top_pos': 'Top APA, x>0 (ch 400-799)',
        'bottom_neg': 'Bottom APA, x<0 (ch 400-799)',
        'bottom_pos': 'Bottom APA, x>0 (ch 0-399)'
    }
    
    for row, (cat_key, cat_name) in enumerate(category_names.items()):
        cluster_info = categories[cat_key]
        ax = axes[row]
        
        if cluster_info is None:
            ax.text(0.5, 0.5, f'No cluster found for {cat_name}',
                   ha='center', va='center', transform=ax.transAxes)
            ax.axis('off')
            continue
        
        file_path, cluster_idx, momentum, apa_id, x_sign, n_tps, energy = cluster_info
        
        # Extract cluster images
        result = extract_clusters_from_file(file_path, verbose=False)
        
        # Plot X plane only
        if 'X' in result and cluster_idx < len(result['X']['images']):
            img = result['X']['images'][cluster_idx]
            
            im = ax.imshow(img, aspect='auto', cmap='viridis', origin='lower')
            ax.set_title(f'{cat_name} | APA {apa_id} | {n_tps} TPs | E={energy:.1f} MeV')
            ax.set_xlabel('Channel')
            ax.set_ylabel('Time (ticks)')
            plt.colorbar(im, ax=ax, label='ADC')
            
            # Add momentum info
            mom_str = f'p_norm=({momentum[0]:.2f}, {momentum[1]:.2f}, {momentum[2]:.2f})'
            ax.text(0.02, 0.98, mom_str, transform=ax.transAxes,
                   verticalalignment='top', bbox=dict(boxstyle='round', facecolor='white', alpha=0.8),
                   fontsize=9)
        else:
            ax.text(0.5, 0.5, f'X plane not available',
                   ha='center', va='center', transform=ax.transAxes)
            ax.axis('off')
    
    plt.tight_layout()
    output_file = output_dir / 'apa_flipping_test.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"\n✓ Saved visualization to: {output_file}")
    plt.close()
    
    # Print summary
    print("\n" + "="*80)
    print("CLUSTER SUMMARY (X plane, E > 10 MeV, |p_i| ∈ [0.35, 0.65] for i=x,y,z)")
    print("="*80)
    for cat_key, cat_name in category_names.items():
        cluster_info = categories[cat_key]
        if cluster_info:
            file_path, cluster_idx, momentum, apa_id, x_sign, n_tps, energy = cluster_info
            print(f"\n{cat_name}:")
            print(f"  File: {Path(file_path).name}")
            print(f"  Cluster index: {cluster_idx}")
            print(f"  APA: {apa_id} ({'Top' if apa_id % 2 == 0 else 'Bottom'})")
            print(f"  x_sign: {x_sign:+d}")
            print(f"  TPs: {n_tps}")
            print(f"  Energy: {energy:.2f} MeV")
            print(f"  Momentum (norm): ({momentum[0]:.3f}, {momentum[1]:.3f}, {momentum[2]:.3f})")
        else:
            print(f"\n{cat_name}: NOT FOUND")


def main():
    # Find matched cluster files
    search_patterns = [
        'data2/prod_es/matched_clusters_*/prod*.root',
        'data2/prod_cc/matched_clusters_*/prod*.root',
        'matched_clusters__tick3_ch2_min2_tot*/prod*.root'
    ]
    
    matched_files = []
    for pattern in search_patterns:
        matched_files.extend(glob.glob(pattern))
    
    if not matched_files:
        print("ERROR: No matched cluster files found!")
        print("Searched patterns:", search_patterns)
        return 1
    
    print(f"Found {len(matched_files)} matched cluster files")
    print("Searching for X plane clusters with E > 10 MeV and |p_i| ∈ [0.35, 0.65]...")
    
    # Skip the cluster we don't like
    skip_clusters = {
        'top_pos': [('data2/prod_es/matched_clusters_es_valid_bg_tick3_ch2_min2_tot3_e2p0/prodmarley_nue_flat_es_dune10kt_1x2x2_20250927T182942Z_gen_004425_supernova_g4_detsim_ana_2025-10-21T_092832Z_bg_matched.root', 170)]
    }
    
    # Find clusters
    categories = find_test_clusters(matched_files, min_tps=5, min_energy_mev=10.0, skip_clusters=skip_clusters)  # Search all files
    
    # Count found clusters
    found_count = sum(1 for v in categories.values() if v is not None)
    print(f"\nFound {found_count}/4 test clusters")
    
    if found_count == 0:
        print("ERROR: No suitable clusters found!")
        return 1
    
    # Visualize
    visualize_test_clusters(categories)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
