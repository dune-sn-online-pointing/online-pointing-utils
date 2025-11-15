#!/usr/bin/env python3
"""
Analyze false positives from MT inference using ROOT cluster files.
Investigate what makes background clusters look like main tracks.
"""

import ROOT
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import sys
import argparse
from pathlib import Path

# Enable ROOT batch mode
ROOT.gROOT.SetBatch(True)


def load_mt_predictions(predictions_csv):
    """Load MT predictions from inference results."""
    df = pd.read_csv(predictions_csv)
    print(f"Loaded {len(df)} predictions from {predictions_csv}")
    return df


def analyze_cluster_from_root(root_file, event_id, match_id, plane='X'):
    """Extract detailed cluster information from ROOT file."""
    
    f = ROOT.TFile.Open(root_file)
    if not f or f.IsZombie():
        print(f"ERROR: Cannot open {root_file}")
        return None
    
    # Navigate to clusters directory and get the tree for specific plane
    clusters_dir = f.Get("clusters")
    if not clusters_dir:
        print(f"ERROR: Cannot find 'clusters' directory")
        f.Close()
        return None
    
    tree = clusters_dir.Get(f"clusters_tree_{plane}")
    if not tree or not isinstance(tree, ROOT.TTree):
        print(f"ERROR: Cannot find TTree 'clusters_tree_{plane}'")
        f.Close()
        return None
    
    cluster_info = []
    
    for i in range(tree.GetEntries()):
        tree.GetEntry(i)
        
        if tree.event != event_id:
            continue
        
        # Get cluster properties
        info = {
            'event': tree.event,
            'cluster_id': tree.cluster_id if hasattr(tree, 'cluster_id') else -1,
            'match_id': -1,  # Not in this file structure
            'is_main_cluster': tree.is_main_cluster if hasattr(tree, 'is_main_cluster') else False,
            'is_marley': (tree.marley_tp_fraction > 0.5) if hasattr(tree, 'marley_tp_fraction') else False,
            'cluster_energy': tree.total_energy if hasattr(tree, 'total_energy') else 0,
            'true_particle_energy': tree.true_particle_energy if hasattr(tree, 'true_particle_energy') else -1,
            'n_hits': tree.n_tps if hasattr(tree, 'n_tps') else 0,
            'start_tick': tree.tp_time_start if hasattr(tree, 'tp_time_start') else 0,
            'end_tick': tree.tp_time_start if hasattr(tree, 'tp_time_start') else 0,  # Need to compute from hits
            'start_channel': 0,  # Not directly available
            'end_channel': 0,  # Not directly available
            'rms_tick': 0,  # Not directly available
            'rms_channel': 0,  # Not directly available
            'true_pdg': tree.true_pdg if hasattr(tree, 'true_pdg') else 0,
            'true_label': str(tree.true_label) if hasattr(tree, 'true_label') else 'unknown',
            'pos_x': tree.true_pos_x if hasattr(tree, 'true_pos_x') else 0,
            'pos_y': tree.true_pos_y if hasattr(tree, 'true_pos_y') else 0,
            'pos_z': tree.true_pos_z if hasattr(tree, 'true_pos_z') else 0,
            'marley_fraction': tree.marley_tp_fraction if hasattr(tree, 'marley_tp_fraction') else 0,
            'generator_fraction': tree.generator_tp_fraction if hasattr(tree, 'generator_tp_fraction') else 0,
        }
        
        cluster_info.append(info)
    
    f.Close()
    return cluster_info


def analyze_fp_event(root_file, event_id, mt_predictions, plane='X'):
    """Deep analysis of an event with false positives."""
    
    # Get all clusters in event from ROOT
    clusters = analyze_cluster_from_root(root_file, event_id, None, plane)
    if not clusters:
        return None
    
    # Simply categorize by is_main_cluster flag
    mt_clusters = [c for c in clusters if c['is_main_cluster']]
    
    # For FP clusters: ALL non-main clusters (not just Marley)
    # These represent all other clusters in the event that could be misclassified
    non_mt_clusters = [c for c in clusters if not c['is_main_cluster']]
    
    if len(clusters) > 1:  # Only print if there are multiple clusters
        print(f"  Event {event_id}: {len(clusters)} total clusters, {len(mt_clusters)} MT, {len(non_mt_clusters)} non-MT")
    
    return {
        'event_id': event_id,
        'mt_clusters': mt_clusters,
        'fp_clusters': non_mt_clusters,  # All non-main clusters
        'all_clusters': clusters
    }


def compare_mt_vs_fp_properties(mt_clusters, fp_clusters):
    """Compare cluster properties between MT and FP."""
    
    mt_df = pd.DataFrame(mt_clusters) if mt_clusters else pd.DataFrame()
    fp_df = pd.DataFrame(fp_clusters) if fp_clusters else pd.DataFrame()
    
    if len(mt_df) == 0 or len(fp_df) == 0:
        print("No MT or FP clusters to compare")
        return None, None
    
    print(f"\n{'='*70}")
    print(f"CLUSTER PROPERTIES COMPARISON")
    print(f"{'='*70}")
    
    print(f"\nNumber of hits:")
    print(f"  MT: mean={mt_df['n_hits'].mean():.1f}, median={mt_df['n_hits'].median():.0f}, range={mt_df['n_hits'].min()}-{mt_df['n_hits'].max()}")
    print(f"  FP: mean={fp_df['n_hits'].mean():.1f}, median={fp_df['n_hits'].median():.0f}, range={fp_df['n_hits'].min()}-{fp_df['n_hits'].max()}")
    
    print(f"\nCluster energy:")
    print(f"  MT: mean={mt_df['cluster_energy'].mean():.2f} MeV, median={mt_df['cluster_energy'].median():.2f} MeV")
    print(f"  FP: mean={fp_df['cluster_energy'].mean():.2f} MeV, median={fp_df['cluster_energy'].median():.2f} MeV")
    
    print(f"\nTrue particle energy:")
    print(f"  MT: mean={mt_df['true_particle_energy'].mean():.2f} MeV, median={mt_df['true_particle_energy'].median():.2f} MeV")
    print(f"  FP: mean={fp_df['true_particle_energy'].mean():.2f} MeV, median={fp_df['true_particle_energy'].median():.2f} MeV")
    
    print(f"\nTrue particle PDG codes (FPs):")
    pdg_counts = fp_df['true_pdg'].value_counts()
    for pdg, count in pdg_counts.items():
        print(f"  PDG {pdg}: {count} ({count/len(fp_df)*100:.1f}%)")
    
    print(f"\nTrue labels (FPs):")
    label_counts = fp_df['true_label'].value_counts()
    for label, count in label_counts.items():
        print(f"  {label}: {count} ({count/len(fp_df)*100:.1f}%)")
    
    print(f"\nMarley fraction:")
    print(f"  MT: {mt_df['marley_fraction'].mean():.3f}")
    print(f"  FP: {fp_df['marley_fraction'].mean():.3f}")
    
    print(f"\nGenerator fraction:")
    print(f"  MT: {mt_df['generator_fraction'].mean():.3f}")
    print(f"  FP: {fp_df['generator_fraction'].mean():.3f}")
    
    return mt_df, fp_df


def create_comparison_plots(mt_df, fp_df, output_pdf):
    """Generate comparison plots between MT and FP clusters."""
    
    with PdfPages(output_pdf) as pdf:
        # Page 1: Size and extent comparisons
        fig, axes = plt.subplots(2, 2, figsize=(11, 8.5))
        
        # 1. Number of hits
        ax = axes[0, 0]
        bins = np.linspace(0, max(mt_df['n_hits'].max(), fp_df['n_hits'].max()), 30)
        ax.hist(fp_df['n_hits'], bins=bins, alpha=0.6, label=f'FP (n={len(fp_df)})', 
               color='red', edgecolor='darkred')
        ax.hist(mt_df['n_hits'], bins=bins, alpha=0.6, label=f'MT (n={len(mt_df)})', 
               color='blue', edgecolor='darkblue')
        ax.set_xlabel('Number of Hits', fontsize=11)
        ax.set_ylabel('Count', fontsize=11)
        ax.set_title('Cluster Size (Number of Hits)', fontsize=12, fontweight='bold')
        ax.legend(fontsize=9)
        ax.grid(True, alpha=0.3, axis='y')
        
        # 2. Tick extent
        ax = axes[0, 1]
        bins = np.linspace(0, max(mt_df['n_hits'].max(), fp_df['n_hits'].max()), 20)
        ax.hist(fp_df['n_hits'], bins=bins, alpha=0.6, label='FP', 
               color='red', edgecolor='darkred')
        ax.hist(mt_df['n_hits'], bins=bins, alpha=0.6, label='MT', 
               color='blue', edgecolor='darkblue')
        ax.set_xlabel('Number of Hits', fontsize=11)
        ax.set_ylabel('Count', fontsize=11)
        ax.set_title('Cluster Size Distribution', fontsize=12, fontweight='bold')
        ax.legend(fontsize=9)
        ax.grid(True, alpha=0.3, axis='y')
        
        # 3. True particle energy
        ax = axes[1, 0]
        bins = np.linspace(0, max(mt_df['true_particle_energy'].max(), fp_df['true_particle_energy'].max()), 30)
        ax.hist(fp_df['true_particle_energy'], bins=bins, alpha=0.6, label='FP', 
               color='red', edgecolor='darkred')
        ax.hist(mt_df['true_particle_energy'], bins=bins, alpha=0.6, label='MT', 
               color='blue', edgecolor='darkblue')
        ax.set_xlabel('True Particle Energy (MeV)', fontsize=11)
        ax.set_ylabel('Count', fontsize=11)
        ax.set_title('Parent Particle Energy', fontsize=12, fontweight='bold')
        ax.legend(fontsize=9)
        ax.grid(True, alpha=0.3, axis='y')
        
        # 4. Energy comparison
        ax = axes[1, 1]
        bins = np.linspace(0, max(mt_df['cluster_energy'].max(), fp_df['cluster_energy'].max()), 30)
        ax.hist(fp_df['cluster_energy'], bins=bins, alpha=0.6, label='FP', 
               color='red', edgecolor='darkred')
        ax.hist(mt_df['cluster_energy'], bins=bins, alpha=0.6, label='MT', 
               color='blue', edgecolor='darkblue')
        ax.set_xlabel('Cluster Energy (MeV)', fontsize=11)
        ax.set_ylabel('Count', fontsize=11)
        ax.set_title('Energy Distribution', fontsize=12, fontweight='bold')
        ax.legend(fontsize=9)
        ax.grid(True, alpha=0.3, axis='y')
        
        plt.suptitle('Cluster Properties: Main Track vs False Positives', 
                     fontsize=14, fontweight='bold')
        plt.tight_layout(rect=[0, 0, 1, 0.96])
        pdf.savefig(fig, dpi=150)
        plt.close()
        
        # Page 2: Shape and PDG info
        fig, axes = plt.subplots(2, 2, figsize=(11, 8.5))
        
        # 1. Marley fraction
        ax = axes[0, 0]
        bins = np.linspace(0, 1, 20)
        ax.hist(fp_df['marley_fraction'], bins=bins, alpha=0.6, label='FP', 
               color='red', edgecolor='darkred')
        ax.hist(mt_df['marley_fraction'], bins=bins, alpha=0.6, label='MT', 
               color='blue', edgecolor='darkblue')
        ax.set_xlabel('Marley TP Fraction', fontsize=11)
        ax.set_ylabel('Count', fontsize=11)
        ax.set_title('Marley Content', fontsize=12, fontweight='bold')
        ax.legend(fontsize=9)
        ax.grid(True, alpha=0.3, axis='y')
        
        # 2. PDG distribution (FPs only)
        ax = axes[0, 1]
        pdg_counts = fp_df['true_pdg'].value_counts()
        ax.bar(range(len(pdg_counts)), pdg_counts.values, color='crimson', 
              edgecolor='darkred', alpha=0.7)
        ax.set_xticks(range(len(pdg_counts)))
        ax.set_xticklabels([f'PDG {p}' for p in pdg_counts.index], rotation=45, ha='right')
        ax.set_ylabel('Count', fontsize=11)
        ax.set_title('FP Particle Types (PDG codes)', fontsize=12, fontweight='bold')
        ax.grid(True, alpha=0.3, axis='y')
        
        # 3. Energy vs n_hits
        ax = axes[1, 0]
        ax.scatter(mt_df['n_hits'], mt_df['cluster_energy'], alpha=0.6, s=50, 
                  label='MT', color='blue', edgecolors='darkblue')
        ax.scatter(fp_df['n_hits'], fp_df['cluster_energy'], alpha=0.6, s=50, 
                  label='FP', color='red', edgecolors='darkred')
        ax.set_xlabel('Number of Hits', fontsize=11)
        ax.set_ylabel('Cluster Energy (MeV)', fontsize=11)
        ax.set_title('Energy vs Cluster Size', fontsize=12, fontweight='bold')
        ax.legend(fontsize=9)
        ax.grid(True, alpha=0.3)
        
        # 4. Generator fraction
        ax = axes[1, 1]
        bins = np.linspace(0, 1, 20)
        ax.hist(fp_df['generator_fraction'], bins=bins, alpha=0.6, label='FP', 
               color='red', edgecolor='darkred')
        ax.hist(mt_df['generator_fraction'], bins=bins, alpha=0.6, label='MT', 
               color='blue', edgecolor='darkblue')
        ax.set_xlabel('Generator TP Fraction', fontsize=11)
        ax.set_ylabel('Count', fontsize=11)
        ax.set_title('Generator Content', fontsize=12, fontweight='bold')
        ax.legend(fontsize=9)
        ax.grid(True, alpha=0.3, axis='y')
        
        plt.suptitle('Detailed Cluster Characteristics', 
                     fontsize=14, fontweight='bold')
        plt.tight_layout(rect=[0, 0, 1, 0.96])
        pdf.savefig(fig, dpi=150)
        plt.close()
    
    print(f"\n✓ Plots saved to: {output_pdf}")


def main():
    parser = argparse.ArgumentParser(
        description='Analyze false positives using ROOT cluster files',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Analyze cat000001 with MT predictions
  python analyze_fp_clusters.py \\
    --predictions /path/to/mt_predictions.csv \\
    --clusters /eos/.../cat000001_clusters_tick3_ch2_min2_tot3_e2p0/ \\
    --output fp_root_analysis.pdf
        """
    )
    
    parser.add_argument('--predictions', type=str, required=True,
                        help='Path to MT predictions CSV file')
    parser.add_argument('--clusters', type=str, required=True,
                        help='Path to cluster ROOT files directory')
    parser.add_argument('--plane', type=str, default='X',
                        help='Plane to analyze (default: X)')
    parser.add_argument('--output', type=str, default='results/fp_root_analysis.pdf',
                        help='Output PDF file')
    parser.add_argument('--max-events', type=int, default=50,
                        help='Maximum number of events to analyze')
    
    args = parser.parse_args()
    
    # Load predictions
    mt_predictions = load_mt_predictions(args.predictions)
    
    # Get events with false positives
    fp_events = mt_predictions[
        (mt_predictions['is_main_track'] == 0) & 
        (mt_predictions['predicted_label'] == 1)
    ]['event'].unique()
    
    print(f"\nFound {len(fp_events)} events with false positives")
    print(f"Analyzing up to {args.max_events} events...")
    
    # Collect all MT and FP clusters
    all_mt = []
    all_fp = []
    
    for i, event_id in enumerate(fp_events[:args.max_events]):
        # Find corresponding ROOT file
        # Determine if ES or CC based on file index
        file_idx = mt_predictions[mt_predictions['event'] == event_id]['file_index'].values[0]
        
        if file_idx < 10:  # ES files
            root_file = f"{args.clusters}/es_{file_idx:06d}_bg_clusters.root"
        else:  # CC files
            cc_idx = file_idx - 10
            root_file = f"{args.clusters}/cc_{cc_idx:06d}_bg_clusters.root"
        
        if not Path(root_file).exists():
            print(f"Warning: {root_file} not found, skipping event {event_id}")
            continue
        
        print(f"Analyzing event {event_id} from {Path(root_file).name}...")
        
        event_analysis = analyze_fp_event(root_file, event_id, mt_predictions, args.plane)
        
        if event_analysis:
            all_mt.extend(event_analysis['mt_clusters'])
            all_fp.extend(event_analysis['fp_clusters'])
    
    print(f"\n{'='*70}")
    print(f"Collected {len(all_mt)} MT clusters and {len(all_fp)} FP clusters")
    print(f"{'='*70}")
    
    if len(all_mt) > 0 and len(all_fp) > 0:
        mt_df, fp_df = compare_mt_vs_fp_properties(all_mt, all_fp)
        
        if mt_df is not None and fp_df is not None:
            # Create output directory
            Path(args.output).parent.mkdir(parents=True, exist_ok=True)
            create_comparison_plots(mt_df, fp_df, args.output)
            
            # Save detailed CSV
            csv_output = args.output.replace('.pdf', '_clusters.csv')
            all_clusters = all_mt + all_fp
            df_all = pd.DataFrame(all_clusters)
            df_all['cluster_type'] = ['MT'] * len(all_mt) + ['FP'] * len(all_fp)
            df_all.to_csv(csv_output, index=False)
            print(f"\n✓ Cluster data saved to: {csv_output}")
            print(f"  Saved {len(all_mt)} MT and {len(all_fp)} FP/non-MT clusters")
    else:
        print("ERROR: Not enough data to compare")


if __name__ == '__main__':
    main()
