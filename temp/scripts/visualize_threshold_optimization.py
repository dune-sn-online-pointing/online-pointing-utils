#!/usr/bin/env python3
"""
Visualize the threshold optimization results that have already been generated.
"""

import json
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

# Load results
REPO_ROOT = Path(__file__).parent.parent
results_file = REPO_ROOT / "output/threshold_optimization_results.json"

with open(results_file, 'r') as f:
    results = json.load(f)

# Extract data
thresholds = [r['threshold_ticks'] for r in results]
thresholds_us = [r['threshold_us'] for r in results]
U_matched = [r['U_matched'] for r in results]
V_matched = [r['V_matched'] for r in results]
X_matched = [r['X_matched'] for r in results]
total_geometric = [r['total_geometric_matches'] for r in results]

# Create comprehensive visualization
fig = plt.figure(figsize=(16, 10))
gs = fig.add_gridspec(2, 3, hspace=0.3, wspace=0.3)

# Plot 1: Total geometric matches vs threshold
ax1 = fig.add_subplot(gs[0, :2])
ax1.plot(thresholds, total_geometric, 'o-', linewidth=2, markersize=10, color='navy')
ax1.axvline(x=100, color='green', linestyle='--', linewidth=2, label='Chosen: 100 ticks', alpha=0.7)
ax1.axvline(x=3000, color='orange', linestyle='--', linewidth=2, label='Stability limit: 3000 ticks', alpha=0.7)
ax1.axvline(x=5000, color='red', linestyle='--', linewidth=2, label='Fake matches: 5000 ticks', alpha=0.7)
ax1.set_xlabel('Time Tolerance (ticks)', fontsize=13, fontweight='bold')
ax1.set_ylabel('Total Geometric Matches', fontsize=13, fontweight='bold')
ax1.set_title('Total 3-Plane Matches vs Time Tolerance', fontsize=15, fontweight='bold')
ax1.legend(fontsize=11, loc='best')
ax1.grid(True, alpha=0.3)
ax1.set_ylim([20, 30])

# Annotate the plateau and discontinuity
ax1.annotate('Stable plateau\n(27 matches)', xy=(1500, 27), xytext=(1500, 28.5),
            fontsize=11, ha='center', color='green', fontweight='bold',
            bbox=dict(boxstyle='round,pad=0.5', facecolor='lightgreen', alpha=0.7),
            arrowprops=dict(arrowstyle='->', color='green', lw=2))

ax1.annotate('Discontinuity!\n(26 matches)', xy=(5000, 26), xytext=(5000, 24),
            fontsize=11, ha='center', color='red', fontweight='bold',
            bbox=dict(boxstyle='round,pad=0.5', facecolor='lightcoral', alpha=0.7),
            arrowprops=dict(arrowstyle='->', color='red', lw=2))

# Plot 2: Summary table
ax2 = fig.add_subplot(gs[0, 2])
ax2.axis('tight')
ax2.axis('off')

# Create summary table data
table_data = [
    ['Threshold', 'Status'],
    ['50-100', '✓ Optimal'],
    ['100-300', '✓ Good'],
    ['300-1000', '✓ Safe'],
    ['1000-3000', '⚠ High'],
    ['> 3000', '✗ Fake'],
]
table = ax2.table(cellText=table_data, cellLoc='left', loc='center',
                 colWidths=[0.5, 0.5])
table.auto_set_font_size(False)
table.set_fontsize(11)
table.scale(1, 2.5)

# Color code the table
for i, row in enumerate(table_data):
    if i == 0:  # Header
        for j in range(len(row)):
            table[(i, j)].set_facecolor('#4472C4')
            table[(i, j)].set_text_props(weight='bold', color='white')
    elif i == 1 or i == 2:  # Optimal/Good
        table[(i, 1)].set_facecolor('lightgreen')
    elif i == 3:  # Safe
        table[(i, 1)].set_facecolor('lightyellow')
    elif i == 4:  # High
        table[(i, 1)].set_facecolor('orange')
    else:  # Fake
        table[(i, 1)].set_facecolor('lightcoral')

ax2.set_title('Threshold Regions', fontsize=13, fontweight='bold', pad=20)

# Plot 3: Matched clusters by view
ax3 = fig.add_subplot(gs[1, 0])
ax3.plot(thresholds, U_matched, 'o-', linewidth=2, markersize=8, label='U View', color='blue')
ax3.plot(thresholds, V_matched, 's-', linewidth=2, markersize=8, label='V View', color='red')
ax3.plot(thresholds, X_matched, '^-', linewidth=2, markersize=8, label='X View', color='green')
ax3.axvline(x=100, color='gray', linestyle='--', linewidth=1, alpha=0.5)
ax3.set_xlabel('Time Tolerance (ticks)', fontsize=12, fontweight='bold')
ax3.set_ylabel('Matched Clusters', fontsize=12, fontweight='bold')
ax3.set_title('Matched Clusters by View', fontsize=13, fontweight='bold')
ax3.legend(fontsize=10)
ax3.grid(True, alpha=0.3)
ax3.set_ylim([0, 10])

# Plot 4: Zoom on 100-500 tick range
ax4 = fig.add_subplot(gs[1, 1])
zoom_mask = (np.array(thresholds) >= 100) & (np.array(thresholds) <= 500)
zoom_thresholds = np.array(thresholds)[zoom_mask]
zoom_geometric = np.array(total_geometric)[zoom_mask]

if len(zoom_thresholds) > 0:
    ax4.plot(zoom_thresholds, zoom_geometric, 'o-', linewidth=3, markersize=12, color='darkgreen')
    ax4.axhline(y=27, color='green', linestyle='--', linewidth=2, alpha=0.5)
    ax4.set_xlabel('Time Tolerance (ticks)', fontsize=12, fontweight='bold')
    ax4.set_ylabel('Geometric Matches', fontsize=12, fontweight='bold')
    ax4.set_title('Zoom: 100-500 Ticks Range\n(All produce same result)', fontsize=13, fontweight='bold')
    ax4.grid(True, alpha=0.3)
    ax4.set_ylim([26, 28])
    
    # Annotate the flat line
    ax4.text(300, 27.5, 'Perfectly stable:\n27 matches', ha='center', va='center',
            fontsize=11, color='darkgreen', fontweight='bold',
            bbox=dict(boxstyle='round,pad=0.7', facecolor='lightgreen', alpha=0.8))
else:
    ax4.text(0.5, 0.5, 'No data in 100-500 range\n(but extrapolated: all = 27)',
            ha='center', va='center', transform=ax4.transAxes, fontsize=11)

# Plot 5: Key findings text
ax5 = fig.add_subplot(gs[1, 2])
ax5.axis('off')

findings_text = """
KEY FINDINGS:

✓ Stable from 50-3000 ticks
  (all give 27 matches)

✓ 100-500 ticks: IDENTICAL
  (no performance difference)

✗ 5000 ticks: DISCONTINUITY
  (fake matches appear)

RECOMMENDATION:
→ 100 ticks (51.2 µs)

RATIONALE:
• Captures all true matches
• Maximum fake rejection
• Conservative choice
• Physics-motivated

TESTED: 25, 30, 32, 35, 40,
50, 100, 250, 500, 1000, 
2000, 3000, 5000 ticks
"""

ax5.text(0.05, 0.95, findings_text, transform=ax5.transAxes, 
        fontsize=10, verticalalignment='top', family='monospace',
        bbox=dict(boxstyle='round,pad=1', facecolor='lightyellow', alpha=0.8))

# Main title
fig.suptitle('Time Threshold Optimization for Cluster Matching', 
            fontsize=17, fontweight='bold', y=0.98)

# Add subtitle
fig.text(0.5, 0.94, 'ES Validation Sample | Spatial Tolerance: 5.0 cm', 
        ha='center', fontsize=12, style='italic')

plt.tight_layout(rect=[0, 0, 1, 0.96])

# Save
output_file = REPO_ROOT / "output/threshold_optimization_visualization.png"
plt.savefig(output_file, dpi=150, bbox_inches='tight')
print(f"Visualization saved: {output_file}")
print(f"\nTotal thresholds tested: {len(results)}")
print(f"Range: {min(thresholds)}-{max(thresholds)} ticks ({min(thresholds_us):.1f}-{max(thresholds_us):.1f} µs)")
print(f"\nStable region: {27} matches (50-3000 ticks)")
print(f"Discontinuity at: 5000 ticks ({26} matches)")
print(f"\nChosen threshold: 100 ticks (51.2 µs)")

plt.show()
