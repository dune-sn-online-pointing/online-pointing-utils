#!/usr/bin/env python3
"""
Test different time thresholds for cluster matching to find the optimal value.

This script modifies match_clusters.cpp with different time windows, recompiles,
runs matching, and compares the results.
"""

import subprocess
import json
import uproot
import numpy as np
import sys
from pathlib import Path

# Test thresholds (in ticks)
TIME_THRESHOLDS = [5000, 2000, 1000, 500, 250, 100, 50]

REPO_ROOT = Path(__file__).parent.parent
MATCH_CLUSTERS_CPP = REPO_ROOT / "src/app/match_clusters.cpp"
CONFIG_FILE = REPO_ROOT / "output/test_es_valid_match_config.json"

# Backup original file
BACKUP_FILE = MATCH_CLUSTERS_CPP.with_suffix('.cpp.backup')


def backup_original():
    """Backup original match_clusters.cpp"""
    if not BACKUP_FILE.exists():
        print(f"Creating backup: {BACKUP_FILE}")
        with open(MATCH_CLUSTERS_CPP, 'r') as f:
            content = f.read()
        with open(BACKUP_FILE, 'w') as f:
            f.write(content)


def restore_original():
    """Restore original match_clusters.cpp"""
    print(f"Restoring original from backup...")
    with open(BACKUP_FILE, 'r') as f:
        content = f.read()
    with open(MATCH_CLUSTERS_CPP, 'w') as f:
        f.write(content)


def modify_time_threshold(threshold_ticks):
    """Modify match_clusters.cpp to use the specified time threshold"""
    print(f"\n{'='*70}")
    print(f"Setting time threshold to ±{threshold_ticks} ticks")
    print(f"{'='*70}")
    
    with open(MATCH_CLUSTERS_CPP, 'r') as f:
        content = f.read()
    
    # Replace all occurrences of the time window (currently 5000)
    # Lines to modify: ~107, ~112, ~133, ~138
    modified = content.replace(
        'GetTimeStart() + 5000',
        f'GetTimeStart() + {threshold_ticks}'
    ).replace(
        'GetTimeStart() - 5000',
        f'GetTimeStart() - {threshold_ticks}'
    )
    
    with open(MATCH_CLUSTERS_CPP, 'w') as f:
        f.write(modified)


def compile_code():
    """Compile the modified code"""
    print("Compiling...")
    result = subprocess.run(
        ['./scripts/compile.sh'],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True
    )
    
    if result.returncode != 0:
        print("COMPILATION FAILED!")
        print(result.stderr)
        return False
    
    return True


def run_matching():
    """Run match_clusters with current configuration"""
    print("Running match_clusters...")
    result = subprocess.run(
        ['./build/src/app/match_clusters', '-j', str(CONFIG_FILE)],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True
    )
    
    if result.returncode != 0:
        print("MATCHING FAILED!")
        print(result.stderr)
        return None
    
    # Extract statistics from output
    output = result.stdout
    lines = output.split('\n')
    
    stats = {}
    for line in lines:
        if 'Number of compatible clusters:' in line:
            # Extract number
            parts = line.split(':')
            if len(parts) > 1:
                try:
                    stats['total_matches'] = int(parts[1].strip())
                except:
                    pass
        if 'Match mappings created:' in line:
            # Extract U=X, V=Y, X=Z
            parts = line.split(':')
            if len(parts) > 1:
                mapping_str = parts[1].strip()
                for item in mapping_str.split(','):
                    if '=' in item:
                        key, val = item.split('=')
                        key = key.strip()
                        try:
                            stats[f'unique_{key}'] = int(val.strip())
                        except:
                            pass
        if 'Time:' in line:
            parts = line.split(':')
            if len(parts) > 1:
                try:
                    time_str = parts[1].strip().replace('ms', '').strip()
                    stats['time_ms'] = float(time_str)
                except:
                    pass
    
    return stats


def analyze_output_file():
    """Analyze the matched_clusters output file"""
    # Read config to get output file path
    with open(CONFIG_FILE, 'r') as f:
        config = json.load(f)
    
    output_file = config['output_file']
    
    try:
        file = uproot.open(output_file)
    except:
        print(f"Could not open output file: {output_file}")
        return {}
    
    stats = {}
    
    for plane in ['U', 'V', 'X']:
        tree = file[f'clusters/clusters_tree_{plane}']
        match_ids = tree['match_id'].array(library='np')
        match_types = tree['match_type'].array(library='np')
        marley_fractions = tree['marley_tp_fraction'].array(library='np')
        
        matched_mask = match_ids >= 0
        n_matched = np.sum(matched_mask)
        n_total = len(match_ids)
        
        stats[f'{plane}_total'] = n_total
        stats[f'{plane}_matched'] = n_matched
        stats[f'{plane}_unmatched'] = n_total - n_matched
        
        if n_matched > 0:
            # MARLEY purity
            marley_matched = marley_fractions[matched_mask]
            stats[f'{plane}_marley_purity'] = np.mean(marley_matched > 0.5)
            stats[f'{plane}_avg_marley_frac'] = np.mean(marley_matched)
            
            # Number of unique matches
            unique_match_ids = len(set(match_ids[matched_mask]))
            stats[f'{plane}_unique_matches'] = unique_match_ids
    
    file.close()
    return stats


def main():
    print("="*70)
    print("Time Threshold Optimization Test")
    print("="*70)
    
    # Backup original
    backup_original()
    
    results = []
    
    try:
        for threshold in TIME_THRESHOLDS:
            # Modify code
            modify_time_threshold(threshold)
            
            # Compile
            if not compile_code():
                print(f"Failed to compile with threshold {threshold}, skipping...")
                continue
            
            # Run matching
            run_stats = run_matching()
            if run_stats is None:
                print(f"Failed to run matching with threshold {threshold}, skipping...")
                continue
            
            # Analyze output
            analysis_stats = analyze_output_file()
            
            # Combine stats
            combined_stats = {
                'threshold_ticks': threshold,
                'threshold_us': threshold * 0.512,
                **run_stats,
                **analysis_stats
            }
            
            results.append(combined_stats)
            
            # Print summary
            print(f"\n--- Results for ±{threshold} ticks ({threshold * 0.512:.1f} µs) ---")
            print(f"  Total geometric matches: {run_stats.get('total_matches', 'N/A')}")
            print(f"  Unique X matched: {analysis_stats.get('X_matched', 'N/A')}")
            print(f"  Unique U matched: {analysis_stats.get('U_matched', 'N/A')}")
            print(f"  Unique V matched: {analysis_stats.get('V_matched', 'N/A')}")
            print(f"  X MARLEY purity: {analysis_stats.get('X_marley_purity', 0):.3f}")
            print(f"  Processing time: {run_stats.get('time_ms', 'N/A'):.1f} ms")
    
    finally:
        # Always restore original
        restore_original()
        print("\n" + "="*70)
        print("Restored original match_clusters.cpp")
        print("="*70)
    
    # Print comparison table
    print("\n" + "="*70)
    print("COMPARISON TABLE")
    print("="*70)
    print(f"{'Threshold':>12} {'(µs)':>8} {'Matches':>8} {'X_match':>8} {'U_match':>8} {'V_match':>8} {'MARLEY':>8} {'Time(ms)':>9}")
    print("-"*70)
    
    for r in results:
        threshold = r['threshold_ticks']
        time_us = r['threshold_us']
        total = r.get('total_matches', 0)
        x_match = r.get('X_matched', 0)
        u_match = r.get('U_matched', 0)
        v_match = r.get('V_matched', 0)
        purity = r.get('X_marley_purity', 0)
        proc_time = r.get('time_ms', 0)
        
        print(f"±{threshold:>5} ticks {time_us:>7.1f} {total:>8} {x_match:>8} {u_match:>8} {v_match:>8} {purity:>7.1%} {proc_time:>9.1f}")
    
    # Find sweet spot
    print("\n" + "="*70)
    print("ANALYSIS")
    print("="*70)
    
    # Look for threshold where we get good matches without too many ambiguities
    if results:
        max_marley_purity = max(r.get('X_marley_purity', 0) for r in results)
        
        # Filter results with 100% MARLEY purity
        pure_results = [r for r in results if r.get('X_marley_purity', 0) >= 0.999]
        
        if pure_results:
            # Among pure results, find the one with most X matches (least restrictive while maintaining purity)
            best = max(pure_results, key=lambda r: r.get('X_matched', 0))
            
            print(f"\nRecommended threshold: ±{best['threshold_ticks']} ticks ({best['threshold_us']:.1f} µs)")
            print(f"  - Maintains 100% MARLEY purity")
            print(f"  - Matches {best.get('X_matched', 0)} X-clusters")
            print(f"  - Total geometric matches: {best.get('total_matches', 0)}")
            print(f"  - Ratio (X_matched/total): {best.get('X_matched', 0) / max(best.get('total_matches', 1), 1):.2f}")
            
            # Compare to loosest threshold
            if len(results) > 1:
                loosest = results[0]
                if best['threshold_ticks'] != loosest['threshold_ticks']:
                    efficiency_loss = (loosest.get('X_matched', 0) - best.get('X_matched', 0)) / max(loosest.get('X_matched', 1), 1)
                    ambiguity_reduction = (loosest.get('total_matches', 0) - best.get('total_matches', 0)) / max(loosest.get('total_matches', 1), 1)
                    
                    print(f"\nVs. loosest threshold (±{loosest['threshold_ticks']} ticks):")
                    print(f"  - Efficiency loss: {efficiency_loss:.1%}")
                    print(f"  - Ambiguity reduction: {ambiguity_reduction:.1%}")
        else:
            print("\nWarning: No threshold maintains 100% MARLEY purity!")
            print("Consider using a looser threshold or checking the matching algorithm.")
    
    # Save detailed results to JSON
    output_json = REPO_ROOT / "output" / "time_threshold_test_results.json"
    with open(output_json, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\nDetailed results saved to: {output_json}")
    
    # Final recompile with original
    print("\nRecompiling original code...")
    compile_code()
    print("\nDone!")


if __name__ == '__main__':
    main()
