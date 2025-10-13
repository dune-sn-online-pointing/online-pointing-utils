#!/usr/bin/env python3
"""
Test script to validate pentagon algorithm.
Compares the improved scanning approach with the old parametric approach.
"""

import numpy as np
import matplotlib.pyplot as plt
from typing import Dict, Optional


def polygon_area(vertices):
    """Calculate polygon area using Shoelace formula."""
    x = vertices[:, 0]
    y = vertices[:, 1]
    return 0.5 * np.abs(np.dot(x, np.roll(y, -1)) - np.dot(y, np.roll(x, -1)))


def find_pentagon_vertices(waveform, time_start, time_peak, time_end, threshold):
    """
    Scan candidate vertices near the peak and pick the pentagon 
    whose area matches the waveform integral.
    
    This is the improved algorithm from the user.
    """
    peak_amplitude = waveform[time_peak]
    target_x = np.arange(time_start, time_end + 1)
    target_y = waveform[time_start:time_end + 1]
    target_area = np.trapz(target_y, target_x)

    best = None
    for t1 in range(time_start + 1, time_peak):
        y1 = waveform[t1]
        if y1 <= threshold:
            continue
        slope_left = (y1 - threshold) / (t1 - time_start)
        if slope_left <= 0:
            continue
        t_left = time_start - threshold / slope_left

        for t2 in range(time_peak + 1, time_end):
            y2 = waveform[t2]
            if y2 <= threshold:
                continue
            slope_right = (y2 - threshold) / (t2 - time_end)
            if slope_right >= 0:
                continue
            t_right = time_end - threshold / slope_right
            if not (t_left < time_start and t_right > time_end):
                continue

            vertices = np.array([
                [t_left, 0.0],
                [t1, y1],
                [time_peak, peak_amplitude],
                [t2, y2],
                [t_right, 0.0]
            ])
            area = polygon_area(vertices)
            diff = abs(area - target_area)
            if best is None or diff < best["diff"]:
                best = {
                    "vertices": vertices,
                    "area": area,
                    "diff": diff,
                    "target_area": target_area,
                    "adc_integral_above_threshold": np.trapz(
                        np.maximum(target_y - threshold, 0), target_x
                    ),
                    "t1": t1,
                    "t2": t2,
                    "y1": y1,
                    "y2": y2,
                }
    return best


def old_parametric_approach(time_start, time_peak, time_end, adc_peak, adc_integral, threshold):
    """
    Old parametric approach using fixed fraction parameter.
    """
    frac = 0.5
    offset = threshold * (time_end - time_start)
    
    # Calculate time positions
    ah = time_peak - time_start
    bh = time_end - time_peak
    
    ad = ah * frac
    eb = bh * frac
    
    t_int_rise = time_start + ad
    t_int_fall = time_peak + eb
    
    # Calculate intermediate height
    time_over_threshold = time_end - time_start
    target_area = adc_integral + offset
    
    dh = ah - ad
    he = bh - eb
    peak_width = dh * (1.0 - frac) + frac * he
    intermediate_height = (2 * target_area - adc_peak * peak_width) / time_over_threshold
    
    return {
        "t1": int(t_int_rise),
        "t2": int(t_int_fall),
        "y1": intermediate_height,
        "y2": intermediate_height,
        "vertices": np.array([
            [time_start, threshold],
            [t_int_rise, intermediate_height],
            [time_peak, adc_peak],
            [t_int_fall, intermediate_height],
            [time_end, threshold]
        ]),
        "area": polygon_area(np.array([
            [time_start, threshold],
            [t_int_rise, intermediate_height],
            [time_peak, adc_peak],
            [t_int_fall, intermediate_height],
            [time_end, threshold]
        ]))
    }


def generate_test_waveform(time_start, time_peak, time_end, adc_peak, threshold):
    """Generate a synthetic waveform for testing."""
    length = time_end - time_start + 1
    waveform = np.zeros(time_end + 1)
    
    # Rising edge
    for t in range(time_start, time_peak + 1):
        frac = (t - time_start) / (time_peak - time_start)
        waveform[t] = threshold + frac * (adc_peak - threshold)
    
    # Falling edge
    for t in range(time_peak + 1, time_end + 1):
        frac = (t - time_peak) / (time_end - time_peak)
        waveform[t] = adc_peak - frac * (adc_peak - threshold)
    
    return waveform


def plot_comparison(waveform, time_start, time_peak, time_end, threshold, 
                   improved_result, old_result):
    """Plot comparison of the two approaches."""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    
    # Plot improved approach
    ax1.plot(range(len(waveform)), waveform, 'k-', alpha=0.3, label='Original waveform')
    ax1.axhline(threshold, color='gray', linestyle='--', alpha=0.5, label='Threshold')
    
    if improved_result:
        verts = improved_result['vertices']
        ax1.plot(verts[:, 0], verts[:, 1], 'r-', linewidth=2, label='Improved pentagon')
        ax1.fill(verts[:, 0], verts[:, 1], 'r', alpha=0.2)
        ax1.set_title(f"Improved (scan-based)\nArea diff: {improved_result['diff']:.2f}")
    else:
        ax1.set_title("Improved (scan-based)\nFailed to find solution")
    
    ax1.set_xlabel('Time [ticks]')
    ax1.set_ylabel('ADC')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Plot old approach
    ax2.plot(range(len(waveform)), waveform, 'k-', alpha=0.3, label='Original waveform')
    ax2.axhline(threshold, color='gray', linestyle='--', alpha=0.5, label='Threshold')
    
    verts = old_result['vertices']
    ax2.plot(verts[:, 0], verts[:, 1], 'b-', linewidth=2, label='Old pentagon')
    ax2.fill(verts[:, 0], verts[:, 1], 'b', alpha=0.2)
    
    target_area = np.trapz(waveform[time_start:time_end+1], np.arange(time_start, time_end+1))
    ax2.set_title(f"Old (parametric)\nArea diff: {abs(old_result['area'] - target_area):.2f}")
    ax2.set_xlabel('Time [ticks]')
    ax2.set_ylabel('ADC')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    return fig


def run_test_case(time_start, time_peak, time_end, adc_peak, threshold):
    """Run a single test case comparing both approaches."""
    print(f"\n{'='*60}")
    print(f"Test case: start={time_start}, peak={time_peak}, end={time_end}")
    print(f"           adc_peak={adc_peak}, threshold={threshold}")
    print(f"{'='*60}")
    
    # Generate waveform
    waveform = generate_test_waveform(time_start, time_peak, time_end, adc_peak, threshold)
    
    # Calculate target integral
    target_x = np.arange(time_start, time_end + 1)
    target_y = waveform[time_start:time_end + 1]
    adc_integral = np.trapz(target_y, target_x)
    print(f"Target waveform integral: {adc_integral:.2f}")
    
    # Test improved approach
    improved_result = find_pentagon_vertices(waveform, time_start, time_peak, time_end, threshold)
    
    if improved_result:
        print(f"\nImproved approach:")
        print(f"  t1={improved_result['t1']}, y1={improved_result['y1']:.2f}")
        print(f"  t2={improved_result['t2']}, y2={improved_result['y2']:.2f}")
        print(f"  Pentagon area: {improved_result['area']:.2f}")
        print(f"  Area difference: {improved_result['diff']:.2f}")
        print(f"  Relative error: {100.0 * improved_result['diff'] / adc_integral:.2f}%")
    else:
        print("\nImproved approach: Failed to find solution")
    
    # Test old approach
    old_result = old_parametric_approach(time_start, time_peak, time_end, adc_peak, adc_integral, threshold)
    print(f"\nOld approach:")
    print(f"  t1={old_result['t1']}, y1={old_result['y1']:.2f}")
    print(f"  t2={old_result['t2']}, y2={old_result['y2']:.2f}")
    print(f"  Pentagon area: {old_result['area']:.2f}")
    print(f"  Area difference: {abs(old_result['area'] - adc_integral):.2f}")
    print(f"  Relative error: {100.0 * abs(old_result['area'] - adc_integral) / adc_integral:.2f}%")
    
    # Plot comparison
    fig = plot_comparison(waveform, time_start, time_peak, time_end, threshold, 
                         improved_result, old_result)
    
    return fig, improved_result, old_result


if __name__ == "__main__":
    # Test cases similar to real TP data
    test_cases = [
        # (time_start, time_peak, time_end, adc_peak, threshold)
        (1369, 1372, 1375, 200.0, 60.0),  # Short TP (6 ticks)
        (1389, 1392, 1395, 250.0, 60.0),  # Another short TP
        (1376, 1380, 1384, 180.0, 60.0),  # 8 ticks
        (100, 110, 120, 300.0, 60.0),     # Longer, symmetric
        (100, 105, 120, 300.0, 60.0),     # Asymmetric (early peak)
        (100, 115, 120, 300.0, 60.0),     # Asymmetric (late peak)
    ]
    
    for i, (ts, tp, te, peak, thresh) in enumerate(test_cases):
        fig, improved, old = run_test_case(ts, tp, te, peak, thresh)
        plt.savefig(f'/tmp/pentagon_test_case_{i+1}.png', dpi=150, bbox_inches='tight')
        print(f"Saved plot to /tmp/pentagon_test_case_{i+1}.png")
    
    print(f"\n{'='*60}")
    print("All test cases completed!")
    print(f"{'='*60}")
