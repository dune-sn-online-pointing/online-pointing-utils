#!/usr/bin/env python3
import matplotlib
matplotlib.use('Agg')  # Use non-interactive backend
import matplotlib.pyplot as plt

# Test calculation
time_start, time_peak, time_end = 100, 120, 180
adc_peak = 250
adc_integral = 5000
offset = 120
frac = 0.5

time_over_threshold = time_end - time_start
dh = time_peak - time_start
he = time_end - time_peak
ch = adc_peak

target_area = adc_integral + offset
numerator = 2 * target_area - ch * (dh + he)
intermediate_height = numerator / time_over_threshold

print(f"time_over_threshold = {time_over_threshold}")
print(f"dh = {dh}, he = {he}")
print(f"ch (adc_peak) = {ch}")
print(f"target_area = {target_area}")
print(f"numerator = 2 * {target_area} - {ch} * ({dh} + {he}) = {numerator}")
print(f"intermediate_height = {numerator} / {time_over_threshold} = {intermediate_height}")
print(f"Valid? {0 <= intermediate_height <= adc_peak}")
