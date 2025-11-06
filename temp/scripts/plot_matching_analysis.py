#!/usr/bin/env python3
"""
Plot matching analysis results from matched cluster files.
Shows MARLEY event identification, purity, and cluster distributions.
"""

import uproot
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import sys

def analyze_matched_clusters(matched_file_path):
    """Analyze matched clusters and create visualizations."""
    
    print(f"Analyzing: {matched_file_path}")
    
    # Open the matched clusters file
    file = uproot.open(matched_file_path)
    tree = file["clusters/clusters_tree_multiplane"]
    
    # Read all relevant branches
    marley_frac = tree["marley_tp_fraction"].array(library="np")
    n_tps = tree["n_tps"].array(library="np")
    total_energy = tree["total_energy"].array(library="np")
    
    # Get time info from TPs (these are arrays of arrays)
    tp_time_start = tree["tp_time_start"].array(library="np")
    # Compute cluster time as mean of TP times
    time_center = np.array([np.mean(times) if len(times) > 0 else 0 for times in tp_time_start])
    
    # Compute statistics
    n_total = len(marley_frac)
    n_marley = np.sum(marley_frac > 0.5)
    n_pure_marley = np.sum(marley_frac == 1.0)
    purity = 100.0 * n_marley / n_total if n_total > 0 else 0
    
    print(f"\n{'='*60}")
    print(f"Matching Analysis Results")
    print(f"{'='*60}")
    print(f"\nTotal multiplane clusters: {n_total}")
    print(f"MARLEY clusters (frac > 0.5): {n_marley} ({100*n_marley/n_total:.1f}%)")
    print(f"Pure MARLEY (frac = 1.0): {n_pure_marley} ({100*n_pure_marley/n_total:.1f}%)")
    print(f"Non-MARLEY clusters: {n_total - n_marley}")
    print(f"\nOverall purity: {purity:.1f}%")
    print(f"\nMarley fraction statistics:")
    print(f"  Min: {marley_frac.min():.3f}")
    print(f"  Max: {marley_frac.max():.3f}")
    print(f"  Mean: {marley_frac.mean():.3f}")
    print(f"  Median: {np.median(marley_frac):.3f}")
    
    print(f"\nCluster size (n_tps) statistics:")
    print(f"  Min: {n_tps.min()}")
    print(f"  Max: {n_tps.max()}")
    print(f"  Mean: {n_tps.mean():.1f}")
    print(f"  Median: {np.median(n_tps):.1f}")
    
    print(f"\nTotal energy [MeV] statistics:")
    print(f"  Min: {total_energy.min():.3f}")
    print(f"  Max: {total_energy.max():.3f}")
    print(f"  Mean: {total_energy.mean():.3f}")
    print(f"  Median: {np.median(total_energy):.3f}")
    
    # Create visualizations
    fig, axes = plt.subplots(2, 3, figsize=(15, 10))
    fig.suptitle('Matched Cluster Analysis', fontsize=16, fontweight='bold')
    
    # 1. MARLEY fraction distribution
    ax = axes[0, 0]
    ax.hist(marley_frac, bins=20, edgecolor='black', alpha=0.7)
    ax.axvline(0.5, color='red', linestyle='--', linewidth=2, label='MARLEY threshold')
    ax.set_xlabel('MARLEY TP Fraction', fontsize=12)
    ax.set_ylabel('Number of Clusters', fontsize=12)
    ax.set_title(f'MARLEY Event Identification\n{n_marley}/{n_total} clusters are MARLEY', fontsize=11)
    ax.legend()
    ax.grid(alpha=0.3)
    
    # 2. Cluster size distribution
    ax = axes[0, 1]
    marley_mask = marley_frac > 0.5
    ax.hist([n_tps[marley_mask], n_tps[~marley_mask]], 
            bins=30, label=['MARLEY', 'Non-MARLEY'], 
            alpha=0.7, edgecolor='black', stacked=True)
    ax.set_xlabel('Number of TPs per Cluster', fontsize=12)
    ax.set_ylabel('Number of Clusters', fontsize=12)
    ax.set_title('Cluster Size Distribution', fontsize=11)
    ax.legend()
    ax.grid(alpha=0.3)
    ax.set_yscale('log')
    
    # 3. Energy distribution
    ax = axes[0, 2]
    ax.hist([total_energy[marley_mask], total_energy[~marley_mask]], 
            bins=30, label=['MARLEY', 'Non-MARLEY'],
            alpha=0.7, edgecolor='black', stacked=True)
    ax.set_xlabel('Total Energy [MeV]', fontsize=12)
    ax.set_ylabel('Number of Clusters', fontsize=12)
    ax.set_title('Energy Distribution', fontsize=11)
    ax.legend()
    ax.grid(alpha=0.3)
    
    # 4. Time distribution
    ax = axes[1, 0]
    ax.hist([time_center[marley_mask], time_center[~marley_mask]], 
            bins=30, label=['MARLEY', 'Non-MARLEY'],
            alpha=0.7, edgecolor='black', stacked=True)
    ax.set_xlabel('Time [ticks]', fontsize=12)
    ax.set_ylabel('Number of Clusters', fontsize=12)
    ax.set_title('Time Distribution', fontsize=11)
    ax.legend()
    ax.grid(alpha=0.3)
    
    # 5. Purity pie chart
    ax = axes[1, 1]
    if n_marley > 0:
        colors = ['#2ecc71', '#e74c3c']
        labels = [f'MARLEY\n({n_marley})', f'Non-MARLEY\n({n_total - n_marley})']
        sizes = [n_marley, n_total - n_marley]
        ax.pie(sizes, labels=labels, colors=colors, autopct='%1.1f%%', 
               startangle=90, textprops={'fontsize': 11})
        ax.set_title(f'Event Type Composition\nPurity: {purity:.1f}%', fontsize=11)
    else:
        ax.text(0.5, 0.5, 'No MARLEY clusters', ha='center', va='center', 
                transform=ax.transAxes, fontsize=12)
        ax.set_title('Event Type Composition', fontsize=11)
    
    # 6. Energy vs Size scatter
    ax = axes[1, 2]
    scatter_marley = ax.scatter(n_tps[marley_mask], total_energy[marley_mask], 
                                 alpha=0.6, s=50, label='MARLEY', c='#2ecc71')
    if np.sum(~marley_mask) > 0:
        scatter_other = ax.scatter(n_tps[~marley_mask], total_energy[~marley_mask], 
                                   alpha=0.6, s=50, label='Non-MARLEY', c='#e74c3c')
    ax.set_xlabel('Number of TPs', fontsize=12)
    ax.set_ylabel('Total Energy [MeV]', fontsize=12)
    ax.set_title('Energy vs Cluster Size', fontsize=11)
    ax.legend()
    ax.grid(alpha=0.3)
    
    plt.tight_layout()
    
    # Save figure
    output_file = matched_file_path.replace('.root', '_analysis.png')
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"\nPlot saved to: {output_file}")
    
    return output_file


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python plot_matching_analysis.py <matched_clusters.root>")
        sys.exit(1)
    
    matched_file = sys.argv[1]
    
    if not Path(matched_file).exists():
        print(f"Error: File not found: {matched_file}")
        sys.exit(1)
    
    output_file = analyze_matched_clusters(matched_file)
    
    print(f"\n{'='*60}")
    print("Analysis complete!")
    print(f"{'='*60}\n")
