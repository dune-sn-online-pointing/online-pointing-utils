#!/usr/bin/env python3
"""
Test time thresholds in the relevant range (25-5000 ticks) to find optimal value.
"""

import subprocess
import json
import uproot
import numpy as np
from pathlib import Path

# Test thresholds focusing on the relevant range
TIME_THRESHOLDS = [5000, 3000, 2000, 1000, 500, 250, 100, 50, 40, 35, 32, 30, 25]

REPO_ROOT = Path(__file__).parent.parent
MATCH_CLUSTERS_CPP = REPO_ROOT / "src/app/match_clusters.cpp"
CONFIG_FILE = REPO_ROOT / "output/test_es_valid_match_config.json"

def modify_and_compile(threshold_ticks):
    """Modify code with threshold and compile"""
    print(f"\nTesting ±{threshold_ticks} ticks ({threshold_ticks * 0.512:.1f} µs)...", end=" ")
    
    # Read original
    with open(MATCH_CLUSTERS_CPP, 'r') as f:
        content = f.read()
    
    # Modify
    modified = content.replace(
        'GetTimeStart() + 5000',
        f'GetTimeStart() + {threshold_ticks}'
    ).replace(
        'GetTimeStart() - 5000',
        f'GetTimeStart() - {threshold_ticks}'
    )
    
    # Write
    with open(MATCH_CLUSTERS_CPP, 'w') as f:
        f.write(modified)
    
    # Compile
    result = subprocess.run(
        ['./scripts/compile.sh'],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True
    )
    
    if result.returncode != 0:
        print("COMPILE FAILED!")
        return False
    
    return True

def run_and_analyze():
    """Run matching and analyze results"""
    # Run
    result = subprocess.run(
        ['./build/src/app/match_clusters', '-j', str(CONFIG_FILE)],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True
    )
    
    if result.returncode != 0:
        print("RUN FAILED!")
        return None
    
    # Parse output for total matches
    total_matches = 0
    for line in result.stdout.split('\n'):
        if 'Number of compatible clusters:' in line:
            parts = line.split(':')
            if len(parts) > 1:
                try:
                    total_matches = int(parts[1].strip())
                except:
                    pass
    
    # Analyze output file
    with open(CONFIG_FILE, 'r') as f:
        config = json.load(f)
    
    try:
        file = uproot.open(config['output_file'])
        
        stats = {}
        for plane in ['U', 'V', 'X']:
            tree = file[f'clusters/clusters_tree_{plane}']
            match_ids = tree['match_id'].array(library='np')
            marley_fracs = tree['marley_tp_fraction'].array(library='np')
            
            matched_mask = match_ids >= 0
            n_matched = int(np.sum(matched_mask))
            
            stats[f'{plane}_matched'] = n_matched
            
            if n_matched > 0:
                marley_purity = float(np.mean(marley_fracs[matched_mask] > 0.5))
                stats[f'{plane}_marley_purity'] = marley_purity
            else:
                stats[f'{plane}_marley_purity'] = 0.0
        
        file.close()
        
        stats['total_geometric_matches'] = total_matches
        return stats
        
    except Exception as e:
        print(f"ANALYSIS FAILED: {e}")
        return None

def main():
    print("="*70)
    print("Time Threshold Optimization (Focused Range)")
    print("="*70)
    
    # Save original
    with open(MATCH_CLUSTERS_CPP, 'r') as f:
        original_content = f.read()
    
    results = []
    
    try:
        for threshold in TIME_THRESHOLDS:
            if not modify_and_compile(threshold):
                continue
            
            stats = run_and_analyze()
            if stats is None:
                continue
            
            result = {
                'threshold_ticks': threshold,
                'threshold_us': threshold * 0.512,
                **stats
            }
            results.append(result)
            
            # Print result
            print(f"Matches: geo={stats['total_geometric_matches']}, " +
                  f"X={stats['X_matched']}, " +
                  f"U={stats['U_matched']}, " +
                  f"V={stats['V_matched']}, " +
                  f"purity={stats['X_marley_purity']:.1%}")
    
    finally:
        # Restore original
        print("\nRestoring original...")
        with open(MATCH_CLUSTERS_CPP, 'w') as f:
            f.write(original_content)
        
        # Recompile
        subprocess.run(['./scripts/compile.sh'], cwd=REPO_ROOT, capture_output=True)
    
    # Results table
    print("\n" + "="*70)
    print("RESULTS TABLE")
    print("="*70)
    print(f"{'Threshold':>12} {'(µs)':>8} {'Geo':>6} {'X':>4} {'U':>4} {'V':>4} {'Purity':>8} {'Ratio':>6}")
    print("-"*70)
    
    for r in results:
        t = r['threshold_ticks']
        us = r['threshold_us']
        geo = r['total_geometric_matches']
        x = r['X_matched']
        u = r['U_matched']
        v = r['V_matched']
        purity = r['X_marley_purity']
        ratio = x / geo if geo > 0 else 0
        
        print(f"±{t:>5} ticks {us:>7.1f} {geo:>6} {x:>4} {u:>4} {v:>4} {purity:>7.1%} {ratio:>6.2f}")
    
    # Analysis
    print("\n" + "="*70)
    print("RECOMMENDATION")
    print("="*70)
    
    # Find thresholds with 100% purity
    pure_results = [r for r in results if r['X_marley_purity'] >= 0.999 and r['X_matched'] > 0]
    
    if pure_results:
        # Find tightest threshold that still gets good matches
        best = min(pure_results, key=lambda r: r['threshold_ticks'])
        
        print(f"\nOptimal threshold: ±{best['threshold_ticks']} ticks ({best['threshold_us']:.1f} µs)")
        print(f"  Geometric matches: {best['total_geometric_matches']}")
        print(f"  Unique X matched: {best['X_matched']}")
        print(f"  Unique U matched: {best['U_matched']}")
        print(f"  Unique V matched: {best['V_matched']}")
        print(f"  MARLEY purity: {best['X_marley_purity']:.1%}")
        print(f"  Efficiency (X/geo): {best['X_matched']/best['total_geometric_matches']:.1%}")
        
        # Compare to loosest
        loosest = max(pure_results, key=lambda r: r['threshold_ticks'])
        if best['threshold_ticks'] != loosest['threshold_ticks']:
            print(f"\nVs. ±{loosest['threshold_ticks']} ticks:")
            print(f"  Ambiguity reduction: {(loosest['total_geometric_matches'] - best['total_geometric_matches'])/loosest['total_geometric_matches']:.1%}")
            print(f"  Match loss: {(loosest['X_matched'] - best['X_matched'])/loosest['X_matched']:.1%}")
    else:
        print("\nNo threshold maintains 100% MARLEY purity with matches!")
    
    # Save results
    output_json = REPO_ROOT / "output" / "threshold_optimization_results.json"
    with open(output_json, 'w') as f:
        # Convert numpy types to native Python
        clean_results = []
        for r in results:
            clean_r = {}
            for k, v in r.items():
                if isinstance(v, (np.integer, np.floating)):
                    clean_r[k] = v.item()
                else:
                    clean_r[k] = v
            clean_results.append(clean_r)
        json.dump(clean_results, f, indent=2)
    
    print(f"\nResults saved to: {output_json}")

if __name__ == '__main__':
    main()
