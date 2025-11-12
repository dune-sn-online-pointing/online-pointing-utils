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


def load_volume_metadata(volume_folder, verbose=False, max_files=None, skip_files=0):
    """
    Load metadata from all volume NPZ files in the folder.
    
    Args:
        volume_folder: Path to folder containing volume NPZ files
        verbose: Enable verbose output
        max_files: Maximum number of files to process (None = all)
        skip_files: Number of files to skip from beginning
        
    Returns:
        list of metadata dictionaries
    """
    volume_files = sorted(Path(volume_folder).glob('*.npz'))
    
    if len(volume_files) == 0:
        print(f"No volume files found in {volume_folder}")
        return []
    
    # Apply skip and max
    if skip_files > 0:
        if verbose:
            print(f"Skipping first {skip_files} files")
        volume_files = volume_files[skip_files:]
    if max_files is not None:
        if verbose:
            print(f"Processing at most {max_files} files")
        volume_files = volume_files[:max_files]
    
    if verbose:
        print(f"Found {len(volume_files)} volume files")
    
    all_metadata = []
    for vol_file in volume_files:
        try:
            npz = np.load(vol_file, allow_pickle=True)
            # Each NPZ file can contain multiple volumes - load all metadata entries
            metadata_array = npz['metadata']
            if metadata_array.shape == ():
                # Single metadata entry (scalar)
                all_metadata.append(metadata_array.item())
            else:
                # Multiple metadata entries (array)
                for meta in metadata_array:
                    all_metadata.append(meta)
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


def analyze_marley_distances(metadata_list):
    """
    Analyze MARLEY cluster distances from main track.
    
    Returns:
        dict with distance statistics
    """
    avg_distances = []
    max_distances = []
    
    for meta in metadata_list:
        avg_dist = meta.get('avg_marley_cluster_distance_cm', None)
        max_dist = meta.get('max_marley_cluster_distance_cm', None)
        
        # Only include positive distances (valid measurements)
        if avg_dist is not None and avg_dist > 0:
            avg_distances.append(avg_dist)
        if max_dist is not None and max_dist > 0:
            max_distances.append(max_dist)
    
    if len(avg_distances) == 0:
        return None
    
    avg_distances = np.array(avg_distances)
    max_distances = np.array(max_distances)
    
    return {
        'avg_mean': np.mean(avg_distances),
        'avg_std': np.std(avg_distances),
        'avg_median': np.median(avg_distances),
        'avg_min': np.min(avg_distances),
        'avg_max': np.max(avg_distances),
        'avg_values': avg_distances,
        'max_mean': np.mean(max_distances),
        'max_std': np.std(max_distances),
        'max_median': np.median(max_distances),
        'max_min': np.min(max_distances),
        'max_max': np.max(max_distances),
        'max_values': max_distances,
    }


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
        # Display in MeV
        print(f"Mean:   {energy_stats['mean']*1000:7.2f} MeV")
        print(f"Median: {energy_stats['median']*1000:7.2f} MeV")
        print(f"Std:    {energy_stats['std']*1000:7.2f} MeV")
        print(f"Min:    {energy_stats['min']*1000:7.2f} MeV")
        print(f"Max:    {energy_stats['max']*1000:7.2f} MeV")
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
    
    # MARLEY cluster distance distribution
    print("-" * 80)
    print("MARLEY CLUSTER DISTANCES FROM MAIN TRACK")
    print("-" * 80)
    dist_stats = analyze_marley_distances(metadata_list)
    if dist_stats:
        print("Average distance per volume:")
        print(f"  Mean:   {dist_stats['avg_mean']:7.2f} cm")
        print(f"  Median: {dist_stats['avg_median']:7.2f} cm")
        print(f"  Std:    {dist_stats['avg_std']:7.2f} cm")
        print(f"  Range:  {dist_stats['avg_min']:7.2f} - {dist_stats['avg_max']:7.2f} cm")
        print()
        print("Maximum distance per volume:")
        print(f"  Mean:   {dist_stats['max_mean']:7.2f} cm")
        print(f"  Median: {dist_stats['max_median']:7.2f} cm")
        print(f"  Std:    {dist_stats['max_std']:7.2f} cm")
        print(f"  Range:  {dist_stats['max_min']:7.2f} - {dist_stats['max_max']:7.2f} cm")
    else:
        print("No MARLEY distance information available")
    print()
    
    print("="*80)


def create_plots(metadata_list, output_folder, conditions_name):
    """
    Create visualization plots and save them as a multi-page PDF.
    """
    from matplotlib.backends.backend_pdf import PdfPages
    
    output_path = Path(output_folder)
    output_path.mkdir(parents=True, exist_ok=True)
    
    # Get statistics
    comp_stats = analyze_cluster_composition(metadata_list)
    energy_stats = analyze_energies(metadata_list)
    mom_stats = analyze_momentum(metadata_list)
    interaction_stats = analyze_interaction_types(metadata_list)
    dist_stats = analyze_marley_distances(metadata_list)
    
    # Create multi-page PDF with descriptive name
    output_file = output_path / f'volume_analysis_{conditions_name}.pdf'
    
    with PdfPages(output_file) as pdf:
        # Page 1: Composition and basic statistics
        fig1 = plt.figure(figsize=(16, 10))
        
        # 1. MARLEY fraction distribution
        ax1 = plt.subplot(2, 3, 1)
        if len(comp_stats['marley_fractions']) > 0:
            ax1.hist(comp_stats['marley_fractions'], bins=20, edgecolor='black', alpha=0.7)
            ax1.set_xlabel('MARLEY Fraction per Volume')
            ax1.set_ylabel('Count')
            ax1.set_title('Distribution of MARLEY Fraction')
            ax1.grid(True, alpha=0.3)
        
        # 2. Clusters per volume
        ax2 = plt.subplot(2, 3, 2)
        if len(comp_stats['clusters_per_volume']) > 0:
            ax2.hist(comp_stats['clusters_per_volume'], bins=20, edgecolor='black', alpha=0.7)
            ax2.set_xlabel('Number of Clusters')
            ax2.set_ylabel('Count')
            ax2.set_title('Clusters per Volume')
            ax2.grid(True, alpha=0.3)
        
        # 3. Volume composition pie chart (fixed with legend)
        ax3 = plt.subplot(2, 3, 3)
        labels = ['Pure MARLEY', 'Pure Non-MARLEY', 'Mixed']
        sizes = [comp_stats['pure_marley'], comp_stats['pure_non_marley'], comp_stats['mixed']]
        colors = ['#ff9999', '#66b3ff', '#99ff99']
        wedges, texts, autotexts = ax3.pie(sizes, colors=colors, autopct='%1.1f%%', startangle=90)
        ax3.legend(wedges, labels, loc="center left", bbox_to_anchor=(1, 0, 0.5, 1))
        ax3.set_title('Volume Composition')
        
        # 4. Energy distribution (MeV)
        ax4 = plt.subplot(2, 3, 4)
        if energy_stats:
            # Convert GeV to MeV
            energies_mev = energy_stats['values'] * 1000
            ax4.hist(energies_mev, bins=30, edgecolor='black', alpha=0.7)
            ax4.set_xlabel('Particle Energy (MeV)')
            ax4.set_ylabel('Count')
            ax4.set_title('Particle Energy Distribution')
            ax4.grid(True, alpha=0.3)
        
        # 5. Momentum distribution
        ax5 = plt.subplot(2, 3, 5)
        if mom_stats:
            ax5.hist(mom_stats['values'], bins=30, edgecolor='black', alpha=0.7)
            ax5.set_xlabel('Main Track Momentum (GeV/c)')
            ax5.set_ylabel('Count')
            ax5.set_title('Main Track Momentum Distribution')
            ax5.grid(True, alpha=0.3)
        
        # 6. MARLEY vs non-MARLEY clusters total
        ax6 = plt.subplot(2, 3, 6)
        cluster_types = ['MARLEY', 'Non-MARLEY']
        cluster_counts = [comp_stats['total_marley_clusters'], comp_stats['total_non_marley_clusters']]
        bar_colors = ['#ff9999', '#66b3ff']
        ax6.bar(cluster_types, cluster_counts, color=bar_colors, edgecolor='black', alpha=0.7)
        ax6.set_ylabel('Total Clusters')
        ax6.set_title('Total MARLEY vs Non-MARLEY Clusters')
        ax6.grid(True, alpha=0.3, axis='y')
        
        plt.tight_layout()
        pdf.savefig(fig1, bbox_inches='tight')
        plt.close(fig1)
        
        # Page 2: Correlations
        fig2 = plt.figure(figsize=(16, 10))
        
        # 1. Energy vs MARLEY fraction scatter
        ax1 = plt.subplot(2, 3, 1)
        if energy_stats and len(comp_stats['marley_fractions']) > 0:
            marley_fracs = np.array(comp_stats['marley_fractions'])
            valid_mask = np.array([meta.get('particle_energy', 0) > 0 for meta in metadata_list])
            if np.sum(valid_mask) > 0:
                energies = np.array([meta.get('particle_energy', 0) for meta in metadata_list]) * 1000  # MeV
                ax1.scatter(marley_fracs[valid_mask], energies[valid_mask], alpha=0.6)
                ax1.set_xlabel('MARLEY Fraction')
                ax1.set_ylabel('Particle Energy (MeV)')
                ax1.set_title('Energy vs MARLEY Fraction')
                ax1.grid(True, alpha=0.3)
        
        # 2. Momentum vs number of clusters scatter
        ax2 = plt.subplot(2, 3, 2)
        if mom_stats:
            valid_mask = np.array([meta.get('main_track_momentum', 0) > 0 for meta in metadata_list])
            if np.sum(valid_mask) > 0:
                momenta = np.array([meta.get('main_track_momentum', 0) for meta in metadata_list])
                n_clusters = np.array([meta.get('n_clusters_in_volume', 0) for meta in metadata_list])
                ax2.scatter(n_clusters[valid_mask], momenta[valid_mask], alpha=0.6)
                ax2.set_xlabel('Number of Clusters in Volume')
                ax2.set_ylabel('Main Track Momentum (GeV/c)')
                ax2.set_title('Momentum vs Number of Clusters')
                ax2.grid(True, alpha=0.3)
        
        # 3. Interaction types bar chart
        ax3 = plt.subplot(2, 3, 3)
        int_types = list(interaction_stats.keys())
        int_counts = list(interaction_stats.values())
        ax3.bar(int_types, int_counts, edgecolor='black', alpha=0.7)
        ax3.set_xlabel('Interaction Type')
        ax3.set_ylabel('Count')
        ax3.set_title('Interaction Types')
        ax3.grid(True, alpha=0.3, axis='y')
        
        # 4. Average MARLEY distance distribution
        ax4 = plt.subplot(2, 3, 4)
        if dist_stats:
            ax4.hist(dist_stats['avg_values'], bins=30, edgecolor='black', alpha=0.7, color='orange')
            ax4.set_xlabel('Average Distance (cm)')
            ax4.set_ylabel('Count')
            ax4.set_title('Avg MARLEY Cluster Distance from Main Track')
            ax4.grid(True, alpha=0.3)
            ax4.axvline(dist_stats['avg_mean'], color='red', linestyle='--', linewidth=2, 
                       label=f'Mean: {dist_stats["avg_mean"]:.1f} cm')
            ax4.legend()
        
        # 5. Maximum MARLEY distance distribution
        ax5 = plt.subplot(2, 3, 5)
        if dist_stats:
            ax5.hist(dist_stats['max_values'], bins=30, edgecolor='black', alpha=0.7, color='red')
            ax5.set_xlabel('Maximum Distance (cm)')
            ax5.set_ylabel('Count')
            ax5.set_title('Max MARLEY Cluster Distance Across Events')
            ax5.grid(True, alpha=0.3)
            ax5.axvline(dist_stats['max_mean'], color='darkred', linestyle='--', linewidth=2, 
                       label=f'Mean: {dist_stats["max_mean"]:.1f} cm')
            ax5.legend()
        
        # 6. Max distance vs energy scatter
        ax6 = plt.subplot(2, 3, 6)
        if dist_stats and energy_stats:
            max_dists = []
            energies = []
            for meta in metadata_list:
                max_dist = meta.get('max_marley_cluster_distance_cm', -1)
                energy = meta.get('particle_energy', -1)
                if max_dist > 0 and energy > 0:
                    max_dists.append(max_dist)
                    energies.append(energy * 1000)  # MeV
            if len(max_dists) > 0:
                ax6.scatter(energies, max_dists, alpha=0.6, color='purple')
                ax6.set_xlabel('Particle Energy (MeV)')
                ax6.set_ylabel('Max MARLEY Distance (cm)')
                ax6.set_title('Max MARLEY Distance vs Energy')
                ax6.grid(True, alpha=0.3)
        
        plt.tight_layout()
        pdf.savefig(fig2, bbox_inches='tight')
        plt.close(fig2)
    
    print(f"Plots saved to: {output_file}")


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
    parser.add_argument(
        '-m', '--max-files',
        type=int,
        default=None,
        help='Maximum number of files to process'
    )
    parser.add_argument(
        '-s', '--skip-files',
        type=int,
        default=0,
        help='Number of files to skip from the beginning'
    )
    
    args = parser.parse_args()
    
    # Load JSON configuration
    with open(args.json, 'r') as f:
        config = json.load(f)
    
    # Auto-generate volumes_folder if not explicitly provided
    volumes_folder = config.get('volumes_folder', None)
    if not volumes_folder:
        # Auto-generate from main_folder or signal_folder (matching create_volumes.py logic)
        base_folder = ""
        if "main_folder" in config and config["main_folder"]:
            base_folder = config["main_folder"]
        elif "signal_folder" in config and config["signal_folder"]:
            base_folder = config["signal_folder"]
        else:
            base_folder = config.get('tpstream_folder', '.')
        
        if base_folder.endswith('/'):
            base_folder = base_folder[:-1]
        
        prefix = config.get('products_prefix', config.get('clusters_folder_prefix', 'volumes'))
        
        # Build conditions string from clustering parameters
        tick_limit = config.get("tick_limit", 0)
        channel_limit = config.get("channel_limit", 0)
        min_tps_to_cluster = config.get("min_tps_to_cluster", 0)
        tot_cut = config.get("tot_cut", 0)
        energy_cut = float(config.get("energy_cut", 0.0))
        
        def sanitize(value):
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
        
        conditions = (
            f"tick{sanitize(tick_limit)}"
            f"_ch{sanitize(channel_limit)}"
            f"_min{sanitize(min_tps_to_cluster)}"
            f"_tot{sanitize(tot_cut)}"
            f"_e{sanitize(energy_cut)}"
        )
        
        volumes_folder = f"{base_folder}/volume_images_{prefix}_{conditions}"
    
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
    metadata_list = load_volume_metadata(
        volumes_folder, 
        verbose=args.verbose,
        max_files=args.max_files,
        skip_files=args.skip_files
    )
    
    if len(metadata_list) == 0:
        print("No volume files to analyze")
        return 1
    
    # Print summary statistics
    print_summary(metadata_list)
    
    # Create plots
    if not args.no_plots:
        # Save to reports/ folder under base_folder (main_folder or signal_folder)
        base_folder = ""
        if "main_folder" in config and config["main_folder"]:
            base_folder = config["main_folder"]
        elif "signal_folder" in config and config["signal_folder"]:
            base_folder = config["signal_folder"]
        else:
            base_folder = "."
        
        if base_folder.endswith('/'):
            base_folder = base_folder[:-1]
        
        reports_folder = Path(base_folder) / 'reports'
        reports_folder.mkdir(parents=True, exist_ok=True)
        
        # Extract conditions from volumes_folder name
        # e.g., "volume_images_es_valid_bg_tick3_ch2_min2_tot3_e2p0" -> "es_valid_bg_tick3_ch2_min2_tot3_e2p0"
        folder_name = volumes_path.name
        if folder_name.startswith('volume_images_'):
            conditions_name = folder_name.replace('volume_images_', '')
        else:
            conditions_name = folder_name
        
        print(f"Creating analysis plots...")
        create_plots(metadata_list, reports_folder, conditions_name)
        print()
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
