#!/usr/bin/env python3
"""
Visual test of pentagon algorithm - generates a plot showing the pentagon shape
"""

import sys
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent / 'python'))

from cluster_display import DrawingAlgorithms

def visualize_pentagon(time_start, time_peak, time_end, adc_peak, adc_integral, threshold, title="Pentagon Visualization"):
    """Create a visualization of the pentagon algorithm result"""
    
    # Calculate pentagon parameters
    params = DrawingAlgorithms.calculate_pentagon_params(
        time_start, time_peak, time_end, adc_peak, adc_integral, threshold
    )
    
    if not params:
        print("Failed to calculate pentagon parameters!")
        return
    
    # Create figure
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    # Left plot: Pentagon vertices
    vertices = [
        [time_start, threshold],
        [params['time_int_rise'], params['h_int_rise']],
        [time_peak, adc_peak],
        [params['time_int_fall'], params['h_int_fall']],
        [time_end, threshold],
        [time_start, threshold]  # Close the polygon
    ]
    vertices = np.array(vertices)
    
    # Plot pentagon
    ax1.fill(vertices[:, 0], vertices[:, 1], alpha=0.3, color='blue', label='Pentagon')
    ax1.plot(vertices[:, 0], vertices[:, 1], 'b-', linewidth=2, marker='o', markersize=8)
    
    # Plot plateau baseline
    ax1.fill_between([time_start, time_end], [0, 0], [threshold, threshold], 
                     alpha=0.2, color='gray', label='Plateau baseline')
    ax1.axhline(threshold, color='red', linestyle='--', linewidth=1, label=f'Threshold = {threshold}')
    
    # Annotations
    ax1.plot([time_peak], [adc_peak], 'ro', markersize=10, label='Peak')
    ax1.text(time_start, threshold, f'  t_start={time_start}', va='bottom')
    ax1.text(time_peak, adc_peak, f'  peak={adc_peak:.0f}', va='bottom')
    ax1.text(time_end, threshold, f't_end={time_end}  ', ha='right', va='bottom')
    
    ax1.set_xlabel('Time [ticks]', fontsize=12)
    ax1.set_ylabel('ADC', fontsize=12)
    ax1.set_title(title, fontsize=14, fontweight='bold')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Right plot: Area comparison
    pentagon_area = DrawingAlgorithms.polygon_area(vertices[:-1].tolist())
    total_area = pentagon_area + params['offset_area']
    diff = abs(total_area - adc_integral)
    
    areas = {
        'Plateau\nBaseline': params['offset_area'],
        'Pentagon\nAbove Threshold': pentagon_area,
        'Total\n(Pentagon + Plateau)': total_area,
        'Target\nADC Integral': adc_integral
    }
    
    colors = ['gray', 'blue', 'green', 'orange']
    bars = ax2.bar(range(len(areas)), areas.values(), color=colors, alpha=0.6, edgecolor='black')
    ax2.set_xticks(range(len(areas)))
    ax2.set_xticklabels(areas.keys())
    ax2.set_ylabel('Area [ADC × ticks]', fontsize=12)
    ax2.set_title('Area Breakdown', fontsize=14, fontweight='bold')
    ax2.grid(True, alpha=0.3, axis='y')
    
    # Add value labels on bars
    for i, (bar, (name, value)) in enumerate(zip(bars, areas.items())):
        height = bar.get_height()
        ax2.text(bar.get_x() + bar.get_width()/2., height,
                f'{value:.1f}',
                ha='center', va='bottom', fontsize=10, fontweight='bold')
    
    # Add difference annotation
    ax2.text(0.5, 0.95, f'Difference: {diff:.2f} ADC×ticks\n({100*diff/adc_integral:.2f}% of target)',
             transform=ax2.transAxes, ha='center', va='top',
             bbox=dict(boxstyle='round', facecolor='yellow', alpha=0.5),
             fontsize=11)
    
    # Add parameter info
    info_text = f"h_opt = {params['h_opt']:.2f} (height above threshold)\n"
    info_text += f"t1 = {params['time_int_rise']:.1f}, t2 = {params['time_int_fall']:.1f}"
    ax1.text(0.02, 0.98, info_text, transform=ax1.transAxes,
             va='top', ha='left',
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8),
             fontsize=10, family='monospace')
    
    plt.tight_layout()
    return fig

def main():
    """Run visualization tests"""
    print("Pentagon Algorithm Visual Tests")
    print("=" * 60)
    
    # Test case 1: Typical TP
    print("\nTest 1: Typical trigger primitive (X plane)")
    fig1 = visualize_pentagon(
        time_start=100, time_peak=105, time_end=115,
        adc_peak=180, adc_integral=900, threshold=60,
        title="Test 1: Typical TP (X plane, threshold=60)"
    )
    
    # Test case 2: High amplitude TP
    print("\nTest 2: High amplitude TP (U plane)")
    fig2 = visualize_pentagon(
        time_start=200, time_peak=207, time_end=220,
        adc_peak=300, adc_integral=2500, threshold=70,
        title="Test 2: High Amplitude TP (U plane, threshold=70)"
    )
    
    # Test case 3: Narrow TP
    print("\nTest 3: Narrow TP with high peak")
    fig3 = visualize_pentagon(
        time_start=50, time_peak=53, time_end=58,
        adc_peak=250, adc_integral=800, threshold=60,
        title="Test 3: Narrow TP (short duration, high peak)"
    )
    
    print("\n" + "=" * 60)
    print("Displaying plots... Close windows to exit.")
    plt.show()

if __name__ == '__main__':
    main()
