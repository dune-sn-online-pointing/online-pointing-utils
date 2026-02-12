#!/usr/bin/env python3
"""
Visualize pentagon construction from TP information
Shows how we reconstruct ADC waveform shape from trigger primitive data
"""

import numpy as np
import matplotlib.pyplot as plt
import sys
from pathlib import Path

# Add scripts directory to path
sys.path.insert(0, str(Path(__file__).parent))
from generate_cluster_arrays import calculate_pentagon_params


def visualize_pentagon_from_tp(time_start, samples_over_threshold, samples_to_peak, 
                                adc_integral, adc_peak=None, threshold_adc=60.0,
                                title="Pentagon Construction from TP Data"):
    """
    Visualize how pentagon is constructed from TP information
    
    TP provides:
    - time_start: When signal crosses threshold
    - samples_over_threshold: Duration of signal
    - samples_to_peak: Time to reach peak
    - adc_integral: Total charge (area under curve)
    
    Pentagon algorithm reconstructs the waveform shape
    Args:
        threshold_adc: Plane threshold (60 for X/collection, 70 for U/V induction) - the plateau baseline
    """
    time_end = time_start + samples_over_threshold
    time_peak = time_start + samples_to_peak
    
    # Estimate peak from integral if not provided
    if adc_peak is None:
        adc_peak = adc_integral / max(samples_over_threshold, 1) * 2
    
    # Calculate pentagon parameters using plane-specific threshold
    params = calculate_pentagon_params(time_start, time_peak, time_end, 
                                       adc_peak, adc_integral, threshold_adc)
    
    if not params[5]:  # Check valid flag
        print("Pentagon calculation failed!")
        return
    
    time_int_rise, time_int_fall, h_int_rise, h_int_fall, threshold, valid = params
    
    # Generate time samples
    time_samples = np.arange(time_start - 5, time_end + 5, 0.1)
    pentagon_values = []
    
    for t in time_samples:
        # IMPORTANT: Pentagon sits on a threshold baseline (plateau)
        # Outside the signal region, we show 0, but the pentagon itself has the baseline
        if t < time_start:
            intensity = 0.0  # Before signal
        elif t > time_end:
            intensity = 0.0  # After signal
        elif t == time_start or t == time_end:
            intensity = threshold  # Endpoints at threshold (the plateau)
        elif t < time_int_rise:
            span = time_int_rise - time_start
            if span > 0:
                frac = (t - time_start) / span
                intensity = threshold + frac * (h_int_rise - threshold)
            else:
                intensity = threshold
        elif t < time_peak:
            span = time_peak - time_int_rise
            if span > 0:
                frac = (t - time_int_rise) / span
                intensity = h_int_rise + frac * (adc_peak - h_int_rise)
            else:
                intensity = adc_peak
        elif t == time_peak:
            intensity = adc_peak
        elif t <= time_int_fall:
            span = time_int_fall - time_peak
            if span > 0:
                frac = (t - time_peak) / span
                intensity = adc_peak - frac * (adc_peak - h_int_fall)
            else:
                intensity = h_int_fall
        else:  # Between time_int_fall and time_end
            span = time_end - time_int_fall
            if span > 0:
                frac = (t - time_int_fall) / span
                intensity = h_int_fall - frac * (h_int_fall - threshold)
            else:
                intensity = threshold
        
        pentagon_values.append(intensity)
    
    pentagon_values = np.array(pentagon_values)
    
    # Create figure
    fig, ax = plt.subplots(figsize=(12, 7))
    
    # Plot pentagon reconstruction
    ax.plot(time_samples, pentagon_values, 'b-', linewidth=2.5, label='Pentagon reconstruction', zorder=3)
    
    # Highlight the TP window (where signal is above threshold)
    ax.axvspan(time_start, time_end, alpha=0.15, color='green', label='TP window')
    
    # CLEARLY SHOW THE THRESHOLD PLATEAU
    # Draw a thick line showing the plateau/baseline that the pentagon sits on
    ax.plot([time_start, time_end], [threshold, threshold], 'r-', linewidth=4, 
            alpha=0.7, label=f'Threshold plateau = {threshold:.1f} ADC (plane-specific)', zorder=2)
    
    # Also draw threshold line across the plot
    ax.axhline(threshold, color='red', linestyle='--', linewidth=1.5, alpha=0.5, zorder=1)
    ax.axhline(0, color='black', linestyle=':', linewidth=1, alpha=0.5, zorder=1)
    
    # Mark key TP information points
    # Start and end (at threshold)
    ax.plot([time_start, time_end], [threshold, threshold], 'go', markersize=10, 
            label='TP start/end (ON threshold plateau)', zorder=4)
    
    # Peak
    ax.plot(time_peak, adc_peak, 'r*', markersize=20, 
            label=f'Peak (t={time_peak:.0f}, ADC={adc_peak:.1f})', zorder=4)
    
    # Intermediate vertices
    ax.plot([time_int_rise, time_int_fall], [h_int_rise, h_int_fall], 'mo', markersize=8,
            label=f'Intermediate vertices (h={h_int_rise:.1f} ADC)', zorder=4)
    
    # Pentagon vertices outline
    vertices_t = [time_start, time_int_rise, time_peak, time_int_fall, time_end]
    vertices_adc = [threshold, h_int_rise, adc_peak, h_int_fall, threshold]
    ax.plot(vertices_t, vertices_adc, 'b--', linewidth=1, alpha=0.5, zorder=2)
    
    # Annotate TP parameters
    info_text = (
        f"TP Information:\n"
        f"• time_start = {time_start:.0f} ticks\n"
        f"• samples_over_threshold = {samples_over_threshold:.0f}\n"
        f"• samples_to_peak = {samples_to_peak:.0f}\n"
        f"• adc_integral = {adc_integral:.0f}\n"
        f"• adc_peak (estimated) = {adc_peak:.1f}\n"
        f"\n"
        f"Pentagon Construction:\n"
        f"• PLATEAU = {threshold:.1f} ADC ← BASE\n"
        f"  (X plane: 60, U/V planes: 70)\n"
        f"• h_intermediate = {h_int_rise:.1f} ADC\n"
        f"• time_int_rise = {time_int_rise:.1f}\n"
        f"• time_int_fall = {time_int_fall:.1f}\n"
        f"\n"
        f"Pentagon sits ON the plateau,\n"
        f"NOT on zero!"
    )
    
    ax.text(0.02, 0.98, info_text, transform=ax.transAxes, 
            fontsize=10, verticalalignment='top',
            bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))
    
    ax.set_xlabel('Time [ticks]', fontsize=12)
    ax.set_ylabel('Amplitude [ADC]', fontsize=12)
    ax.set_title(title, fontsize=14, fontweight='bold')
    ax.legend(loc='upper right', fontsize=10)
    ax.grid(True, alpha=0.3)
    ax.set_xlim(time_start - 5, time_end + 5)
    
    # Set y-axis to show from 0 to slightly above peak
    ax.set_ylim(-5, adc_peak * 1.15)
    
    plt.tight_layout()
    return fig


def create_examples():
    """Create several example visualizations with different TP characteristics"""
    
    examples = [
        {
            'name': 'typical_signal',
            'time_start': 100,
            'samples_over_threshold': 20,
            'samples_to_peak': 8,
            'adc_integral': 3000,  # Increased to give realistic peak > threshold
            'title': 'Typical Signal: Fast Rise, Slower Fall'
        },
        {
            'name': 'short_pulse',
            'time_start': 100,
            'samples_over_threshold': 10,
            'samples_to_peak': 5,
            'adc_integral': 1500,  # Increased to give peak ~300 ADC (well above 60)
            'title': 'Short Pulse: Quick Rise and Fall'
        },
        {
            'name': 'long_tail',
            'time_start': 100,
            'samples_over_threshold': 30,
            'samples_to_peak': 6,
            'adc_integral': 4500,  # Increased for realistic peak
            'title': 'Long Tail: Fast Rise, Very Slow Decay'
        },
        {
            'name': 'symmetric',
            'time_start': 100,
            'samples_over_threshold': 20,
            'samples_to_peak': 10,
            'adc_integral': 3500,  # Increased for realistic peak
            'title': 'Symmetric Shape: Peak at Center'
        },
        {
            'name': 'high_energy',
            'time_start': 100,
            'samples_over_threshold': 25,
            'samples_to_peak': 10,
            'adc_integral': 10000,  # Much higher for high energy case
            'title': 'High Energy Deposit: Large Integral'
        },
    ]
    
    output_dir = Path('output/pentagon_examples')
    output_dir.mkdir(parents=True, exist_ok=True)
    
    for example in examples:
        print(f"Generating: {example['title']}")
        
        fig = visualize_pentagon_from_tp(
            time_start=example['time_start'],
            samples_over_threshold=example['samples_over_threshold'],
            samples_to_peak=example['samples_to_peak'],
            adc_integral=example['adc_integral'],
            title=example['title']
        )
        
        output_file = output_dir / f"{example['name']}.png"
        fig.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"  Saved: {output_file}")
        plt.close(fig)
    
    print(f"\nAll examples saved to: {output_dir}")


def create_comparison_plot():
    """Create a side-by-side comparison showing the reconstruction process"""
    
    # Example TP data with realistic ADC values
    time_start = 100
    samples_over_threshold = 20
    samples_to_peak = 8
    adc_integral = 3000  # Increased to give peak ~300 ADC (well above 60 threshold)
    time_end = time_start + samples_over_threshold
    time_peak = time_start + samples_to_peak
    adc_peak = adc_integral / max(samples_over_threshold, 1) * 2
    
    params = calculate_pentagon_params(time_start, time_peak, time_end, 
                                       adc_peak, adc_integral, 60.0)  # Use X plane threshold for example
    time_int_rise, time_int_fall, h_int_rise, h_int_fall, threshold, valid = params
    
    # Generate pentagon curve
    time_samples = np.arange(time_start - 5, time_end + 5, 0.1)
    pentagon_values = []
    
    for t in time_samples:
        if t < time_start or t > time_end:
            intensity = 0.0
        elif t == time_start or t == time_end:
            intensity = threshold
        elif t < time_int_rise:
            span = time_int_rise - time_start
            frac = (t - time_start) / span if span > 0 else 0
            intensity = threshold + frac * (h_int_rise - threshold)
        elif t < time_peak:
            span = time_peak - time_int_rise
            frac = (t - time_int_rise) / span if span > 0 else 0
            intensity = h_int_rise + frac * (adc_peak - h_int_rise)
        elif t == time_peak:
            intensity = adc_peak
        elif t <= time_int_fall:
            span = time_int_fall - time_peak
            frac = (t - time_peak) / span if span > 0 else 0
            intensity = adc_peak - frac * (adc_peak - h_int_fall)
        else:
            span = time_end - time_int_fall
            frac = (t - time_int_fall) / span if span > 0 else 0
            intensity = h_int_fall - frac * (h_int_fall - threshold)
        pentagon_values.append(intensity)
    
    pentagon_values = np.array(pentagon_values)
    
    # Create comparison figure
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))
    
    # LEFT PANEL: What we know (TP data)
    ax1.set_title('INPUT: Trigger Primitive Data', fontsize=14, fontweight='bold')
    
    # Show only the discrete TP information
    ax1.axvspan(time_start, time_end, alpha=0.2, color='green', label='Signal above threshold')
    ax1.axhline(threshold, color='red', linestyle='--', linewidth=2, label='Threshold')
    ax1.axhline(0, color='black', linestyle=':', linewidth=1, alpha=0.5)
    
    # Mark what we know
    ax1.plot(time_start, threshold, 'go', markersize=15, label='Start time', zorder=5)
    ax1.plot(time_end, threshold, 'gs', markersize=15, label='End time', zorder=5)
    ax1.plot(time_peak, adc_peak, 'r*', markersize=25, label='Peak (time & amplitude)', zorder=5)
    
    # Draw question marks for unknown shape
    ax1.text(time_start + samples_over_threshold/4, adc_peak/2, '?', 
             fontsize=40, ha='center', va='center', color='gray', alpha=0.3)
    ax1.text(time_peak + samples_over_threshold/4, adc_peak/2, '?', 
             fontsize=40, ha='center', va='center', color='gray', alpha=0.3)
    
    tp_info = (
        "Known from TP:\n"
        f"✓ Start: t={time_start}\n"
        f"✓ Duration: {samples_over_threshold} ticks\n"
        f"✓ Time to peak: {samples_to_peak} ticks\n"
        f"✓ Total charge: {adc_integral:.0f}\n"
        f"✓ Peak amplitude: {adc_peak:.1f}\n"
        f"\n"
        "Unknown:\n"
        "✗ Exact waveform shape\n"
        "✗ Rise/fall profiles"
    )
    ax1.text(0.02, 0.98, tp_info, transform=ax1.transAxes, 
             fontsize=11, verticalalignment='top',
             bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.8))
    
    ax1.set_xlabel('Time [ticks]', fontsize=12)
    ax1.set_ylabel('Amplitude [ADC]', fontsize=12)
    ax1.legend(loc='upper right', fontsize=10)
    ax1.grid(True, alpha=0.3)
    ax1.set_xlim(time_start - 5, time_end + 5)
    ax1.set_ylim(-5, adc_peak * 1.15)
    
    # RIGHT PANEL: Pentagon reconstruction
    ax2.set_title('OUTPUT: Pentagon Reconstruction', fontsize=14, fontweight='bold')
    
    # Plot the full pentagon
    ax2.plot(time_samples, pentagon_values, 'b-', linewidth=3, label='Reconstructed waveform', zorder=3)
    ax2.axvspan(time_start, time_end, alpha=0.15, color='green', zorder=1)
    
    # CLEARLY SHOW THE THRESHOLD PLATEAU - thick red line
    ax2.plot([time_start, time_end], [threshold, threshold], 'r-', linewidth=5, 
            alpha=0.7, label=f'Threshold PLATEAU = {threshold:.1f} ADC', zorder=2)
    
    ax2.axhline(threshold, color='red', linestyle='--', linewidth=1.5, alpha=0.5, zorder=1)
    ax2.axhline(0, color='black', linestyle=':', linewidth=1, alpha=0.5, zorder=1)
    
    # Mark all vertices
    vertices_t = [time_start, time_int_rise, time_peak, time_int_fall, time_end]
    vertices_adc = [threshold, h_int_rise, adc_peak, h_int_fall, threshold]
    ax2.plot(vertices_t, vertices_adc, 'mo', markersize=10, label='Pentagon vertices (on plateau)', zorder=4)
    
    # Connect vertices with dashed lines to show the pentagon structure
    ax2.plot(vertices_t, vertices_adc, 'b--', linewidth=1.5, alpha=0.4, zorder=2)
    
    # Add arrows showing the construction
    arrow_props = dict(arrowstyle='->', lw=2, color='purple', alpha=0.6)
    ax2.annotate('', xy=(time_int_rise, h_int_rise), xytext=(time_start, threshold),
                 arrowprops=arrow_props)
    ax2.annotate('', xy=(time_peak, adc_peak), xytext=(time_int_rise, h_int_rise),
                 arrowprops=arrow_props)
    ax2.annotate('', xy=(time_int_fall, h_int_fall), xytext=(time_peak, adc_peak),
                 arrowprops=arrow_props)
    ax2.annotate('', xy=(time_end, threshold), xytext=(time_int_fall, h_int_fall),
                 arrowprops=arrow_props)
    
    reconstruction_info = (
        "Pentagon Algorithm:\n"
        f"1. PLATEAU (baseline)\n"
        f"   threshold = {threshold:.1f} ADC\n"
        f"   (60 for X, 70 for U/V)\n"
        f"   Pentagon SITS ON plateau!\n"
        f"\n"
        f"2. Calculate intermediate\n"
        f"   vertices:\n"
        f"   t_rise = {time_int_rise:.1f}\n"
        f"   t_fall = {time_int_fall:.1f}\n"
        f"   h = {h_int_rise:.1f} ADC\n"
        f"\n"
        f"3. Connect 5 vertices:\n"
        f"   • (start, threshold) ← ON PLATEAU\n"
        f"   • (t_rise, h)\n"
        f"   • (peak, peak_adc)\n"
        f"   • (t_fall, h)\n"
        f"   • (end, threshold) ← ON PLATEAU\n"
        f"\n"
        f"4. Interpolate linearly\n"
        f"   between vertices"
    )
    ax2.text(0.02, 0.98, reconstruction_info, transform=ax2.transAxes, 
             fontsize=10, verticalalignment='top',
             bbox=dict(boxstyle='round', facecolor='lightgreen', alpha=0.8))
    
    ax2.set_xlabel('Time [ticks]', fontsize=12)
    ax2.set_ylabel('Amplitude [ADC]', fontsize=12)
    ax2.legend(loc='upper right', fontsize=10)
    ax2.grid(True, alpha=0.3)
    ax2.set_xlim(time_start - 5, time_end + 5)
    ax2.set_ylim(-5, adc_peak * 1.15)
    
    plt.tight_layout()
    
    output_dir = Path('output/pentagon_examples')
    output_dir.mkdir(parents=True, exist_ok=True)
    output_file = output_dir / 'pentagon_comparison.png'
    fig.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"Comparison plot saved: {output_file}")
    plt.close(fig)


if __name__ == '__main__':
    print("Generating pentagon construction visualizations...")
    print("=" * 60)
    
    # Create comparison showing input vs output
    print("\n1. Creating comparison plot (TP data → Pentagon reconstruction)")
    create_comparison_plot()
    
    # Create various examples
    print("\n2. Creating example pentagon constructions")
    create_examples()
    
    print("\n" + "=" * 60)
    print("Done! Check output/pentagon_examples/ for images")
