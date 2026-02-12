#!/usr/bin/env python3
"""
Test script to show pentagon construction without offset parameter
"""

import sys
sys.path.insert(0, '/home/virgolaema/dune/online-pointing-utils/scripts')

from generate_cluster_arrays import calculate_pentagon_params
import numpy as np

print("=" * 70)
print("PENTAGON ALGORITHM TEST - WITHOUT OFFSET PARAMETER")
print("=" * 70)
print()

# Test case 1: Typical signal
print("Test 1: Typical Signal")
print("-" * 70)
time_start = 0
time_peak = 50
time_end = 100
adc_peak = 200
adc_integral = 5000

params = calculate_pentagon_params(time_start, time_peak, time_end, adc_peak, adc_integral)
time_int_rise, time_int_fall, h_int_rise, h_int_fall, threshold, valid = params

print(f"Input:")
print(f"  Time range: {time_start} → {time_peak} (peak) → {time_end}")
print(f"  ADC peak: {adc_peak}")
print(f"  ADC integral: {adc_integral}")
print()
print(f"Pentagon vertices:")
print(f"  1. Start point:     ({time_start}, {threshold:.4f}) ← threshold baseline")
print(f"  2. Rise vertex:     ({time_int_rise:.2f}, {h_int_rise:.2f})")
print(f"  3. Peak:            ({time_peak}, {adc_peak})")
print(f"  4. Fall vertex:     ({time_int_fall:.2f}, {h_int_fall:.2f})")
print(f"  5. End point:       ({time_end}, {threshold:.4f}) ← threshold baseline")
print(f"  Valid: {valid}")
print()

# Test case 2: Fast pulse
print("Test 2: Fast Pulse (Short Duration)")
print("-" * 70)
time_start = 0
time_peak = 3
time_end = 10
adc_peak = 300
adc_integral = 800

params = calculate_pentagon_params(time_start, time_peak, time_end, adc_peak, adc_integral)
time_int_rise, time_int_fall, h_int_rise, h_int_fall, threshold, valid = params

print(f"Input:")
print(f"  Time range: {time_start} → {time_peak} (peak) → {time_end}")
print(f"  ADC peak: {adc_peak}")
print(f"  ADC integral: {adc_integral}")
print()
print(f"Pentagon vertices:")
print(f"  1. Start point:     ({time_start}, {threshold:.4f}) ← threshold baseline")
print(f"  2. Rise vertex:     ({time_int_rise:.2f}, {h_int_rise:.2f})")
print(f"  3. Peak:            ({time_peak}, {adc_peak})")
print(f"  4. Fall vertex:     ({time_int_fall:.2f}, {h_int_fall:.2f})")
print(f"  5. End point:       ({time_end}, {threshold:.4f}) ← threshold baseline")
print(f"  Valid: {valid}")
print()

# Test case 3: Long tail
print("Test 3: Long Tail Signal")
print("-" * 70)
time_start = 0
time_peak = 10
time_end = 80
adc_peak = 250
adc_integral = 6000

params = calculate_pentagon_params(time_start, time_peak, time_end, adc_peak, adc_integral)
time_int_rise, time_int_fall, h_int_rise, h_int_fall, threshold, valid = params

print(f"Input:")
print(f"  Time range: {time_start} → {time_peak} (peak) → {time_end}")
print(f"  ADC peak: {adc_peak}")
print(f"  ADC integral: {adc_integral}")
print()
print(f"Pentagon vertices:")
print(f"  1. Start point:     ({time_start}, {threshold:.4f}) ← threshold baseline")
print(f"  2. Rise vertex:     ({time_int_rise:.2f}, {h_int_rise:.2f})")
print(f"  3. Peak:            ({time_peak}, {adc_peak})")
print(f"  4. Fall vertex:     ({time_int_fall:.2f}, {h_int_fall:.2f})")
print(f"  5. End point:       ({time_end}, {threshold:.4f}) ← threshold baseline")
print(f"  Valid: {valid}")
print()

print("=" * 70)
print("KEY POINTS:")
print("=" * 70)
print("1. Threshold baseline = 0.5 / (time_end - time_start)")
print("   - Very small value, adaptive to signal duration")
print("   - NOT zero (avoids artifacts at endpoints)")
print("   - NOT based on configurable offset")
print()
print("2. Pentagon has 5 vertices forming a smooth waveform shape")
print("   - Endpoints at threshold (not zero)")
print("   - Intermediate vertices at 60% of peak height")
print("   - Linear interpolation between vertices")
print()
print("3. Algorithm is now simpler and more consistent")
print("   - No external offset parameter needed")
print("   - Same behavior for all signals")
print("=" * 70)
