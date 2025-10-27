#!/usr/bin/env python3
"""
Extract cluster statistics from ROOT files for different energy cuts.
Analyzes MARLEY clusters, background clusters, and main track clusters in view X.
"""

import ROOT
import os
import glob
import numpy as np
import json

def analyze_clusters(root_file_path):
    """Extract cluster statistics from a ROOT file."""
    
    stats = {
        'marley_clusters_viewX': 0,
        'background_clusters_viewX': 0,
        'main_track_clusters_viewX': 0,
        'total_clusters': 0,
        'total_clusters_viewX': 0
    }
    
    try:
        f = ROOT.TFile.Open(root_file_path, "READ")
        if not f or f.IsZombie():
            print(f"Error opening {root_file_path}")
            return None
        
        tree = f.Get("clusters/clusters")
        if not tree:
            print(f"No clusters tree in {root_file_path}")
            f.Close()
            return None
        
        # Count total clusters
        stats['total_clusters'] = tree.GetEntries()
        
        # Iterate through clusters
        for i in range(tree.GetEntries()):
            tree.GetEntry(i)
            
            view = tree.view
            generator = tree.generator_name if hasattr(tree, 'generator_name') else ""
            is_main_track = tree.is_main_track if hasattr(tree, 'is_main_track') else False
            
            # Only count view X clusters
            if view == "X":
                stats['total_clusters_viewX'] += 1
                
                # Check if MARLEY (case insensitive)
                if "marley" in generator.lower():
                    stats['marley_clusters_viewX'] += 1
                    if is_main_track:
                        stats['main_track_clusters_viewX'] += 1
                else:
                    stats['background_clusters_viewX'] += 1
        
        f.Close()
        return stats
        
    except Exception as e:
        print(f"Error processing {root_file_path}: {e}")
        return None


def process_energy_cut_scan(base_path, sample_type, energy_cuts):
    """Process all energy cuts for a given sample type (cc or es)."""
    
    results = []
    
    for ecut in energy_cuts:
        # Format energy cut for folder name
        # The script multiplies by 10, so 0->0, 0.5->5, 1.0->10, etc.
        ecut_x10 = int(ecut * 10)
        ecut_str = f"e{ecut_x10}p0"
        
        # Construct cluster folder path
        cluster_folder = os.path.join(
            base_path,
            f"clusters_{sample_type}_valid_bg_tick3_ch2_min2_tot2_{ecut_str}"
        )
        
        print(f"\nProcessing {sample_type.upper()} energy_cut={ecut}")
        print(f"Looking in: {cluster_folder}")
        
        if not os.path.exists(cluster_folder):
            print(f"  Directory not found, skipping...")
            results.append({
                'energy_cut': ecut,
                'found': False
            })
            continue
        
        # Find all cluster ROOT files in this folder
        cluster_files = glob.glob(os.path.join(cluster_folder, "clusters_*.root"))
        
        if not cluster_files:
            print(f"  No cluster files found")
            results.append({
                'energy_cut': ecut,
                'found': False
            })
            continue
        
        print(f"  Found {len(cluster_files)} cluster file(s)")
        
        # Aggregate statistics from all files
        total_stats = {
            'marley_clusters_viewX': 0,
            'background_clusters_viewX': 0,
            'main_track_clusters_viewX': 0,
            'total_clusters': 0,
            'total_clusters_viewX': 0
        }
        
        for cluster_file in sorted(cluster_files):
            stats = analyze_clusters(cluster_file)
            if stats:
                for key in total_stats:
                    total_stats[key] += stats[key]
        
        print(f"  View X - MARLEY: {total_stats['marley_clusters_viewX']}, "
              f"Background: {total_stats['background_clusters_viewX']}, "
              f"Main track: {total_stats['main_track_clusters_viewX']}")
        
        results.append({
            'energy_cut': ecut,
            'found': True,
            **total_stats
        })
    
    return results


def save_results(cc_results, es_results, output_file='energy_cut_scan_data.txt'):
    """Save results to a text file."""
    
    with open(output_file, 'w') as f:
        f.write("# Energy Cut Scan Results\n")
        f.write("# Format: energy_cut marley_viewX background_viewX main_track_viewX total_viewX found\n\n")
        
        f.write("# CC Results\n")
        f.write("CC_DATA:\n")
        for r in cc_results:
            if r['found']:
                f.write(f"{r['energy_cut']:.1f} {r['marley_clusters_viewX']} "
                       f"{r['background_clusters_viewX']} {r['main_track_clusters_viewX']} "
                       f"{r['total_clusters_viewX']} 1\n")
            else:
                f.write(f"{r['energy_cut']:.1f} 0 0 0 0 0\n")
        
        f.write("\n# ES Results\n")
        f.write("ES_DATA:\n")
        for r in es_results:
            if r['found']:
                f.write(f"{r['energy_cut']:.1f} {r['marley_clusters_viewX']} "
                       f"{r['background_clusters_viewX']} {r['main_track_clusters_viewX']} "
                       f"{r['total_clusters_viewX']} 1\n")
            else:
                f.write(f"{r['energy_cut']:.1f} 0 0 0 0 0\n")
    
    print(f"\nResults saved to {output_file}")


if __name__ == "__main__":
    # Energy cuts to analyze (matching the simulation script)
    energy_cuts = [0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0]
    
    # Base paths for CC and ES data
    cc_base = "/home/virgolaema/dune/online-pointing-utils/data/prod_cc/"
    es_base = "/home/virgolaema/dune/online-pointing-utils/data/prod_es/"
    
    # Process CC samples
    print("="*60)
    print("Processing CC samples")
    print("="*60)
    cc_results = process_energy_cut_scan(cc_base, "cc", energy_cuts)
    
    # Process ES samples
    print("\n" + "="*60)
    print("Processing ES samples")
    print("="*60)
    es_results = process_energy_cut_scan(es_base, "es", energy_cuts)
    
    # Save results
    save_results(cc_results, es_results)
    
    print("\n" + "="*60)
    print("Analysis complete!")
    print("="*60)
