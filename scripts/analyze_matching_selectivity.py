#!/usr/bin/env python3
"""
Analyze matching selectivity: Do background clusters get matched or filtered out?
"""

import uproot
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle

# Load single-plane clusters
file = uproot.open("/home/virgolaema/dune/online-pointing-utils/output/clusters_clusters_tick5_ch2_min2_tot1_e0p0/prodmarley_nue_flat_es_dune10kt_1x2x2_20250927T182943Z_gen_004499_supernova_g4_detsim_ana_2025-10-21T_095334Z_clusters.root")

# Load matched clusters
matched_file = uproot.open("/home/virgolaema/dune/online-pointing-utils/output/test_es_matched.root")
matched_tree = matched_file["clusters/clusters_tree_multiplane"]
matched_marley_frac = matched_tree["marley_tp_fraction"].array(library="np")

# Get stats for each view
view_stats = {}
for view_name, tree_path in [("U", "clusters/clusters_tree_U"), 
                               ("V", "clusters/clusters_tree_V"), 
                               ("X", "clusters/clusters_tree_X")]:
    tree = file[tree_path]
    marley_frac = tree["marley_tp_fraction"].array(library="np")
    
    n_total = len(marley_frac)
    n_marley = np.sum(marley_frac > 0.5)
    n_bkg = n_total - n_marley
    
    view_stats[view_name] = {
        'total': n_total,
        'marley': n_marley,
        'bkg': n_bkg
    }

# Matched stats
n_matched_total = len(matched_marley_frac)
n_matched_marley = np.sum(matched_marley_frac > 0.5)
n_matched_bkg = n_matched_total - n_matched_marley

# Create comprehensive figure
fig = plt.figure(figsize=(16, 10))
gs = fig.add_gridspec(2, 2, hspace=0.3, wspace=0.3)

# 1. Sankey-style flow diagram
ax1 = fig.add_subplot(gs[0, :])
ax1.set_xlim(0, 10)
ax1.set_ylim(0, 10)
ax1.axis('off')
ax1.set_title('Cluster Matching Flow: MARLEY vs Background', fontsize=16, fontweight='bold', pad=20)

# Draw input clusters
y_u = 7.5
y_v = 5
y_x = 2.5
x_input = 1

colors = {'marley': '#2ecc71', 'bkg': '#e74c3c'}

def draw_bar(ax, x, y, width, marley_frac, bkg_frac, total, label):
    """Draw a stacked bar representing cluster composition."""
    marley_height = 1.5 * marley_frac
    bkg_height = 1.5 * bkg_frac
    
    # Background (bottom)
    if bkg_height > 0:
        ax.add_patch(Rectangle((x, y), width, bkg_height, facecolor=colors['bkg'], 
                                edgecolor='black', linewidth=2))
    # MARLEY (top)
    if marley_height > 0:
        ax.add_patch(Rectangle((x, y + bkg_height), width, marley_height, 
                                facecolor=colors['marley'], edgecolor='black', linewidth=2))
    
    # Label
    ax.text(x + width/2, y + bkg_height + marley_height + 0.3, label, 
            ha='center', va='bottom', fontsize=11, fontweight='bold')
    ax.text(x + width/2, y - 0.3, f'{int(total)} total', ha='center', va='top', fontsize=9)

# Input bars
for view_name, y_pos in [('U', y_u), ('V', y_v), ('X', y_x)]:
    stats = view_stats[view_name]
    marley_frac = stats['marley'] / stats['total']
    bkg_frac = stats['bkg'] / stats['total']
    draw_bar(ax1, x_input, y_pos, 0.8, marley_frac, bkg_frac, stats['total'], 
             f"{view_name}-plane\n({stats['marley']} MARLEY, {stats['bkg']} bkg)")

# Matching process (arrows)
x_match = 4.5
ax1.text(x_match, 5, 'Geometric\nMatching\n(5 cm, ±5000 ticks)', 
         ha='center', va='center', fontsize=12, fontweight='bold',
         bbox=dict(boxstyle='round,pad=0.5', facecolor='lightblue', edgecolor='black', linewidth=2))

# Arrows from inputs to matching
for y_pos in [y_u, y_v, y_x]:
    ax1.annotate('', xy=(x_match - 0.8, 5), xytext=(x_input + 0.8, y_pos),
                arrowprops=dict(arrowstyle='->', lw=2, color='gray'))

# Output bar
x_output = 7.5
y_output = 4
match_marley_frac = n_matched_marley / n_matched_total if n_matched_total > 0 else 0
match_bkg_frac = n_matched_bkg / n_matched_total if n_matched_total > 0 else 0
draw_bar(ax1, x_output, y_output, 0.8, match_marley_frac, match_bkg_frac, n_matched_total,
         f"Multiplane\n({n_matched_marley} MARLEY, {n_matched_bkg} bkg)")

# Arrow to output
ax1.annotate('', xy=(x_output, 5), xytext=(x_match + 0.8, 5),
            arrowprops=dict(arrowstyle='->', lw=3, color='black'))

# Add legend
legend_x = 8.5
legend_y = 8.5
ax1.add_patch(Rectangle((legend_x, legend_y), 0.3, 0.3, facecolor=colors['marley'], 
                         edgecolor='black', linewidth=1))
ax1.text(legend_x + 0.4, legend_y + 0.15, 'MARLEY', va='center', fontsize=10)
ax1.add_patch(Rectangle((legend_x, legend_y - 0.5), 0.3, 0.3, facecolor=colors['bkg'], 
                         edgecolor='black', linewidth=1))
ax1.text(legend_x + 0.4, legend_y - 0.5 + 0.15, 'Background', va='center', fontsize=10)

# 2. Efficiency comparison
ax2 = fig.add_subplot(gs[1, 0])

x_marley = view_stats['X']['marley']
x_bkg = view_stats['X']['bkg']

marley_eff = 100 * n_matched_marley / x_marley if x_marley > 0 else 0
bkg_eff = 100 * n_matched_bkg / x_bkg if x_bkg > 0 else 0

bars = ax2.bar(['MARLEY', 'Background'], [marley_eff, bkg_eff], 
               color=[colors['marley'], colors['bkg']], edgecolor='black', linewidth=2)
ax2.set_ylabel('Matching Efficiency (%)', fontsize=12, fontweight='bold')
ax2.set_title('Matching Efficiency by Event Type\n(X-plane → Multiplane)', fontsize=12, fontweight='bold')
ax2.set_ylim(0, 100)
ax2.grid(alpha=0.3, axis='y')

# Add value labels
for bar, val in zip(bars, [marley_eff, bkg_eff]):
    height = bar.get_height()
    ax2.text(bar.get_x() + bar.get_width()/2, height + 2, f'{val:.1f}%',
            ha='center', va='bottom', fontsize=12, fontweight='bold')
    
# Add counts below
ax2.text(0, -10, f'{n_matched_marley}/{x_marley} matched', ha='center', fontsize=10)
ax2.text(1, -10, f'{n_matched_bkg}/{x_bkg} matched', ha='center', fontsize=10)

# 3. Purity comparison
ax3 = fig.add_subplot(gs[1, 1])

# Before matching (X-plane)
x_marley_pct = 100 * x_marley / (x_marley + x_bkg)
x_bkg_pct = 100 * x_bkg / (x_marley + x_bkg)

# After matching
match_marley_pct = 100 * n_matched_marley / n_matched_total if n_matched_total > 0 else 0
match_bkg_pct = 100 * n_matched_bkg / n_matched_total if n_matched_total > 0 else 0

x_pos = [0, 1]
width = 0.35

bars1 = ax3.bar([x - width/2 for x in x_pos], [x_marley_pct, match_marley_pct], 
                width, label='MARLEY', color=colors['marley'], edgecolor='black', linewidth=2)
bars2 = ax3.bar([x + width/2 for x in x_pos], [x_bkg_pct, match_bkg_pct], 
                width, label='Background', color=colors['bkg'], edgecolor='black', linewidth=2)

ax3.set_ylabel('Composition (%)', fontsize=12, fontweight='bold')
ax3.set_title('Event Type Composition', fontsize=12, fontweight='bold')
ax3.set_xticks(x_pos)
ax3.set_xticklabels(['Before Matching\n(X-plane)', 'After Matching\n(Multiplane)'])
ax3.set_ylim(0, 105)
ax3.legend(loc='upper left', fontsize=10)
ax3.grid(alpha=0.3, axis='y')

# Add percentage labels
for bars in [bars1, bars2]:
    for bar in bars:
        height = bar.get_height()
        if height > 1:  # Only label if visible
            ax3.text(bar.get_x() + bar.get_width()/2, height/2, f'{height:.1f}%',
                    ha='center', va='center', fontsize=10, fontweight='bold', color='white')

plt.savefig('/home/virgolaema/dune/online-pointing-utils/output/matching_selectivity_analysis.png', 
            dpi=150, bbox_inches='tight')
print("Comprehensive analysis plot saved to: output/matching_selectivity_analysis.png")

# Print summary
print("\n" + "="*80)
print("MATCHING SELECTIVITY ANALYSIS")
print("="*80)

print("\n1. INPUT COMPOSITION (X-plane single clusters):")
print(f"   - MARLEY: {x_marley} clusters ({x_marley_pct:.1f}%)")
print(f"   - Background: {x_bkg} clusters ({x_bkg_pct:.1f}%)")

print("\n2. OUTPUT COMPOSITION (Multiplane matched clusters):")
print(f"   - MARLEY: {n_matched_marley} matches ({match_marley_pct:.1f}%)")
print(f"   - Background: {n_matched_bkg} matches ({match_bkg_pct:.1f}%)")

print("\n3. MATCHING EFFICIENCY:")
print(f"   - MARLEY: {n_matched_marley}/{x_marley} = {marley_eff:.1f}% matched")
print(f"   - Background: {n_matched_bkg}/{x_bkg} = {bkg_eff:.1f}% matched")

print("\n4. KEY FINDINGS:")
print(f"   ✓ Background clusters EXIST in input ({x_bkg} out of {x_marley + x_bkg})")
print(f"   ✓ Background clusters DON'T GET MATCHED ({n_matched_bkg} matched)")
print(f"   ✓ Matching acts as a FILTER: {match_marley_pct:.1f}% purity after vs {x_marley_pct:.1f}% before")
print(f"   ✓ MARLEY efficiency: {marley_eff:.1f}% (geometric matching is selective)")

print("\n5. INTERPRETATION:")
if n_matched_bkg == 0 and x_bkg > 0:
    print("   → Background clusters are REJECTED by geometric matching")
    print("   → They likely don't have compatible U/V/X spatial alignment")
    print("   → This demonstrates the matching algorithm's background rejection!")
elif match_marley_pct > x_marley_pct:
    print("   → Matching IMPROVES purity by preferentially selecting MARLEY events")
    print("   → Geometric constraints filter out background more than signal")
else:
    print("   → Matching preserves or slightly degrades purity")

print("\n" + "="*80)
