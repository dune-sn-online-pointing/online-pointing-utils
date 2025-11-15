#!/usr/bin/env python3
"""
Verify that the main cluster selection is correct.

This script checks that in the matched_clusters files, the cluster marked as
'main' (is_main==True) is actually the cluster with the highest total_energy
in each event.

Usage:
    python verify_main_cluster.py --sample es --max-files 10
    python verify_main_cluster.py --sample cc --max-files 10
"""

import uproot
import numpy as np
import argparse
from pathlib import Path
import sys

def check_main_cluster_in_file(file_path, verbose=False):
    """
    Check main cluster correctness in a single file.
    
    Returns:
        dict with statistics: total_events, correct, incorrect, no_main, multiple_main
    """
    stats = {
        'total_events': 0,
        'correct': 0,
        'incorrect': 0,
        'no_main': 0,
        'multiple_main': 0,
        'inconsistencies': []
    }
    
    try:
        with uproot.open(file_path) as f:
            tree = f['clusters/clusters_tree_X']
            
            # Get branches we need
            event_ids = tree['event'].array(library='np')
            is_main = tree['is_main_cluster'].array(library='np')
            total_energy = tree['total_energy'].array(library='np')
            true_label = tree['true_label'].array(library='np')
            
            # Group by event
            unique_events = np.unique(event_ids)
            stats['total_events'] = len(unique_events)
            
            for event_id in unique_events:
                event_mask = event_ids == event_id
                event_is_main = is_main[event_mask]
                event_energies = total_energy[event_mask]
                event_labels = true_label[event_mask]
                
                # Filter only marley clusters (same as make_clusters.cpp logic)
                marley_mask = event_labels == 'marley'
                marley_is_main = event_is_main[marley_mask]
                marley_energies = event_energies[marley_mask]
                
                if len(marley_energies) == 0:
                    # No marley clusters in this event
                    stats['no_main'] += 1
                    if verbose:
                        stats['inconsistencies'].append(f"Event {event_id}: No marley clusters")
                    continue
                
                # Find clusters marked as main (among marley clusters)
                main_indices = np.where(marley_is_main)[0]
                
                if len(main_indices) == 0:
                    stats['no_main'] += 1
                    if verbose:
                        stats['inconsistencies'].append(f"Event {event_id}: No main cluster (has {len(marley_energies)} marley clusters)")
                    continue
                
                if len(main_indices) > 1:
                    stats['multiple_main'] += 1
                    if verbose:
                        stats['inconsistencies'].append(
                            f"Event {event_id}: Multiple main clusters ({len(main_indices)})"
                        )
                    continue
                
                # Get the main cluster's energy
                main_idx = main_indices[0]
                main_energy = marley_energies[main_idx]
                
                # Find the cluster with highest energy (among marley clusters)
                max_energy_idx = np.argmax(marley_energies)
                max_energy = marley_energies[max_energy_idx]
                
                # Check if they match
                if main_idx == max_energy_idx:
                    stats['correct'] += 1
                else:
                    stats['incorrect'] += 1
                    if verbose:
                        stats['inconsistencies'].append(
                            f"Event {event_id}: Main cluster has energy {main_energy:.2f}, "
                            f"but highest energy is {max_energy:.2f} "
                            f"(cluster {max_energy_idx}, marked main: {main_idx})"
                        )
    
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return None
    
    return stats

def main():
    parser = argparse.ArgumentParser(description='Verify main cluster selection correctness')
    parser.add_argument('--sample', type=str, required=True, choices=['es', 'cc'],
                        help='Sample type: es or cc')
    parser.add_argument('--max-files', type=int, default=10,
                        help='Maximum number of files to check')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Print detailed inconsistencies')
    args = parser.parse_args()
    
    # Define paths
    base_path = Path('/eos/home-e/evilla/dune/sn-tps')
    if args.sample == 'es':
        sample_dir = base_path / 'prod_es' / 'es_production_matched_clusters_tick3_ch2_min2_tot3_e2p0'
    else:
        sample_dir = base_path / 'prod_cc' / 'cc_production_matched_clusters_tick3_ch2_min2_tot3_e2p0'
    
    if not sample_dir.exists():
        print(f"ERROR: Directory not found: {sample_dir}")
        sys.exit(1)
    
    # Get files
    files = sorted(sample_dir.glob('*.root'))
    if not files:
        print(f"ERROR: No ROOT files found in {sample_dir}")
        sys.exit(1)
    
    files = files[:args.max_files]
    print(f"\n{'='*80}")
    print(f"MAIN CLUSTER VERIFICATION - {args.sample.upper()} SAMPLE")
    print(f"{'='*80}")
    print(f"Directory: {sample_dir}")
    print(f"Checking {len(files)} files...\n")
    
    # Aggregate statistics
    total_stats = {
        'total_events': 0,
        'correct': 0,
        'incorrect': 0,
        'no_main': 0,
        'multiple_main': 0,
        'inconsistencies': []
    }
    
    files_processed = 0
    files_failed = 0
    
    for i, file_path in enumerate(files, 1):
        print(f"[{i}/{len(files)}] Checking {file_path.name}...", end=' ')
        sys.stdout.flush()
        
        stats = check_main_cluster_in_file(file_path, verbose=args.verbose)
        
        if stats is None:
            print("FAILED")
            files_failed += 1
            continue
        
        files_processed += 1
        
        # Aggregate
        for key in ['total_events', 'correct', 'incorrect', 'no_main', 'multiple_main']:
            total_stats[key] += stats[key]
        
        if args.verbose and stats['inconsistencies']:
            total_stats['inconsistencies'].extend(stats['inconsistencies'])
        
        # Print file summary
        if stats['incorrect'] > 0 or stats['no_main'] > 0 or stats['multiple_main'] > 0:
            print(f"⚠️  {stats['correct']}/{stats['total_events']} correct")
        else:
            print(f"✓ {stats['correct']}/{stats['total_events']} correct")
    
    # Print summary
    print(f"\n{'='*80}")
    print("SUMMARY")
    print(f"{'='*80}")
    print(f"Files processed: {files_processed}/{len(files)}")
    print(f"Files failed:    {files_failed}/{len(files)}")
    print(f"\nTotal events:    {total_stats['total_events']}")
    print(f"Correct:         {total_stats['correct']} ({100*total_stats['correct']/total_stats['total_events']:.2f}%)")
    print(f"Incorrect:       {total_stats['incorrect']} ({100*total_stats['incorrect']/total_stats['total_events']:.2f}%)")
    print(f"No main:         {total_stats['no_main']} ({100*total_stats['no_main']/total_stats['total_events']:.2f}%)")
    print(f"Multiple main:   {total_stats['multiple_main']} ({100*total_stats['multiple_main']/total_stats['total_events']:.2f}%)")
    
    if args.verbose and total_stats['inconsistencies']:
        print(f"\n{'='*80}")
        print("INCONSISTENCIES (first 20)")
        print(f"{'='*80}")
        for inc in total_stats['inconsistencies'][:20]:
            print(f"  {inc}")
        if len(total_stats['inconsistencies']) > 20:
            print(f"  ... and {len(total_stats['inconsistencies']) - 20} more")
    
    print(f"{'='*80}\n")
    
    # Exit with error code if inconsistencies found
    if total_stats['incorrect'] > 0 or total_stats['no_main'] > 0 or total_stats['multiple_main'] > 0:
        print("❌ VERIFICATION FAILED: Inconsistencies detected!")
        sys.exit(1)
    else:
        print("✅ VERIFICATION PASSED: All main clusters correctly assigned!")
        sys.exit(0)

if __name__ == '__main__':
    main()
