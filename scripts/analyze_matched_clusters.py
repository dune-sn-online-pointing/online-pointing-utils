#!/usr/bin/env python3
"""
Analyze matched clusters and create performance plots.

This script reads matched_clusters files and generates plots showing:
1. Number of matches per file
2. MARLEY purity of matched clusters
3. Match efficiency (matched/total clusters)
4. Matching quality vs energy

Usage:
    python analyze_matched_clusters.py --sample es_valid
"""

import uproot
import numpy as np
import argparse
from pathlib import Path
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend

def read_matched_clusters(file_path):
    """Read matched clusters from ROOT file."""
    try:
        f = uproot.open(file_path)
    except Exception as e:
        print(f"Error opening file: {file_path}: {e}")
        return None
    
    data = {'U': [], 'V': [], 'X': []}
    
    for view in ['U', 'V', 'X']:
        tree_name = f'clusters/clusters_tree_{view}'
        if tree_name not in f:
            print(f"Warning: Tree {tree_name} not found in {file_path}")
            continue
        
        tree = f[tree_name]
        arrays = tree.arrays(['match_id', 'match_type', 'true_neutrino_energy',
                              'true_particle_energy', 'true_label', 'n_tps',
                              'marley_tp_fraction'],
                             library='np')
        
        for i in range(len(arrays['match_id'])):
            entry = {
                'match_id': arrays['match_id'][i],
                'match_type': arrays['match_type'][i],
                'true_neutrino_energy': arrays['true_neutrino_energy'][i],
                'true_particle_energy': arrays['true_particle_energy'][i],
                'generator_label': 'marley' if arrays['marley_tp_fraction'][i] > 0.5 else 'other',
                'true_label': arrays['true_label'][i].decode('utf-8') if isinstance(arrays['true_label'][i], bytes) else str(arrays['true_label'][i]),
                'n_tps': arrays['n_tps'][i],
                'marley_fraction': arrays['marley_tp_fraction'][i]
            }
            data[view].append(entry)
    
    return data

def analyze_single_file(file_path):
    """Analyze a single matched clusters file."""
    data = read_matched_clusters(str(file_path))
    if not data:
        return None
    
    stats = {}
    
    # For each view, count matches
    for view in ['U', 'V', 'X']:
        clusters = data[view]
        matched = [c for c in clusters if c['match_id'] >= 0]
        triple_matched = [c for c in clusters if c['match_type'] == 3]
        
        # MARLEY purity
        marley_matched = [c for c in matched if 'marley' in c['generator_label'].lower()]
        
        stats[view] = {
            'total': len(clusters),
            'matched': len(matched),
            'triple_matched': len(triple_matched),
            'marley_purity': len(marley_matched) / len(matched) if matched else 0,
            'match_efficiency': len(matched) / len(clusters) if clusters else 0
        }
    
    return stats

def main():
    parser = argparse.ArgumentParser(description='Analyze matched clusters')
    parser.add_argument('--sample', default='es_valid', help='Sample name (es_valid or cc_valid)')
    parser.add_argument('--max-files', type=int, default=None, help='Maximum files to analyze')
    parser.add_argument('--skip-files', type=int, default=0, help='Number of files to skip from beginning')
    args = parser.parse_args()
    
    # Find matched clusters folder
    base = Path('/home/virgolaema/dune/online-pointing-utils/data')
    sample_type = 'prod_es' if 'es' in args.sample else 'prod_cc'
    
    matched_folder = base / sample_type / f'matched_clusters_{args.sample}_tick3_ch2_min2_tot1_e0p0'
    
    if not matched_folder.exists():
        print(f"Error: Matched clusters folder not found: {matched_folder}")
        return
    
    # Find all matched files
    matched_files = sorted(list(matched_folder.glob('*_matched.root')))
    if args.skip_files > 0:
        matched_files = matched_files[args.skip_files:]
    if args.max_files:
        matched_files = matched_files[:args.max_files]
    
    print(f"Found {len(matched_files)} matched cluster files")
    
    # Analyze all files
    all_stats = []
    for i, fpath in enumerate(matched_files):
        print(f"[{i+1}/{len(matched_files)}] Analyzing {fpath.name}")
        stats = analyze_single_file(fpath)
        if stats:
            all_stats.append(stats)
    
    # Create summary plots
    create_summary_plots(all_stats, args.sample)

def create_summary_plots(all_stats, sample):
    """Create summary plots of matching performance."""
    
    fig, axs = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle(f'Matching Performance - {sample.upper()}', fontsize=16)
    
    views = ['U', 'V', 'X']
    colors = {'U': 'blue', 'V': 'green', 'X': 'red'}
    
    # Plot 1: Number of matches per file
    ax = axs[0, 0]
    for view in views:
        matched_per_file = [s[view]['matched'] for s in all_stats]
        ax.plot(matched_per_file, label=view, color=colors[view], marker='o')
    ax.set_xlabel('File Index')
    ax.set_ylabel('Number of Matched Clusters')
    ax.set_title('Matched Clusters per File')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Plot 2: Match efficiency per view
    ax = axs[0, 1]
    for view in views:
        efficiency = [s[view]['match_efficiency'] * 100 for s in all_stats]
        mean_eff = np.mean(efficiency)
        ax.plot(efficiency, label=f'{view} (mean={mean_eff:.1f}%)', color=colors[view], marker='o')
    ax.set_xlabel('File Index')
    ax.set_ylabel('Match Efficiency (%)')
    ax.set_title('Match Efficiency = Matched / Total Clusters')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Plot 3: MARLEY purity per view
    ax = axs[1, 0]
    for view in views:
        purity = [s[view]['marley_purity'] * 100 for s in all_stats]
        mean_purity = np.mean(purity)
        ax.plot(purity, label=f'{view} (mean={mean_purity:.1f}%)', color=colors[view], marker='o')
    ax.set_xlabel('File Index')
    ax.set_ylabel('MARLEY Purity (%)')
    ax.set_title('MARLEY Purity in Matched Clusters')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_ylim([50, 105])
    
    # Plot 4: Average statistics
    ax = axs[1, 1]
    x_pos = np.arange(len(views))
    
    matched_avg = [np.mean([s[v]['matched'] for s in all_stats]) for v in views]
    total_avg = [np.mean([s[v]['total'] for s in all_stats]) for v in views]
    
    width = 0.35
    ax.bar(x_pos - width/2, total_avg, width, label='Total Clusters', alpha=0.7, color='gray')
    ax.bar(x_pos + width/2, matched_avg, width, label='Matched Clusters', alpha=0.7, color='orange')
    
    ax.set_xlabel('View')
    ax.set_ylabel('Average Number of Clusters')
    ax.set_title('Average Clusters per File')
    ax.set_xticks(x_pos)
    ax.set_xticklabels(views)
    ax.legend()
    ax.grid(True, alpha=0.3, axis='y')
    
    # Add text annotations
    for i, v in enumerate(views):
        eff = 100 * matched_avg[i] / total_avg[i] if total_avg[i] > 0 else 0
        ax.text(i, matched_avg[i] + 5, f'{eff:.1f}%', ha='center', fontsize=9)
    
    plt.tight_layout()
    
    output_file = f'/home/virgolaema/dune/online-pointing-utils/output/matching_performance_{sample}.png'
    plt.savefig(output_file, dpi=150)
    print(f"\nSaved plot: {output_file}")
    
    # Print summary statistics
    print("\n" + "="*60)
    print(f"MATCHING PERFORMANCE SUMMARY - {sample.upper()}")
    print("="*60)
    for view in views:
        print(f"\n{view} View:")
        total_clusters = sum([s[view]['total'] for s in all_stats])
        matched_clusters = sum([s[view]['matched'] for s in all_stats])
        triple_matched = sum([s[view]['triple_matched'] for s in all_stats])
        avg_purity = np.mean([s[view]['marley_purity'] * 100 for s in all_stats])
        
        print(f"  Total clusters: {total_clusters}")
        print(f"  Matched clusters: {matched_clusters}")
        print(f"  Triple matches (U+V+X): {triple_matched}")
        if total_clusters > 0:
            print(f"  Match efficiency: {100*matched_clusters/total_clusters:.2f}%")
        else:
            print(f"  Match efficiency: N/A (no clusters)")
        print(f"  Average MARLEY purity: {avg_purity:.2f}%")
    
    print("\n" + "="*60)

if __name__ == '__main__':
    main()
