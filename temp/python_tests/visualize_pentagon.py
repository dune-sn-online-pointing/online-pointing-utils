#!/usr/bin/env python3
"""
Visualization script to verify pentagon shape calculations.
Pentagon vertices go to baseline (0), but sides pass through threshold points.
"""
import numpy as np
import matplotlib.pyplot as plt

def calculate_pentagon_params(time_start, time_peak, time_end, adc_peak, adc_integral, frac, offset):
    """
    Calculate pentagon parameters matching the C++ implementation.
    Returns: (time_int_rise, h_int_rise, time_int_fall, h_int_fall, valid)
    """
    # Total time over threshold
    time_over_threshold = time_end - time_start
    if time_over_threshold <= 0:
        return None, None, None, None, False
    
    # Time to peak and from peak to end
    dh = time_peak - time_start  # rise time
    he = time_end - time_peak    # fall time
    
    if dh <= 0 or he <= 0:
        return None, None, None, None, False
    
    # Target area
    target_area = adc_integral + offset
    
    # Calculate channel height (intermediate height)
    ch = adc_peak - 0  # peak relative to baseline (0)
    
    # Pentagon area with vertices at baseline:
    # Total Area = 0.5*h*(time_end - time_start) + 0.5*peak*(t_int_fall - t_int_rise)
    # where t_int_fall - t_int_rise = dh*(1-frac) + frac*he
    # Solving for h:
    # h = (2*target_area - peak*(dh*(1-frac) + frac*he)) / time_over_threshold
    
    ad = frac * dh
    eb = frac * he
    peak_width = dh * (1.0 - frac) + frac * he
    numerator = 2 * target_area - ch * peak_width
    intermediate_height = numerator / time_over_threshold
    
    # Debug output
    print(f"  Debug: time_start={time_start}, time_peak={time_peak}, time_end={time_end}")
    print(f"  Debug: dh={dh}, he={he}, ch={ch}, frac={frac}")
    print(f"  Debug: peak_width={peak_width}, target_area={target_area}")
    print(f"  Debug: numerator={numerator}, intermediate_height={intermediate_height}")
    
    # Check validity
    if intermediate_height < 0 or intermediate_height > adc_peak:
        return None, None, None, None, False
    
    # Calculate intermediate time positions
    time_int_rise = time_start + frac * dh
    time_int_fall = time_peak + frac * he
    
    return time_int_rise, intermediate_height, time_int_fall, intermediate_height, True


def find_threshold_crossing(x1, y1, x2, y2, threshold):
    """Find where a line from (x1,y1) to (x2,y2) crosses y=threshold."""
    if y1 == y2:
        return None
    t = (threshold - y1) / (y2 - y1)
    if 0 <= t <= 1:
        return x1 + t * (x2 - x1)
    return None


def draw_pentagon(ax, time_start, time_peak, time_end, adc_peak, adc_integral, threshold, offset=120, label_suffix=""):
    """Draw pentagon shape with vertices at baseline (0)."""
    frac = 0.5  # Initial guess
    
    # Calculate pentagon parameters
    t_int_rise, h_int_rise, t_int_fall, h_int_fall, valid = calculate_pentagon_params(
        time_start, time_peak, time_end, adc_peak, adc_integral, frac, offset
    )
    
    if not valid:
        print(f"Pentagon calculation failed for {label_suffix}")
        return None
    
    # Create pentagon vertices (baseline at 0)
    times = [time_start, t_int_rise, time_peak, t_int_fall, time_end, time_start]
    amplitudes = [0, h_int_rise, adc_peak, h_int_fall, 0, 0]
    
    # Plot pentagon
    color = ax.plot(times, amplitudes, 'o-', linewidth=2, label=f'Pentagon {label_suffix}', markersize=8)[0].get_color()
    
    # Find and mark threshold crossing points
    # Rising edge: from (time_start, 0) to (t_int_rise, h_int_rise)
    t_cross_rise = find_threshold_crossing(time_start, 0, t_int_rise, h_int_rise, threshold)
    if t_cross_rise:
        ax.plot(t_cross_rise, threshold, 'r*', markersize=15, markeredgecolor='black', 
                markeredgewidth=1.5, zorder=10)
        ax.text(t_cross_rise, threshold + 10, f't={t_cross_rise:.1f}', 
                fontsize=9, ha='center', color='red', fontweight='bold')
    
    # Falling edge: from (t_int_fall, h_int_fall) to (time_end, 0)
    t_cross_fall = find_threshold_crossing(t_int_fall, h_int_fall, time_end, 0, threshold)
    if t_cross_fall:
        ax.plot(t_cross_fall, threshold, 'r*', markersize=15, markeredgecolor='black',
                markeredgewidth=1.5, zorder=10)
        ax.text(t_cross_fall, threshold + 10, f't={t_cross_fall:.1f}', 
                fontsize=9, ha='center', color='red', fontweight='bold')
    
    # Calculate actual area
    actual_area = 0.5 * h_int_rise * (t_int_rise - time_start)  # Triangle 1
    actual_area += 0.5 * (h_int_rise + adc_peak) * (time_peak - t_int_rise)  # Trapezoid 1
    actual_area += 0.5 * (adc_peak + h_int_fall) * (t_int_fall - time_peak)  # Trapezoid 2
    actual_area += 0.5 * h_int_fall * (time_end - t_int_fall)  # Triangle 2
    
    target_area = adc_integral + offset
    
    print(f"\nPentagon {label_suffix}:")
    print(f"  Intermediate height: {h_int_rise:.2f}")
    print(f"  Target area: {target_area:.2f}")
    print(f"  Actual area: {actual_area:.2f}")
    print(f"  Error: {abs(actual_area - target_area):.2f}")
    if t_cross_rise:
        print(f"  Rising threshold crossing at t={t_cross_rise:.2f}")
    if t_cross_fall:
        print(f"  Falling threshold crossing at t={t_cross_fall:.2f}")
    
    return color


def draw_triangle(ax, time_start, time_peak, time_end, adc_peak, threshold, label_suffix=""):
    """Draw triangle shape."""
    times = [time_start, time_peak, time_end, time_start]
    amplitudes = [threshold, adc_peak, threshold, threshold]
    
    line = ax.plot(times, amplitudes, 's--', linewidth=2, alpha=0.5, 
                   label=f'Triangle {label_suffix}', markersize=6)
    
    # Calculate triangle area
    base = time_end - time_start
    height = adc_peak - threshold
    triangle_area = 0.5 * base * height
    
    print(f"\nTriangle {label_suffix}:")
    print(f"  Area: {triangle_area:.2f}")
    
    return line


# Test cases
fig, axes = plt.subplots(2, 2, figsize=(16, 11))
fig.suptitle('Pentagon Drawing Mode: Vertices at Baseline, Sides Pass Through Threshold', fontsize=16, fontweight='bold')

# Test case 1: X plane (threshold = 60)
ax = axes[0, 0]
time_start, time_peak, time_end = 100, 120, 180
adc_peak = 250
adc_integral = 12000  # Increased to make pentagon feasible
threshold_x = 60

print("="*60)
print("TEST CASE 1: X Plane")
draw_pentagon(ax, time_start, time_peak, time_end, adc_peak, adc_integral, threshold_x, offset=120, label_suffix="X")
draw_triangle(ax, time_start, time_peak, time_end, adc_peak, threshold_x, label_suffix="X")
ax.axhline(y=threshold_x, color='r', linestyle=':', alpha=0.7, linewidth=2, label=f'Threshold X = {threshold_x}')
ax.axhline(y=0, color='k', linestyle='-', alpha=0.5, linewidth=1.5, label='Baseline')
ax.fill_between([time_start, time_end], 0, 0, alpha=0.1)
ax.set_xlabel('Time (ticks)', fontsize=11)
ax.set_ylabel('ADC', fontsize=11)
ax.set_title(f'X Plane (threshold={threshold_x})', fontsize=12, fontweight='bold')
ax.legend(loc='upper right', fontsize=9)
ax.grid(True, alpha=0.3)
ax.set_ylim(-20, adc_peak + 30)

# Test case 2: U plane (threshold = 70)
ax = axes[0, 1]
time_start, time_peak, time_end = 100, 115, 160
adc_peak = 200
adc_integral = 8000  # Increased
threshold_u = 70

print("="*60)
print("TEST CASE 2: U Plane")
draw_pentagon(ax, time_start, time_peak, time_end, adc_peak, adc_integral, threshold_u, offset=120, label_suffix="U")
draw_triangle(ax, time_start, time_peak, time_end, adc_peak, threshold_u, label_suffix="U")
ax.axhline(y=threshold_u, color='r', linestyle=':', alpha=0.7, linewidth=2, label=f'Threshold U = {threshold_u}')
ax.axhline(y=0, color='k', linestyle='-', alpha=0.5, linewidth=1.5, label='Baseline')
ax.fill_between([time_start, time_end], 0, 0, alpha=0.1)
ax.set_xlabel('Time (ticks)', fontsize=11)
ax.set_ylabel('ADC', fontsize=11)
ax.set_title(f'U Plane (threshold={threshold_u})', fontsize=12, fontweight='bold')
ax.legend(loc='upper right', fontsize=9)
ax.grid(True, alpha=0.3)
ax.set_ylim(-20, adc_peak + 30)

# Test case 3: V plane (threshold = 70)
ax = axes[1, 0]
time_start, time_peak, time_end = 50, 80, 140
adc_peak = 300
adc_integral = 18000  # Increased
threshold_v = 70

print("="*60)
print("TEST CASE 3: V Plane")
draw_pentagon(ax, time_start, time_peak, time_end, adc_peak, adc_integral, threshold_v, offset=120, label_suffix="V")
draw_triangle(ax, time_start, time_peak, time_end, adc_peak, threshold_v, label_suffix="V")
ax.axhline(y=threshold_v, color='r', linestyle=':', alpha=0.7, linewidth=2, label=f'Threshold V = {threshold_v}')
ax.axhline(y=0, color='k', linestyle='-', alpha=0.5, linewidth=1.5, label='Baseline')
ax.fill_between([time_start, time_end], 0, 0, alpha=0.1)
ax.set_xlabel('Time (ticks)', fontsize=11)
ax.set_ylabel('ADC', fontsize=11)
ax.set_title(f'V Plane (threshold={threshold_v})', fontsize=12, fontweight='bold')
ax.legend(loc='upper right', fontsize=9)
ax.grid(True, alpha=0.3)
ax.set_ylim(-20, adc_peak + 30)

# Test case 4: Asymmetric peak (early peak)
ax = axes[1, 1]
time_start, time_peak, time_end = 100, 110, 180
adc_peak = 280
adc_integral = 15000  # Increased
threshold_x = 60

print("="*60)
print("TEST CASE 4: Asymmetric (early peak)")
draw_pentagon(ax, time_start, time_peak, time_end, adc_peak, adc_integral, threshold_x, offset=120, label_suffix="asym")
draw_triangle(ax, time_start, time_peak, time_end, adc_peak, threshold_x, label_suffix="asym")
ax.axhline(y=threshold_x, color='r', linestyle=':', alpha=0.7, linewidth=2, label=f'Threshold = {threshold_x}')
ax.axhline(y=0, color='k', linestyle='-', alpha=0.5, linewidth=1.5, label='Baseline')
ax.fill_between([time_start, time_end], 0, 0, alpha=0.1)
ax.set_xlabel('Time (ticks)', fontsize=11)
ax.set_ylabel('ADC', fontsize=11)
ax.set_title('Asymmetric Case (early peak)', fontsize=12, fontweight='bold')
ax.legend(loc='upper right', fontsize=9)
ax.grid(True, alpha=0.3)
ax.set_ylim(-20, adc_peak + 30)

plt.tight_layout()
plt.savefig('pentagon_vs_triangle.png', dpi=150, bbox_inches='tight')
print("\n" + "="*60)
print("Plot saved to pentagon_vs_triangle.png")
print("="*60)
plt.show()
