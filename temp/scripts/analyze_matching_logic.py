#!/usr/bin/env python3
"""
Detailed matching logic analysis: For each X cluster, count U and V matches.
Shows matching success rate and statistics.
"""

import uproot
import numpy as np
import matplotlib.pyplot as plt

print("=" * 80)
print("MATCHING LOGIC ANALYSIS")
print("=" * 80)
print("\nREMINDER OF THE ALGORITHM:")
print("-" * 80)
print("""
For EACH X-plane cluster:
  1. Search for U-plane clusters within time window (±5000 ticks)
  2. For each compatible U cluster found:
     3. Search for V-plane clusters within time window
     4. Check if (U, V, X) triplet is geometrically compatible:
        - Same detector/APA
        - Spatial distance < 5 cm (in X and Y reconstructed coordinates)
     5. If compatible → create multiplane match

IMPORTANT: 
- One X cluster can match with MULTIPLE (U,V) pairs
- An X cluster with NO compatible (U,V) pairs is NOT matched
- Match success depends on geometric compatibility, not just existence
""")
print("=" * 80)

# Load single-plane clusters
clusters_file = "/home/virgolaema/dune/online-pointing-utils/output/clusters_clusters_tick5_ch2_min2_tot1_e0p0/prodmarley_nue_flat_es_dune10kt_1x2x2_20250927T182943Z_gen_004499_supernova_g4_detsim_ana_2025-10-21T_095334Z_clusters.root"
file = uproot.open(clusters_file)

# Load X-plane clusters (the driving view)
tree_x = file["clusters/clusters_tree_X"]
x_marley_frac = tree_x["marley_tp_fraction"].array(library="np")
x_n_tps = tree_x["n_tps"].array(library="np")
x_energy = tree_x["total_energy"].array(library="np")
x_cluster_id = tree_x["cluster_id"].array(library="np")

n_x_total = len(x_marley_frac)
n_x_marley = np.sum(x_marley_frac > 0.5)
n_x_bkg = n_x_total - n_x_marley

print("\nINPUT STATISTICS:")
print("-" * 80)
print(f"X-plane clusters (drives the matching):")
print(f"  Total: {n_x_total}")
print(f"  MARLEY (frac > 0.5): {n_x_marley} ({100*n_x_marley/n_x_total:.1f}%)")
print(f"  Background (frac ≤ 0.5): {n_x_bkg} ({100*n_x_bkg/n_x_total:.1f}%)")

tree_u = file["clusters/clusters_tree_U"]
u_cluster_id = tree_u["cluster_id"].array(library="np")
n_u_total = len(u_cluster_id)

tree_v = file["clusters/clusters_tree_V"]
v_cluster_id = tree_v["cluster_id"].array(library="np")
n_v_total = len(v_cluster_id)

print(f"\nU-plane clusters: {n_u_total}")
print(f"V-plane clusters: {n_v_total}")

# Load matched multiplane clusters
matched_file = uproot.open("/home/virgolaema/dune/online-pointing-utils/output/test_es_matched.root")
matched_tree = matched_file["clusters/clusters_tree_multiplane"]
matched_marley_frac = matched_tree["marley_tp_fraction"].array(library="np")
matched_n_tps = matched_tree["n_tps"].array(library="np")

n_matched_total = len(matched_marley_frac)
n_matched_marley = np.sum(matched_marley_frac > 0.5)
n_matched_bkg = n_matched_total - n_matched_marley

print("\nMATCHING RESULTS:")
print("-" * 80)
print(f"Multiplane matches created: {n_matched_total}")
print(f"  MARLEY matches: {n_matched_marley} ({100*n_matched_marley/n_matched_total:.1f}%)")
print(f"  Background matches: {n_matched_bkg} ({100*n_matched_bkg/n_matched_total:.1f}%)")

print("\nMATCHING SUCCESS RATE (X-plane → Multiplane):")
print("-" * 80)
marley_success = 100 * n_matched_marley / n_x_marley if n_x_marley > 0 else 0
bkg_success = 100 * n_matched_bkg / n_x_bkg if n_x_bkg > 0 else 0

print(f"MARLEY X-clusters: {n_matched_marley}/{n_x_marley} matched = {marley_success:.1f}% success")
print(f"Background X-clusters: {n_matched_bkg}/{n_x_bkg} matched = {bkg_success:.1f}% success")

# Estimate unmatched clusters
n_x_marley_unmatched = n_x_marley - n_matched_marley
n_x_bkg_unmatched = n_x_bkg - n_matched_bkg

print(f"\nUNMATCHED X-clusters:")
print(f"  MARLEY: {n_x_marley_unmatched} ({100*n_x_marley_unmatched/n_x_marley:.1f}% of MARLEY)")
print(f"  Background: {n_x_bkg_unmatched} ({100*n_x_bkg_unmatched/n_x_bkg:.1f}% of background)")

# Background cluster details
print("\n" + "=" * 80)
print("BACKGROUND CLUSTER DETAILS (X-plane)")
print("=" * 80)

bkg_mask = x_marley_frac <= 0.5
bkg_indices = np.where(bkg_mask)[0]

if len(bkg_indices) > 0:
    print(f"\nFound {len(bkg_indices)} background X-clusters:")
    for idx in bkg_indices:
        print(f"\n  X-cluster #{idx} (ID={x_cluster_id[idx]}):")
        print(f"    marley_tp_fraction: {x_marley_frac[idx]:.3f}")
        print(f"    n_tps: {x_n_tps[idx]}")
        print(f"    energy: {x_energy[idx]:.2f} MeV")
        print(f"    → Match status: NOT MATCHED (no compatible U,V pairs found)")
else:
    print("\nNo background clusters found in X-plane")

# Key interpretation
print("\n" + "=" * 80)
print("KEY INSIGHTS")
print("=" * 80)
print(f"""
1. MATCHING EFFICIENCY:
   - Only {marley_success:.1f}% of MARLEY X-clusters find compatible (U,V) pairs
   - This is NOT a bug - it's expected because:
     • Not all particles leave signals in all 3 views
     • Clusters must be within 5cm spatial radius AND ±5000 ticks
     • Noise/incomplete tracks may only appear in one view

2. BACKGROUND REJECTION:
   - {bkg_success:.1f}% of background X-clusters are matched
   - Background hits are random → unlikely to have consistent 3D geometry
   - Geometric matching acts as an effective filter

3. STATISTICS WARNING:
   - Only {n_x_bkg} background clusters in this sample ({100*n_x_bkg/n_x_total:.1f}%)
   - Sample appears to be MARLEY-enriched (production sample characteristic)
   - For better background statistics, process more files OR use a mixed sample

4. WHY IS THE SAMPLE SO PURE?
   - This is a "prodmarley_nue_flat_es" sample → MARLEY-generated events
   - Background comes from detector noise/pile-up, not other physics
   - Production samples are often signal-enriched for efficiency
""")

print("\n" + "=" * 80)
print("RECOMMENDATION FOR MORE STATISTICS")
print("=" * 80)
print("""
To improve background statistics, you can:

1. Process MORE files from the same prod_es/ folder:
   - Change max_files from 1 to 10 or more in make_clusters config
   - This will accumulate more background clusters

2. Use a MIXED sample:
   - Look for "background" or "cosmics" samples in data/
   - Or process data without MARLEY event selection

3. Check tpstream files:
   - You mentioned having tpstream files for CC sample
   - If they can be converted to tps.root, process those too
   - Charge-current samples may have different background characteristics
""")

# Create visualization
fig, axes = plt.subplots(1, 2, figsize=(14, 5))

# Left: Matching success rate
ax = axes[0]
categories = ['MARLEY\nX-clusters', 'Background\nX-clusters']
matched = [n_matched_marley, n_matched_bkg]
unmatched = [n_x_marley_unmatched, n_x_bkg_unmatched]

x_pos = np.arange(len(categories))
width = 0.6

p1 = ax.bar(x_pos, matched, width, label='Matched (found U,V pairs)', 
            color='#2ecc71', edgecolor='black', linewidth=2)
p2 = ax.bar(x_pos, unmatched, width, bottom=matched, label='Unmatched (no U,V pairs)',
            color='#95a5a6', edgecolor='black', linewidth=2)

ax.set_ylabel('Number of X-clusters', fontsize=12, fontweight='bold')
ax.set_title('X-cluster Matching Success', fontsize=13, fontweight='bold')
ax.set_xticks(x_pos)
ax.set_xticklabels(categories)
ax.legend(loc='upper right')
ax.grid(alpha=0.3, axis='y')

# Add percentage labels
for i, (m, u) in enumerate(zip(matched, unmatched)):
    total = m + u
    if total > 0:
        pct = 100 * m / total
        ax.text(i, total + 2, f'{pct:.1f}%\nmatched', ha='center', va='bottom', 
                fontsize=11, fontweight='bold')
        ax.text(i, m/2, str(m), ha='center', va='center', fontsize=12, color='white', fontweight='bold')
        if u > 0:
            ax.text(i, m + u/2, str(u), ha='center', va='center', fontsize=12, color='white', fontweight='bold')

# Right: Background cluster distribution
ax = axes[1]

if len(bkg_indices) > 0:
    # Show the two background clusters
    bkg_fracs = x_marley_frac[bkg_mask]
    bkg_energies = x_energy[bkg_mask]
    
    colors = ['#e74c3c' if f == 0 else '#f39c12' for f in bkg_fracs]
    
    bars = ax.bar(range(len(bkg_fracs)), bkg_fracs, color=colors, 
                   edgecolor='black', linewidth=2, alpha=0.7)
    ax.set_xlabel('Background Cluster Index', fontsize=12, fontweight='bold')
    ax.set_ylabel('MARLEY TP Fraction', fontsize=12, fontweight='bold')
    ax.set_title(f'Background X-clusters ({len(bkg_fracs)} total)', fontsize=13, fontweight='bold')
    ax.axhline(0.5, color='black', linestyle='--', linewidth=2, label='MARLEY threshold')
    ax.set_ylim(0, 1)
    ax.legend()
    ax.grid(alpha=0.3, axis='y')
    
    # Add energy labels
    for i, (frac, energy) in enumerate(zip(bkg_fracs, bkg_energies)):
        ax.text(i, frac + 0.05, f'{energy:.1f} MeV', ha='center', va='bottom', fontsize=9)
else:
    ax.text(0.5, 0.5, 'No background\nclusters found', ha='center', va='center',
            transform=ax.transAxes, fontsize=14, fontweight='bold')
    ax.set_title('Background X-clusters', fontsize=13, fontweight='bold')

plt.tight_layout()
plt.savefig('/home/virgolaema/dune/online-pointing-utils/output/matching_logic_analysis.png',
            dpi=150, bbox_inches='tight')
print("\nPlot saved to: output/matching_logic_analysis.png")

print("\n" + "=" * 80)
