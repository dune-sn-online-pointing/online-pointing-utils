#!/usr/bin/env python3
"""
Test fine-grained time thresholds for cluster matching in the 100-500 tick range.

This script runs match_clusters with different time tolerances and analyzes
the trade-off between match efficiency and fake match rate.
"""

import subprocess
import json
import uproot
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import tempfile
import shutil

# Test thresholds (in ticks) - focused on the range you suggested
TIME_THRESHOLDS = [100, 150, 200, 250, 300, 350, 400, 450, 500]

REPO_ROOT = Path(__file__).parent.parent
BUILD_DIR = REPO_ROOT / "build"
MATCH_CLUSTERS_EXE = BUILD_DIR / "src/app/match_clusters"
CONFIG_TEMPLATE = REPO_ROOT / "json/match_clusters/es_valid.json"
OUTPUT_DIR = REPO_ROOT / "output/fine_threshold_tests"
OUTPUT_DIR.mkdir(exist_ok=True)

def read_config_template():
    """Read the base configuration"""
    with open(CONFIG_TEMPLATE, 'r') as f:
        return json.load(f)

def run_matching_with_threshold(threshold_ticks, max_files=5):
    """
    Run match_clusters with a specific time threshold.
    Returns path to output matched clusters folder.
    """
    print(f"\n{'='*70}")
    print(f"Testing threshold: {threshold_ticks} ticks ({threshold_ticks * 0.512:.1f} µs)")
    print(f"{'='*70}")
    
    # Load base config
    config = read_config_template()
    
    # Modify time tolerance
    config['time_tolerance_ticks'] = threshold_ticks
    
    # Create unique output folder for this threshold
    base_matched_folder = Path(config['matched_clusters_folder'])
    threshold_folder = base_matched_folder.parent / f"matched_clusters_es_valid_tick3_ch2_min2_tot1_e0p0_t{threshold_ticks}"
    config['matched_clusters_folder'] = str(threshold_folder)
    
    # Write temporary config
    temp_config = OUTPUT_DIR / f"config_t{threshold_ticks}.json"
    with open(temp_config, 'w') as f:
        json.dump(config, f, indent=2)
    
    print(f"Config: {temp_config}")
    print(f"Output: {threshold_folder}")
    
    # Run match_clusters
    cmd = [
        str(MATCH_CLUSTERS_EXE),
        "-j",
        str(temp_config)
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        if result.returncode != 0:
            print(f"ERROR: match_clusters failed with code {result.returncode}")
            print(f"STDERR: {result.stderr}")
            return None
        print(f"Success! Output in {threshold_folder}")
        return threshold_folder
    except subprocess.TimeoutExpired:
        print(f"ERROR: Timeout after 300s")
        return None
    except Exception as e:
        print(f"ERROR: {e}")
        return None

def analyze_matched_output(matched_folder, threshold_ticks):
    """
    Analyze the matched clusters output for a given threshold.
    Returns dict with statistics.
    """
    if not matched_folder or not matched_folder.exists():
        return None
    
    stats = {
        'threshold_ticks': threshold_ticks,
        'threshold_us': threshold_ticks * 0.512,
    }
    
    # Find all matched files
    matched_files = list(matched_folder.glob("*_matched.root"))
    stats['n_files'] = len(matched_files)
    
    if len(matched_files) == 0:
        print(f"  No matched files found in {matched_folder}")
        return stats
    
    # Analyze each view
    for view in ['U', 'V', 'X']:
        view_total = 0
        view_matched = 0
        view_triple_matched = 0
        view_marley_count = 0
        view_marley_matched = 0
        
        for f in matched_files:
            try:
                with uproot.open(f) as root_file:
                    tree_path = f"clusters/clusters_tree_{view}"
                    if tree_path not in root_file:
                        continue
                    
                    tree = root_file[tree_path]
                    
                    # Read branches
                    match_id = tree['match_id'].array(library='np')
                    match_type = tree['match_type'].array(library='np')
                    marley_frac = tree['marley_tp_fraction'].array(library='np') if 'marley_tp_fraction' in tree.keys() else None
                    
                    view_total += len(match_id)
                    view_matched += np.sum(match_id >= 0)
                    view_triple_matched += np.sum(match_type == 3)
                    
                    if marley_frac is not None:
                        # Consider cluster as MARLEY if >50% of TPs are from MARLEY
                        marley_mask = marley_frac > 0.5
                        view_marley_count += np.sum(marley_mask)
                        view_marley_matched += np.sum((match_id >= 0) & marley_mask)
                        
            except Exception as e:
                print(f"  Error reading {f.name}: {e}")
                continue
        
        stats[f'{view}_total'] = view_total
        stats[f'{view}_matched'] = view_matched
        stats[f'{view}_triple_matched'] = view_triple_matched
        stats[f'{view}_marley_total'] = view_marley_count
        stats[f'{view}_marley_matched'] = view_marley_matched
        stats[f'{view}_efficiency'] = (view_matched / view_total * 100) if view_total > 0 else 0
        stats[f'{view}_marley_efficiency'] = (view_marley_matched / view_marley_count * 100) if view_marley_count > 0 else 0
    
    # Overall statistics
    total_clusters = stats['U_total'] + stats['V_total'] + stats['X_total']
    total_matched = stats['U_matched'] + stats['V_matched'] + stats['X_matched']
    total_marley = stats['U_marley_total'] + stats['V_marley_total'] + stats['X_marley_total']
    total_marley_matched = stats['U_marley_matched'] + stats['V_marley_matched'] + stats['X_marley_matched']
    
    stats['total_clusters'] = total_clusters
    stats['total_matched'] = total_matched
    stats['overall_efficiency'] = (total_matched / total_clusters * 100) if total_clusters > 0 else 0
    stats['marley_efficiency'] = (total_marley_matched / total_marley * 100) if total_marley > 0 else 0
    
    print(f"  Total: {total_clusters} clusters, {total_matched} matched ({stats['overall_efficiency']:.1f}%)")
    print(f"  MARLEY: {total_marley} clusters, {total_marley_matched} matched ({stats['marley_efficiency']:.1f}%)")
    
    return stats

def plot_results(all_stats):
    """Create visualization of threshold optimization results"""
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(14, 10))
    
    thresholds = [s['threshold_ticks'] for s in all_stats]
    
    # Plot 1: Total matches vs threshold
    total_matched = [s['total_matched'] for s in all_stats]
    ax1.plot(thresholds, total_matched, 'o-', linewidth=2, markersize=8)
    ax1.set_xlabel('Time Tolerance (ticks)', fontsize=12)
    ax1.set_ylabel('Total Matched Clusters', fontsize=12)
    ax1.set_title('Total Matches vs Time Tolerance', fontsize=14, fontweight='bold')
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: Efficiency by view
    for view in ['U', 'V', 'X']:
        eff = [s[f'{view}_efficiency'] for s in all_stats]
        ax2.plot(thresholds, eff, 'o-', label=f'{view} View', linewidth=2, markersize=8)
    ax2.set_xlabel('Time Tolerance (ticks)', fontsize=12)
    ax2.set_ylabel('Match Efficiency (%)', fontsize=12)
    ax2.set_title('Match Efficiency by View', fontsize=14, fontweight='bold')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    # Plot 3: MARLEY efficiency
    marley_eff = [s['marley_efficiency'] for s in all_stats]
    ax3.plot(thresholds, marley_eff, 'o-', color='purple', linewidth=2, markersize=8)
    ax3.set_xlabel('Time Tolerance (ticks)', fontsize=12)
    ax3.set_ylabel('MARLEY Match Efficiency (%)', fontsize=12)
    ax3.set_title('MARLEY Signal Efficiency', fontsize=14, fontweight='bold')
    ax3.grid(True, alpha=0.3)
    
    # Plot 4: Triple matches (all 3 views)
    triple_matches = [s['U_triple_matched'] + s['V_triple_matched'] + s['X_triple_matched'] for s in all_stats]
    ax4.plot(thresholds, triple_matches, 'o-', color='green', linewidth=2, markersize=8)
    ax4.set_xlabel('Time Tolerance (ticks)', fontsize=12)
    ax4.set_ylabel('Triple-Plane Matches', fontsize=12)
    ax4.set_title('3-Plane Matches vs Time Tolerance', fontsize=14, fontweight='bold')
    ax4.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    output_plot = OUTPUT_DIR / "fine_threshold_optimization.png"
    plt.savefig(output_plot, dpi=150, bbox_inches='tight')
    print(f"\nPlot saved: {output_plot}")
    
    return output_plot

def main():
    """Run threshold optimization test"""
    print("="*70)
    print("FINE-GRAINED TIME THRESHOLD OPTIMIZATION")
    print("Testing range: 100-500 ticks (51.2-256 µs)")
    print("="*70)
    
    # Check if build exists
    if not MATCH_CLUSTERS_EXE.exists():
        print(f"ERROR: match_clusters executable not found at {MATCH_CLUSTERS_EXE}")
        print("Please run 'make' from the build directory first.")
        return 1
    
    all_stats = []
    
    for threshold in TIME_THRESHOLDS:
        matched_folder = run_matching_with_threshold(threshold, max_files=5)
        stats = analyze_matched_output(matched_folder, threshold)
        if stats:
            all_stats.append(stats)
    
    # Save results
    results_file = OUTPUT_DIR / "fine_threshold_results.json"
    with open(results_file, 'w') as f:
        json.dump(all_stats, f, indent=2)
    print(f"\nResults saved: {results_file}")
    
    # Create plots
    if all_stats:
        plot_results(all_stats)
    
    # Print summary
    print("\n" + "="*70)
    print("SUMMARY")
    print("="*70)
    for s in all_stats:
        print(f"\nThreshold: {s['threshold_ticks']:4d} ticks ({s['threshold_us']:6.1f} µs)")
        print(f"  Total matches: {s['total_matched']:3d} / {s['total_clusters']:3d} ({s['overall_efficiency']:5.1f}%)")
        print(f"  MARLEY efficiency: {s['marley_efficiency']:5.1f}%")
        print(f"  By view: U={s['U_efficiency']:.1f}%, V={s['V_efficiency']:.1f}%, X={s['X_efficiency']:.1f}%")
    
    return 0

if __name__ == "__main__":
    exit(main())
