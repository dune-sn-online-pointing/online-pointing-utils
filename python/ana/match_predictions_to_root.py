#!/usr/bin/env python3
"""
Match MT predictions to ROOT cluster files to understand what clusters are being
predicted as main tracks (both true positives and false positives).
"""

import pandas as pd
import numpy as np
import ROOT
import argparse
from pathlib import Path
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages


def load_mt_predictions(predictions_csv):
    """Load MT predictions CSV file."""
    print(f"Loading predictions from {predictions_csv}...")
    df = pd.read_csv(predictions_csv)
    print(f"Loaded {len(df)} predictions")
    return df


def get_root_file_path(clusters_dir, event_id):
    """Find ROOT file containing the event."""
    clusters_path = Path(clusters_dir)
    
    # Try ES file first (event 0-49)
    es_file = clusters_path / "es_000000_bg_clusters.root"
    if es_file.exists():
        f = ROOT.TFile.Open(str(es_file))
        if f and not f.IsZombie():
            tree = f.Get("clusters/clusters_tree_X")
            if tree:
                for i in range(tree.GetEntries()):
                    tree.GetEntry(i)
                    if tree.event == event_id:
                        f.Close()
                        return str(es_file)
            f.Close()
    
    # Try CC files (event 50+)
    for i in range(10):  # Assume max 10 CC files
        cc_file = clusters_path / f"cc_{i:06d}_bg_clusters.root"
        if not cc_file.exists():
            break
        
        f = ROOT.TFile.Open(str(cc_file))
        if f and not f.IsZombie():
            tree = f.Get("clusters/clusters_tree_X")
            if tree:
                for j in range(tree.GetEntries()):
                    tree.GetEntry(j)
                    if tree.event == event_id:
                        f.Close()
                        return str(cc_file)
            f.Close()
    
    return None


def analyze_event_predictions(root_file, event_id, predictions_for_event, plane='X'):
    """Analyze all predictions for a specific event using ROOT cluster info."""
    
    f = ROOT.TFile.Open(root_file)
    if not f or f.IsZombie():
        return []
    
    tree = f.Get(f"clusters/clusters_tree_{plane}")
    if not tree:
        f.Close()
        return []
    
    # Get all clusters in this event
    event_clusters = []
    for i in range(tree.GetEntries()):
        tree.GetEntry(i)
        if tree.event != event_id:
            continue
        
        info = {
            'cluster_idx': i,
            'n_hits': tree.n_tps,
            'cluster_energy': tree.total_energy,
            'true_particle_energy': tree.true_particle_energy,
            'is_main_cluster': tree.is_main_cluster,
            'true_pdg': tree.true_pdg,
            'true_label': str(tree.true_label),
            'marley_fraction': tree.marley_tp_fraction,
            'generator_fraction': tree.generator_tp_fraction,
            'is_marley': 'marley' in str(tree.true_label).lower(),
        }
        event_clusters.append(info)
    
    f.Close()
    
    # Now match predictions to clusters
    # This is hard because we don't have exact position matching
    # Simplest approach: use is_main_cluster flag from ROOT as ground truth
    
    results = []
    for _, pred in predictions_for_event.iterrows():
        predicted_as_mt = pred['predicted'] > 0.5
        
        # Try to find the cluster this prediction refers to
        # We know from the data that is_main_cluster in ROOT is the ground truth
        # If predicted as MT and there's a main cluster, it could be TP or FP
        
        # For simplicity: attribute the prediction outcome
        if pred['is_main_track'] == 1 and predicted_as_mt:
            # True positive - must be the main cluster
            cluster = [c for c in event_clusters if c['is_main_cluster']]
            if cluster:
                result = cluster[0].copy()
                result['prediction_outcome'] = 'TP'
                result['prediction_prob'] = pred['predicted']
                results.append(result)
        
        elif pred['is_main_track'] == 0 and predicted_as_mt:
            # False positive - any non-main cluster
            # We can't match exactly, so we'll use ALL non-main clusters
            for cluster in event_clusters:
                if not cluster['is_main_cluster']:
                    result = cluster.copy()
                    result['prediction_outcome'] = 'FP_candidate'
                    result['prediction_prob'] = pred['predicted']
                    results.append(result)
            break  # Only add once per FP prediction
    
    return results


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--predictions', required=True, help='MT predictions CSV')
    parser.add_argument('--clusters', required=True, help='ROOT clusters directory')
    parser.add_argument('--plane', default='X', help='Plane to analyze (U/V/X)')
    parser.add_argument('--max-events', type=int, default=50, help='Max events to analyze')
    parser.add_argument('--output', default='results/matched_predictions_root.pdf')
    args = parser.parse_args()
    
    # Load predictions
    predictions = load_mt_predictions(args.predictions)
    
    # Focus on events with both TP and FP (most interesting)
    event_stats = predictions.groupby('event_id').agg({
        'is_main_track': 'sum',
        'predicted': lambda x: (x > 0.5).sum()
    }).rename(columns={'is_main_track': 'true_mts', 'predicted': 'predicted_mts'})
    
    # Events with false positives
    fp_events = event_stats[event_stats['predicted_mts'] > event_stats['true_mts']]
    print(f"\nFound {len(fp_events)} events with false positives")
    
    # Analyze subset
    analyzed_events = fp_events.head(args.max_events).index.tolist()
    print(f"Analyzing {len(analyzed_events)} events...")
    
    all_results = []
    for event_id in analyzed_events:
        root_file = get_root_file_path(args.clusters, event_id)
        if not root_file:
            print(f"  Skipping event {event_id} - ROOT file not found")
            continue
        
        event_preds = predictions[predictions['event_id'] == event_id]
        results = analyze_event_predictions(root_file, event_id, event_preds, args.plane)
        all_results.extend(results)
        
        if len(results) > 0:
            fp_count = sum(1 for r in results if r['prediction_outcome'] == 'FP_candidate')
            print(f"  Event {event_id}: {len(results)} matched, {fp_count} FP candidates")
    
    if not all_results:
        print("No results to analyze!")
        return
    
    # Convert to DataFrame
    df = pd.DataFrame(all_results)
    
    print(f"\n{'='*70}")
    print("MATCHED PREDICTIONS ANALYSIS")
    print(f"{'='*70}")
    
    # Separate TP and FP
    tp_df = df[df['prediction_outcome'] == 'TP']
    fp_df = df[df['prediction_outcome'] == 'FP_candidate']
    
    print(f"\nCollected {len(tp_df)} TP matches and {len(fp_df)} FP candidates")
    
    if len(fp_df) > 0:
        print(f"\nFALSE POSITIVE CHARACTERISTICS:")
        print(f"\nPDG codes:")
        pdg_counts = fp_df['true_pdg'].value_counts()
        for pdg, count in pdg_counts.items():
            print(f"  PDG {pdg}: {count} ({count/len(fp_df)*100:.1f}%)")
        
        print(f"\nTrue labels:")
        label_counts = fp_df['true_label'].value_counts().head(10)
        for label, count in label_counts.items():
            print(f"  {label}: {count} ({count/len(fp_df)*100:.1f}%)")
        
        print(f"\nEnergy statistics:")
        print(f"  Mean cluster energy: {fp_df['cluster_energy'].mean():.2f} MeV")
        print(f"  Median cluster energy: {fp_df['cluster_energy'].median():.2f} MeV")
        print(f"  Mean true particle energy: {fp_df['true_particle_energy'].mean():.2f} MeV")
        
        print(f"\nMarley fraction:")
        print(f"  Mean: {fp_df['marley_fraction'].mean():.3f}")
        print(f"  Median: {fp_df['marley_fraction'].median():.3f}")
        
        print(f"\nIs Marley:")
        marley_count = fp_df['is_marley'].sum()
        print(f"  Marley: {marley_count} ({marley_count/len(fp_df)*100:.1f}%)")
        print(f"  Non-Marley: {len(fp_df) - marley_count} ({(len(fp_df)-marley_count)/len(fp_df)*100:.1f}%)")
    
    # Create plots
    print(f"\nGenerating plots...")
    with PdfPages(args.output) as pdf:
        # Page 1: Energy comparison
        fig, axes = plt.subplots(2, 2, figsize=(11, 8.5))
        
        # Cluster energy
        ax = axes[0, 0]
        bins = np.linspace(0, 20, 40)
        if len(tp_df) > 0:
            ax.hist(tp_df['cluster_energy'], bins=bins, alpha=0.6, label='TP', 
                   color='blue', edgecolor='darkblue')
        if len(fp_df) > 0:
            ax.hist(fp_df['cluster_energy'], bins=bins, alpha=0.6, label='FP', 
                   color='red', edgecolor='darkred')
        ax.set_xlabel('Cluster Energy (MeV)', fontsize=11)
        ax.set_ylabel('Count', fontsize=11)
        ax.set_title('Cluster Energy Distribution', fontsize=12, fontweight='bold')
        ax.legend(fontsize=9)
        ax.grid(True, alpha=0.3, axis='y')
        
        # Number of hits
        ax = axes[0, 1]
        if len(tp_df) > 0:
            tp_hits = tp_df['n_hits'].value_counts().sort_index()
            ax.bar(tp_hits.index - 0.2, tp_hits.values, width=0.4, alpha=0.6, 
                  label='TP', color='blue', edgecolor='darkblue')
        if len(fp_df) > 0:
            fp_hits = fp_df['n_hits'].value_counts().sort_index()
            ax.bar(fp_hits.index + 0.2, fp_hits.values, width=0.4, alpha=0.6, 
                  label='FP', color='red', edgecolor='darkred')
        ax.set_xlabel('Number of Hits', fontsize=11)
        ax.set_ylabel('Count', fontsize=11)
        ax.set_title('Hit Multiplicity', fontsize=12, fontweight='bold')
        ax.legend(fontsize=9)
        ax.grid(True, alpha=0.3, axis='y')
        
        # Marley fraction
        ax = axes[1, 0]
        bins = np.linspace(0, 1, 20)
        if len(tp_df) > 0:
            ax.hist(tp_df['marley_fraction'], bins=bins, alpha=0.6, label='TP', 
                   color='blue', edgecolor='darkblue')
        if len(fp_df) > 0:
            ax.hist(fp_df['marley_fraction'], bins=bins, alpha=0.6, label='FP', 
                   color='red', edgecolor='darkred')
        ax.set_xlabel('Marley TP Fraction', fontsize=11)
        ax.set_ylabel('Count', fontsize=11)
        ax.set_title('Marley Content', fontsize=12, fontweight='bold')
        ax.legend(fontsize=9)
        ax.grid(True, alpha=0.3, axis='y')
        
        # PDG codes (FP only)
        ax = axes[1, 1]
        if len(fp_df) > 0:
            pdg_counts = fp_df['true_pdg'].value_counts().head(5)
            ax.barh(range(len(pdg_counts)), pdg_counts.values, color='red', alpha=0.6, edgecolor='darkred')
            ax.set_yticks(range(len(pdg_counts)))
            ax.set_yticklabels([f'PDG {int(p)}' for p in pdg_counts.index])
            ax.set_xlabel('Count', fontsize=11)
            ax.set_title('False Positive PDG Codes', fontsize=12, fontweight='bold')
            ax.grid(True, alpha=0.3, axis='x')
        
        plt.suptitle('Matched Predictions: TP vs FP from ROOT', 
                     fontsize=14, fontweight='bold')
        plt.tight_layout(rect=[0, 0, 1, 0.96])
        pdf.savefig(fig, dpi=150)
        plt.close()
    
    print(f"\n✓ Plots saved to: {args.output}")


if __name__ == "__main__":
    main()
