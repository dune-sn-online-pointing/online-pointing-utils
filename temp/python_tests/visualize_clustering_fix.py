"""
Visual explanation of the clustering bug and fix.
"""

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

# Example data showing the chaining problem
tps_buggy = [
    (4050, 10, 'A'),
    (4055, 12, 'B'),
    (4060, 14, 'C'),
    (4065, 16, 'D'),
    (4140, 11, 'E'),
    (4145, 13, 'F'),
    (4180, 10, 'G'),
    (4185, 12, 'H'),
]

# Left plot: OLD BUGGY BEHAVIOR
ax1.set_title('OLD BUGGY ALGORITHM\n(All connected into 1 cluster)', fontsize=14, fontweight='bold')
ax1.set_xlabel('Channel', fontsize=12)
ax1.set_ylabel('Time (ticks)', fontsize=12)
ax1.grid(True, alpha=0.3)

# Draw all TPs in one color (same cluster)
for time, ch, label in tps_buggy:
    ax1.scatter(ch, time, s=300, c='red', marker='o', edgecolors='black', linewidths=2, zorder=3)
    ax1.text(ch, time, label, ha='center', va='center', fontsize=12, fontweight='bold', color='white')

# Draw arrows showing the chaining
arrows = [
    (10, 4050, 12, 4055),  # A → B
    (12, 4055, 14, 4060),  # B → C
    (14, 4060, 16, 4065),  # C → D
    (16, 4065, 11, 4140),  # D → E (BUG: large time gap!)
    (11, 4140, 13, 4145),  # E → F
    (13, 4145, 10, 4180),  # F → G (BUG: large time gap!)
    (10, 4180, 12, 4185),  # G → H
]

for ch1, t1, ch2, t2 in arrows:
    if abs(t2 - t1) > 20:
        # Large time gap - show as dashed red (bug!)
        ax1.annotate('', xy=(ch2, t2), xytext=(ch1, t1),
                    arrowprops=dict(arrowstyle='->', lw=3, color='red', linestyle='--'))
        # Add warning label
        mid_t = (t1 + t2) / 2
        mid_ch = (ch1 + ch2) / 2
        ax1.text(mid_ch + 0.5, mid_t, f'Δt={t2-t1}!', fontsize=10, color='red', 
                fontweight='bold', bbox=dict(boxstyle='round', facecolor='yellow', alpha=0.8))
    else:
        ax1.annotate('', xy=(ch2, t2), xytext=(ch1, t1),
                    arrowprops=dict(arrowstyle='->', lw=2, color='darkred'))

ax1.set_xlim(8, 18)
ax1.set_ylim(4040, 4195)
ax1.text(13, 4035, 'BUG: Temporal chaining allows\nTPs 130 ticks apart in same cluster!',
         ha='center', fontsize=11, color='red', fontweight='bold',
         bbox=dict(boxstyle='round', facecolor='yellow', alpha=0.8))

# Right plot: NEW FIXED BEHAVIOR
ax2.set_title('NEW FIXED ALGORITHM\n(Separated into 3 clusters)', fontsize=14, fontweight='bold')
ax2.set_xlabel('Channel', fontsize=12)
ax2.set_ylabel('Time (ticks)', fontsize=12)
ax2.grid(True, alpha=0.3)

# Draw TPs in different colors (different clusters)
clusters = {
    'Cluster 1': [(4050, 10, 'A'), (4055, 12, 'B'), (4060, 14, 'C'), (4065, 16, 'D')],
    'Cluster 2': [(4140, 11, 'E'), (4145, 13, 'F')],
    'Cluster 3': [(4180, 10, 'G'), (4185, 12, 'H')],
}

colors = ['blue', 'green', 'purple']

for i, (cluster_name, tps) in enumerate(clusters.items()):
    color = colors[i]
    for time, ch, label in tps:
        ax2.scatter(ch, time, s=300, c=color, marker='o', edgecolors='black', linewidths=2, zorder=3)
        ax2.text(ch, time, label, ha='center', va='center', fontsize=12, fontweight='bold', color='white')
    
    # Draw connections within each cluster
    for j in range(len(tps) - 1):
        ch1, t1 = tps[j][1], tps[j][0]
        ch2, t2 = tps[j+1][1], tps[j+1][0]
        ax2.annotate('', xy=(ch2, t2), xytext=(ch1, t1),
                    arrowprops=dict(arrowstyle='->', lw=2, color=color))

ax2.set_xlim(8, 18)
ax2.set_ylim(4040, 4195)

# Add legend for clusters
legend_elements = [
    mpatches.Patch(color='blue', label='Cluster 1 (span: 15 ticks)'),
    mpatches.Patch(color='green', label='Cluster 2 (span: 5 ticks)'),
    mpatches.Patch(color='purple', label='Cluster 3 (span: 5 ticks)'),
]
ax2.legend(handles=legend_elements, loc='upper left', fontsize=10)

ax2.text(13, 4035, 'FIXED: Each TP must be within tick_limit\nAND channel_limit of existing TP',
         ha='center', fontsize=11, color='green', fontweight='bold',
         bbox=dict(boxstyle='round', facecolor='lightgreen', alpha=0.8))

plt.tight_layout()
plt.savefig('clustering_bug_fix_diagram.png', dpi=150, bbox_inches='tight')
print("Diagram saved to clustering_bug_fix_diagram.png")
plt.show()
