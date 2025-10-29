#!/usr/bin/env python3
"""
analyze_volumes.py - Analyze volume image metadata and provide statistics

Reads volume NPZ files and analyzes metadata to provide insights about:
- Distribution of MARLEY vs non-MARLEY clusters
- Interaction types (ES, CC)
- Energy distributions
- Momentum distributions
- Volume cluster density
"""

import numpy as np
import argparse
import json
import sys
from pathlib import Path
from collections import defaultdict
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend
import matplotlib.pyplot as plt


def load_volume_metadata(volume_folder, verbose=False):
    """
    Load metadata from all volume NPZ files in the folder.
    
    Returns:
        list of metadata dictionaries
    """
    volume_files = sorted(Path(volume_folder).glob('*.npz'))
    
    if len(volume_files) == 0:
        print(f"No volume files found in {volume_folder}")
        return []
    
    if verbose:
        print(f"Found {len(volume_files)} volume files")
    
    all_metadata = []
    for vol_file in volume_files:
        try:
            npz = np.load(vol_file, allow_pickle=True)
            metadata = npz['metadata'].item() if npz['metadata'].shape == () else npz['metadata'][0]
            all_metadata.append(metadata)
        except Exception as e:
            print(f"Warning: Could not load {vol_file}: {e}")
            continue
    
    return all_metadata


def analyze_cluster_composition(metadata_list):
    """
    Analyze MARLEY vs non-MARLEY cluster composition.
    
    Returns:
        dict with statistics
    """
    stats = {
        'total_volumes': len(metadata_list),
        'pure_marley': 0,
        'pure_non_marley': 0,
        'mixed': 0,
        'total_marley_clusters': 0,
        'total_non_marley_clusters': 0,
        'marley_fractions': [],
        'clusters_per_volume': [],
    }
    
    for meta in metadata_list:
        n_marley = meta.get('n_marley_clusters', 0)
        n_non_marley = meta.get('n_non_marley_clusters', 0)
        n_total = meta.get('n_clusters_in_volume', n_marley + n_non_marley)
        
        stats['total_marley_clusters'] += n_marley
        stats['total_non_marley_clusters'] += n_non_marley
        stats['clusters_per_volume'].append(n_total)
        
        if n_total > 0:
            marley_frac = n_marley / n_total
            stats['marley_fractions'].append(marley_frac)
            
            if n_marley == n_total:
                stats['pure_marley'] += 1
            elif n_non_marley == n_total:
                stats['pure_non_marley'] += 1
            else:
                stats['mixed'] += 1
        else:
            stats['marley_fractions'].append(0)
    
    return stats


def analyze_interaction_types(metadata_list):
    """
    Analyze distribution of interaction types.
    
    Returns:
        dict with interaction type counts
    """
    interaction_counts = defaultdict(int)
    
    for meta in metadata_list:
        interaction_type = meta.get('interaction_type', 'Unknown')
        interaction_counts[interaction_type] += 1
    
    return dict(interaction_counts)


def analyze_energies(metadata_list):
    """
    Analyze particle energy distributions.
    
    Returns:
        dict with energy statistics
    """
    energies = []
    
    for meta in metadata_list:
        energy = meta.get('particle_energy', None)
        if energy is not None and energy > 0:
            energies.append(energy)
    
    if len(energies) == 0:
        return None
    
    energies = np.array(energies)
    
    return {
        'mean': np.mean(energies),
        'std': np.std(energies),
        'min': np.min(energies),
        'max': np.max(energies),
        'median': np.median(energies),
        'values': energies,
    }


def analyze_momentum(metadata_list):
    """
    Analyze momentum distributions.
    
    Returns:
        dict with momentum statistics
    """
    momenta = []
    momenta_x = []
    momenta_y = []
    momenta_z = []
    
    for meta in metadata_list:
        mom = meta.get('main_track_momentum', None)
        mom_x = meta.get('main_track_momentum_x', None)
        mom_y = meta.get('main_track_momentum_y', None)
        mom_z = meta.get('main_track_momentum_z', None)
        
        if mom is not None:
            momenta.append(mom)
        if mom_x is not None:
            momenta_x.append(mom_x)
        if mom_y is not None:
            momenta_y.append(mom_y)
        if mom_z is not None:
            momenta_z.append(mom_z)
    
    if len(momenta) == 0:
        return None
    
    momenta = np.array(momenta)
    
    stats = {
        'mean': np.mean(momenta),
        'std': np.std(momenta),
        'min': np.min(momenta),
        'max': np.max(momenta),
        'median': np.median(momenta),
        'values': momenta,
    }
    
    if len(momenta_x) > 0:
        stats['mean_x'] = np.mean(momenta_x)
        stats['mean_y'] = np.mean(momenta_y)
        stats['mean_z'] = np.mean(momenta_z)
    
    return stats


def print_summary(metadata_list):
    """
    Print comprehensive summary statistics.
    """
    print("="*80)
    print("VOLUME IMAGE ANALYSIS SUMMARY")
    print("="*80)
    print()
    
    # Basic stats
    print(f"Total volume images analyzed: {len(metadata_list)}")
    print()
    
    # Cluster composition
    print("-" * 80)
    print("CLUSTER COMPOSITION (MARLEY vs Non-MARLEY)")
    print("-" * 80)
    comp_stats = analyze_cluster_composition(metadata_list)
    
    print(f"Pure MARLEY volumes:     {comp_stats['pure_marley']:4d} ({comp_stats['pure_marley']/comp_stats['total_volumes']*100:5.1f}%)")
    print(f"Pure non-MARLEY volumes: {comp_stats['pure_non_marley']:4d} ({comp_stats['pure_non_marley']/comp_stats['total_volumes']*100:5.1f}%)")
    print(f"Mixed volumes:           {comp_stats['mixed']:4d} ({comp_stats['mixed']/comp_stats['total_volumes']*100:5.1f}%)")
    print()
    print(f"Total MARLEY clusters:     {comp_stats['total_marley_clusters']}")
    print(f"Total non-MARLEY clusters: {comp_stats['total_non_marley_clusters']}")
    print(f"Overall MARLEY fraction:   {comp_stats['total_marley_clusters']/(comp_stats['total_marley_clusters']+comp_stats['total_non_marley_clusters'])*100:.1f}%")
    print()
    
    if len(comp_stats['marley_fractions']) > 0:
        marley_fracs = np.array(comp_stats['marley_fractions'])
        print(f"MARLEY fraction per volume:")
        print(f"  Mean:   {np.mean(marley_fracs):.3f}")
        print(f"  Median: {np.median(marley_fracs):.3f}")
        print(f"  Std:    {np.std(marley_fracs):.3f}")
    print()
    
    if len(comp_stats['clusters_per_volume']) > 0:
        clusters_per = np.array(comp_stats['clusters_per_volume'])
        print(f"Clusters per volume:")
        print(f"  Mean:   {np.mean(clusters_per):.1f}")
        print(f"  Median: {np.median(clusters_per):.1f}")
        print(f"  Min:    {np.min(clusters_per)}")
        print(f"  Max:    {np.max(clusters_per)}")
    print()
    
    # Interaction types
    print("-" * 80)
    print("INTERACTION TYPES")
    print("-" * 80)
    interaction_stats = analyze_interaction_types(metadata_list)
    for int_type, count in sorted(interaction_stats.items(), key=lambda x: -x[1]):
        print(f"{int_type:15s}: {count:4d} ({count/len(metadata_list)*100:5.1f}%)")
    print()
    
    # Energy distribution
    print("-" * 80)
    print("PARTICLE ENERGY DISTRIBUTION")
    print("-" * 80)
    energy_stats = analyze_energies(metadata_list)
    if energy_stats:
        print(f"Mean:   {energy_stats['mean']:7.2f} GeV")
        print(f"Median: {energy_stats['median']:7.2f} GeV")
        print(f"Std:    {energy_stats['std']:7.2f} GeV")
        print(f"Min:    {energy_stats['min']:7.2f} GeV")
        print(f"Max:    {energy_stats['max']:7.2f} GeV")
    else:
        print("No energy information available")
    print()
    
    # Momentum distribution
    print("-" * 80)
    print("MAIN TRACK MOMENTUM DISTRIBUTION")
    print("-" * 80)
    mom_stats = analyze_momentum(metadata_list)
    if mom_stats:
        print(f"Mean:   {mom_stats['mean']:7.4f} GeV/c")
        print(f"Median: {mom_stats['median']:7.4f} GeV/c")
        print(f"Std:    {mom_stats['std']:7.4f} GeV/c")
        print(f"Min:    {mom_stats['min']:7.4f} GeV/c")
        print(f"Max:    {mom_stats['max']:7.4f} GeV/c")
        if 'mean_x' in mom_stats:
            print(f"\nMean momentum components:")
            print(f"  px: {mom_stats['mean_x']:7.4f} GeV/c")
            print(f"  py: {mom_stats['mean_y']:7.4f} GeV/c")
            print(f"  pz: {mom_stats['mean_z']:7.4f} GeV/c")
    else:
        print("No momentum information available")
    print()
    
    print("="*80)


def create_plots(metadata_list, output_folder):
    """
    Create visualization plots and save them.
    """
    output_path = Path(output_folder)
    output_path.mkdir(parents=True, exist_ok=True)
    
    # Get statistics
    comp_stats = analyze_cluster_composition(metadata_list)
    energy_stats = analyze_energies(metadata_list)
    mom_stats = analyze_momentum(metadata_list)
    interaction_stats = analyze_interaction_types(metadata_list)
    
    # Create figure with subplots
    fig = plt.figure(figsize=(16, 12))
    
    # 1. MARLEY fraction distribution
    ax1 = plt.subplot(3, 3, 1)
    if len(comp_stats['marley_fractions']) > 0:
        ax1.hist(comp_stats['marley_fractions'], bins=20, edgecolor='black', alpha=0.7)
        ax1.set_xlabel('MARLEY Fraction per Volume')
        ax1.set_ylabel('Count')
        ax1.set_title('Distribution of MARLEY Fraction')
        ax1.grid(True, alpha=0.3)
    
    # 2. Clusters per volume
    ax2 = plt.subplot(3, 3, 2)
    if len(comp_stats['clusters_per_volume']) > 0:
        ax2.hist(comp_stats['clusters_per_volume'], bins=20, edgecolor='black', alpha=0.7)
        ax2.set_xlabel('Number of Clusters')
        ax2.set_ylabel('Count')
        ax2.set_title('Clusters per Volume')
        ax2.grid(True, alpha=0.3)
    
    # 3. Volume composition pie chart
    ax3 = plt.subplot(3, 3, 3)
    labels = ['Pure MARLEY', 'Pure Non-MARLEY', 'Mixed']
    sizes = [comp_stats['pure_marley'], comp_stats['pure_non_marley'], comp_stats['mixed']]
    colors = ['#ff9999', '#66b3ff', '#99ff99']
    ax3.pie(sizes, labels=labels, colors=colors, autopct='%1.1f%%', startangle=90)
    ax3.set_title('Volume Composition')
    
    # 4. Energy distribution
    ax4 = plt.subplot(3, 3, 4)
    if energy_stats:
        ax4.hist(energy_stats['values'], bins=30, edgecolor='black', alpha=0.7)
        ax4.set_xlabel('Particle Energy (GeV)')
        ax4.set_ylabel('Count')
        ax4.set_title('Particle Energy Distribution')
        ax4.grid(True, alpha=0.3)
    
    # 5. Momentum distribution
    ax5 = plt.subplot(3, 3, 5)
    if mom_stats:
        ax5.hist(mom_stats['values'], bins=30, edgecolor='black', alpha=0.7)
        ax5.set_xlabel('Main Track Momentum (GeV/c)')
        ax5.set_ylabel('Count')
        ax5.set_title('Main Track Momentum Distribution')
        ax5.grid(True, alpha=0.3)
    
    # 6. Interaction types bar chart
    ax6 = plt.subplot(3, 3, 6)
    int_types = list(interaction_stats.keys())
    int_counts = list(interaction_stats.values())
    ax6.bar(int_types, int_counts, edgecolor='black', alpha=0.7)
    ax6.set_xlabel('Interaction Type')
    ax6.set_ylabel('Count')
    ax6.set_title('Interaction Types')
    ax6.grid(True, alpha=0.3, axis='y')
    
    # 7. MARLEY vs non-MARLEY clusters total
    ax7 = plt.subplot(3, 3, 7)
    cluster_types = ['MARLEY', 'Non-MARLEY']
    cluster_counts = [comp_stats['total_marley_clusters'], comp_stats['total_non_marley_clusters']]
    colors = ['#ff9999', '#66b3ff']
    ax7.bar(cluster_types, cluster_counts, color=colors, edgecolor='black', alpha=0.7)
    ax7.set_ylabel('Total Clusters')
    ax7.set_title('Total MARLEY vs Non-MARLEY Clusters')
    ax7.grid(True, alpha=0.3, axis='y')
    
    # 8. Energy vs MARLEY fraction scatter
    ax8 = plt.subplot(3, 3, 8)
    if energy_stats and len(comp_stats['marley_fractions']) > 0:
        marley_fracs = np.array(comp_stats['marley_fractions'])
        valid_mask = np.array([meta.get('particle_energy', 0) > 0 for meta in metadata_list])
        if np.sum(valid_mask) > 0:
            energies = np.array([meta.get('particle_energy', 0) for meta in metadata_list])
            ax8.scatter(marley_fracs[valid_mask], energies[valid_mask], alpha=0.6)
            ax8.set_xlabel('MARLEY Fraction')
            ax8.set_ylabel('Particle Energy (GeV)')
            ax8.set_title('Energy vs MARLEY Fraction')
            ax8.grid(True, alpha=0.3)
    
    # 9. Momentum vs number of clusters scatter
    ax9 = plt.subplot(3, 3, 9)
    if mom_stats:
        valid_mask = np.array([meta.get('main_track_momentum', 0) > 0 for meta in metadata_list])
        if np.sum(valid_mask) > 0:
            momenta = np.array([meta.get('main_track_momentum', 0) for meta in metadata_list])
            n_clusters = np.array([meta.get('n_clusters_in_volume', 0) for meta in metadata_list])
            ax9.scatter(n_clusters[valid_mask], momenta[valid_mask], alpha=0.6)
            ax9.set_xlabel('Number of Clusters in Volume')
            ax9.set_ylabel('Main Track Momentum (GeV/c)')
            ax9.set_title('Momentum vs Number of Clusters')
            ax9.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    # Save plot
    output_file = output_path / 'volume_analysis.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"Plots saved to: {output_file}")
    plt.close()


def main():
    parser = argparse.ArgumentParser(
        description='Analyze volume image metadata and provide statistics'
    )
    parser.add_argument(
        '-j', '--json',
        required=True,
        help='JSON configuration file'
    )
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Enable verbose output'
    )
    parser.add_argument(
        '--no-plots',
        action='store_true',
        help='Skip creating plots'
    )
    
    args = parser.parse_args()
    
    # Load JSON configuration
    with open(args.json, 'r') as f:
        config = json.load(f)
    
    volumes_folder = config.get('volumes_folder', None)
    if not volumes_folder:
        print("Error: 'volumes_folder' not specified in JSON config")
        return 1
    
    volumes_path = Path(volumes_folder)
    if not volumes_path.exists():
        print(f"Error: Volumes folder does not exist: {volumes_folder}")
        return 1
    
    print("="*80)
    print("Volume Analysis Configuration")
    print("="*80)
    print(f"Volumes folder: {volumes_folder}")
    print("="*80)
    print()
    
    # Load all metadata
    metadata_list = load_volume_metadata(volumes_folder, verbose=args.verbose)
    
    if len(metadata_list) == 0:
        print("No volume files to analyze")
        return 1
    
    # Print summary statistics
    print_summary(metadata_list)
    
    # Create plots
    if not args.no_plots:
        output_folder = volumes_path / 'analysis'
        print(f"Creating analysis plots...")
        create_plots(metadata_list, output_folder)
        print()
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
