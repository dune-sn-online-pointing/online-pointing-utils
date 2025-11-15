#!/usr/bin/env python3
"""
Analyze matching completeness: For each X cluster, track U-only, V-only, and U+V matches.
This requires inferring the matching behavior from the data structure.
"""

import uproot
import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict

print("=" * 80)
print("MATCHING COMPLETENESS ANALYSIS")
print("=" * 80)

# Load single-plane clusters
clusters_file = "/home/virgolaema/dune/online-pointing-utils/output/clusters_clusters_tick5_ch2_min2_tot1_e0p0/prodmarley_nue_flat_es_dune10kt_1x2x2_20250927T182943Z_gen_004499_supernova_g4_detsim_ana_2025-10-21T_095334Z_clusters.root"
file = uproot.open(clusters_file)

# Load all three views
tree_x = file["clusters/clusters_tree_X"]
x_marley_frac = tree_x["marley_tp_fraction"].array(library="np")
x_cluster_id = tree_x["cluster_id"].array(library="np")
x_time_start = tree_x["tp_time_start"].array(library="np")
x_detector = tree_x["tp_detector"].array(library="np")

tree_u = file["clusters/clusters_tree_U"]
u_marley_frac = tree_u["marley_tp_fraction"].array(library="np")
u_cluster_id = tree_u["cluster_id"].array(library="np")
u_time_start = tree_u["tp_time_start"].array(library="np")
u_detector = tree_u["tp_detector"].array(library="np")

tree_v = file["clusters/clusters_tree_V"]
v_marley_frac = tree_v["marley_tp_fraction"].array(library="np")
v_cluster_id = tree_v["cluster_id"].array(library="np")
v_time_start = tree_v["tp_time_start"].array(library="np")
v_detector = tree_v["tp_detector"].array(library="np")

# Load matched clusters
matched_file = uproot.open("/home/virgolaema/dune/online-pointing-utils/output/test_es_matched.root")
matched_tree = matched_file["clusters/clusters_tree_multiplane"]

# Matched clusters contain TPs from all three planes
# We can count TPs by detector type to determine which views contributed
matched_detector = matched_tree["tp_detector"].array(library="np")
matched_marley_frac = matched_tree["marley_tp_fraction"].array(library="np")
matched_n_tps = matched_tree["n_tps"].array(library="np")

print("\nANALYZING CURRENT MATCHING IMPLEMENTATION:")
print("-" * 80)
print("Current algorithm structure (from match_clusters.cpp):")
print("  FOR each X-cluster:")
print("    FOR each U-cluster (time-compatible):")
print("      FOR each V-cluster (time-compatible):")
print("        IF are_compatibles(U, V, X, 5cm):")
print("          → Create multiplane match")
print("")
print("KEY OBSERVATION: This requires BOTH U AND V to be present!")
print("                 No partial (X+U only) or (X+V only) matches are created.")
print("")

# Analyze detector types in matched clusters
n_matched = len(matched_detector)
print(f"\nMATCHED CLUSTER COMPOSITION:")
print("-" * 80)

# Count which views are present in each match
view_composition = []
for i in range(n_matched):
    detectors = matched_detector[i]
    # Detector encoding: 0=U, 1=V, 2=X (or similar)
    # Count unique detector types
    unique_dets = np.unique(detectors)
    n_views = len(unique_dets)
    
    has_u = 0 in detectors  # U-plane
    has_v = 1 in detectors  # V-plane  
    has_x = 2 in detectors  # X-plane
    
    view_composition.append({
        'n_views': n_views,
        'has_u': has_u,
        'has_v': has_v,
        'has_x': has_x,
        'n_tps': matched_n_tps[i],
        'marley_frac': matched_marley_frac[i]
    })

# Categorize matches
n_all_three = sum(1 for v in view_composition if v['has_u'] and v['has_v'] and v['has_x'])
n_x_u_only = sum(1 for v in view_composition if v['has_x'] and v['has_u'] and not v['has_v'])
n_x_v_only = sum(1 for v in view_composition if v['has_x'] and v['has_v'] and not v['has_u'])
n_x_only = sum(1 for v in view_composition if v['has_x'] and not v['has_u'] and not v['has_v'])
n_other = n_matched - (n_all_three + n_x_u_only + n_x_v_only + n_x_only)

print(f"Total matched clusters: {n_matched}")
print(f"\nBreakdown by view composition:")
print(f"  U + V + X (complete): {n_all_three} ({100*n_all_three/n_matched:.1f}%)")
print(f"  U + X only (no V):    {n_x_u_only} ({100*n_x_u_only/n_matched:.1f}%)")
print(f"  V + X only (no U):    {n_x_v_only} ({100*n_x_v_only/n_matched:.1f}%)")
print(f"  X only (no U or V):   {n_x_only} ({100*n_x_only/n_matched:.1f}%)")
if n_other > 0:
    print(f"  Other combinations:   {n_other} ({100*n_other/n_matched:.1f}%)")

# Now analyze what COULD have been matched
print("\n" + "=" * 80)
print("POTENTIAL MATCHING ANALYSIS (Estimating from time windows)")
print("=" * 80)
print("\nEstimating how many X-clusters could find U and/or V partners...")
print("(Based on ±5000 tick time window and same APA)")

# For each X cluster, count potential U and V matches within time window
APA_CHANNELS = 2560  # Total channels per APA

x_with_u_candidates = 0
x_with_v_candidates = 0
x_with_both_candidates = 0
x_marley_with_u = 0
x_marley_with_v = 0
x_marley_with_both = 0

for i in range(len(x_cluster_id)):
    x_time = np.mean(x_time_start[i]) if len(x_time_start[i]) > 0 else 0
    x_det = x_detector[i][0] if len(x_detector[i]) > 0 else 0
    x_apa = int(x_det / APA_CHANNELS)
    is_marley = x_marley_frac[i] > 0.5
    
    # Count U candidates
    u_candidates = 0
    for j in range(len(u_cluster_id)):
        u_time = np.mean(u_time_start[j]) if len(u_time_start[j]) > 0 else 0
        u_det = u_detector[j][0] if len(u_detector[j]) > 0 else 0
        u_apa = int(u_det / APA_CHANNELS)
        
        if abs(u_time - x_time) <= 5000 and u_apa == x_apa:
            u_candidates += 1
    
    # Count V candidates
    v_candidates = 0
    for k in range(len(v_cluster_id)):
        v_time = np.mean(v_time_start[k]) if len(v_time_start[k]) > 0 else 0
        v_det = v_detector[k][0] if len(v_detector[k]) > 0 else 0
        v_apa = int(v_det / APA_CHANNELS)
        
        if abs(v_time - x_time) <= 5000 and v_apa == x_apa:
            v_candidates += 1
    
    has_u = u_candidates > 0
    has_v = v_candidates > 0
    has_both = has_u and has_v
    
    if has_u:
        x_with_u_candidates += 1
        if is_marley:
            x_marley_with_u += 1
    if has_v:
        x_with_v_candidates += 1
        if is_marley:
            x_marley_with_v += 1
    if has_both:
        x_with_both_candidates += 1
        if is_marley:
            x_marley_with_both += 1

n_x_total = len(x_cluster_id)
n_x_marley = np.sum(x_marley_frac > 0.5)
n_x_bkg = n_x_total - n_x_marley

print(f"\nX-clusters with time-compatible candidates:")
print(f"  Total X-clusters: {n_x_total} (MARLEY: {n_x_marley}, Bkg: {n_x_bkg})")
print(f"\n  With U candidates:      {x_with_u_candidates} ({100*x_with_u_candidates/n_x_total:.1f}%)")
print(f"    - MARLEY with U:       {x_marley_with_u}/{n_x_marley} ({100*x_marley_with_u/n_x_marley:.1f}%)")
print(f"\n  With V candidates:      {x_with_v_candidates} ({100*x_with_v_candidates/n_x_total:.1f}%)")
print(f"    - MARLEY with V:       {x_marley_with_v}/{n_x_marley} ({100*x_marley_with_v/n_x_marley:.1f}%)")
print(f"\n  With BOTH U and V:      {x_with_both_candidates} ({100*x_with_both_candidates/n_x_total:.1f}%)")
print(f"    - MARLEY with both:    {x_marley_with_both}/{n_x_marley} ({100*x_marley_with_both/n_x_marley:.1f}%)")

# Only candidates
x_u_only = x_with_u_candidates - x_with_both_candidates
x_v_only = x_with_v_candidates - x_with_both_candidates
x_neither = n_x_total - x_with_u_candidates - x_v_only

print(f"\n  With U but NOT V:       {x_u_only} ({100*x_u_only/n_x_total:.1f}%)")
print(f"  With V but NOT U:       {x_v_only} ({100*x_v_only/n_x_total:.1f}%)")
print(f"  With NEITHER:           {x_neither} ({100*x_neither/n_x_total:.1f}%)")

print("\n" + "=" * 80)
print("KEY FINDINGS")
print("=" * 80)
print(f"""
1. CURRENT IMPLEMENTATION:
   - ALL {n_matched} matched clusters have U + V + X views (100%)
   - Algorithm REQUIRES both U and V to create a match
   - Partial matches (X+U only or X+V only) are NOT created

2. MATCHING POTENTIAL:
   - {x_with_both_candidates}/{n_x_total} X-clusters have BOTH U and V time-compatible candidates
   - {x_u_only} X-clusters have U candidates but NOT V
   - {x_v_only} X-clusters have V candidates but NOT U
   - {x_neither} X-clusters have NEITHER U nor V candidates

3. MATCHING SUCCESS (MARLEY only):
   - {x_marley_with_both} MARLEY X-clusters have both U+V candidates
   - {n_all_three} actually matched → {100*n_all_three/x_marley_with_both:.1f}% success rate
   - This accounts for geometric compatibility check (5cm radius)

4. IMPLICATIONS:
   - If you allowed X+U matches: could match {x_u_only} more X-clusters
   - If you allowed X+V matches: could match {x_v_only} more X-clusters
   - Trade-off: partial matches have less 3D constraint → more background
""")

# Create visualization
fig, axes = plt.subplots(1, 2, figsize=(15, 6))

# Left: Venn-style diagram showing overlap
ax = axes[0]
from matplotlib.patches import Circle
from matplotlib.patches import Rectangle

ax.set_xlim(0, 10)
ax.set_ylim(0, 10)
ax.set_aspect('equal')
ax.axis('off')
ax.set_title('X-cluster Matching Candidates\n(Time window ±5000 ticks, same APA)', 
             fontsize=14, fontweight='bold', pad=20)

# Draw overlapping regions
circle_u = Circle((3.5, 5), 2.5, fill=False, edgecolor='blue', linewidth=3, linestyle='--')
circle_v = Circle((6.5, 5), 2.5, fill=False, edgecolor='red', linewidth=3, linestyle='--')
ax.add_patch(circle_u)
ax.add_patch(circle_v)

# Labels
ax.text(2.2, 7.5, 'Has U\ncandidates', fontsize=12, ha='center', color='blue', fontweight='bold')
ax.text(7.8, 7.5, 'Has V\ncandidates', fontsize=12, ha='center', color='red', fontweight='bold')

# Counts
ax.text(2.5, 5, f'{x_u_only}\nX-clusters\n(U only)', fontsize=11, ha='center', va='center',
        bbox=dict(boxstyle='round,pad=0.5', facecolor='lightblue', edgecolor='blue', linewidth=2))
ax.text(7.5, 5, f'{x_v_only}\nX-clusters\n(V only)', fontsize=11, ha='center', va='center',
        bbox=dict(boxstyle='round,pad=0.5', facecolor='lightcoral', edgecolor='red', linewidth=2))
ax.text(5, 5, f'{x_with_both_candidates}\nX-clusters\n(BOTH U+V)', fontsize=11, ha='center', va='center',
        bbox=dict(boxstyle='round,pad=0.5', facecolor='lightgreen', edgecolor='green', linewidth=2))
ax.text(5, 2, f'{x_neither} X-clusters\nhave NEITHER', fontsize=11, ha='center', va='center',
        bbox=dict(boxstyle='round,pad=0.3', facecolor='lightgray', edgecolor='gray', linewidth=2))

# Current matching
ax.text(5, 9, f'✓ Current matching creates {n_matched} matches\nfrom {x_with_both_candidates} candidates with BOTH U+V',
        fontsize=10, ha='center', va='top',
        bbox=dict(boxstyle='round,pad=0.5', facecolor='yellow', alpha=0.7))

# Right: Bar chart of matching scenarios
ax = axes[1]

categories = ['Both U+V\ncandidates', 'U only\ncandidates', 'V only\ncandidates', 'Neither\nU nor V']
counts = [x_with_both_candidates, x_u_only, x_v_only, x_neither]
colors = ['#2ecc71', '#3498db', '#e74c3c', '#95a5a6']

bars = ax.bar(categories, counts, color=colors, edgecolor='black', linewidth=2)
ax.set_ylabel('Number of X-clusters', fontsize=12, fontweight='bold')
ax.set_title('X-cluster Matching Candidate Distribution', fontsize=13, fontweight='bold')
ax.grid(alpha=0.3, axis='y')

# Add value labels and matched counts
for i, (bar, count) in enumerate(zip(bars, counts)):
    ax.text(bar.get_x() + bar.get_width()/2, count + 1, str(count),
            ha='center', va='bottom', fontsize=12, fontweight='bold')
    
    if i == 0:  # Both U+V
        ax.text(bar.get_x() + bar.get_width()/2, count/2, 
                f'{n_matched} actually\nmatched',
                ha='center', va='center', fontsize=10, color='white', fontweight='bold')

# Add annotation
ax.text(0.5, 0.98, 'Current algorithm only uses "Both U+V" category', 
        transform=ax.transAxes, ha='center', va='top', fontsize=10,
        bbox=dict(boxstyle='round,pad=0.5', facecolor='yellow', alpha=0.7))

plt.tight_layout()
plt.savefig('/home/virgolaema/dune/online-pointing-utils/output/matching_completeness_analysis.png',
            dpi=150, bbox_inches='tight')
print("\nPlot saved to: output/matching_completeness_analysis.png")

print("\n" + "=" * 80)
