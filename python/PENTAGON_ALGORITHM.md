# Pentagon Drawing Algorithm with Plateau Baseline

## Overview

The pentagon algorithm creates a 5-vertex polygon to approximate the shape of trigger primitives (TPs) with a plateau rectangular baseline at the threshold level.

## Algorithm Description

### Key Concept

The TP waveform is modeled as:
1. **Rectangular plateau** (baseline): From ADC = 0 to ADC = threshold, spanning the entire time_over_threshold
2. **Pentagon shape** (signal): Sitting on top of the plateau, from threshold to peak

This separation allows for cleaner optimization by excluding the constant baseline offset from the minimization.

### Steps

1. **Extract TP attributes**:
   - `time_start`: Absolute start time
   - `time_peak`: Time of peak amplitude (time_start + samples_to_peak)
   - `time_end`: End time (time_start + samples_over_threshold)
   - `adc_peak`: Peak ADC value (measured from baseline = 0)
   - `adc_integral`: Total ADC integral (target area)
   - `threshold`: Plane-specific threshold (from parameters)

2. **Calculate fixed intermediate times** (at 0.5 distance):
   ```python
   t1 = time_start + 0.5 * (time_peak - time_start)
   t2 = time_peak + 0.5 * (time_end - time_peak)
   ```

3. **Calculate plateau offset area**:
   ```python
   offset_area = threshold * (time_end - time_start)
   ```

4. **Optimize intermediate height** `h`:
   - Define pentagon vertices as:
     ```
     V1: (time_start, threshold)
     V2: (t1, threshold + h)
     V3: (time_peak, adc_peak)
     V4: (t2, threshold + h)
     V5: (time_end, threshold)
     ```
   - Minimize: `|pentagon_area + offset_area - adc_integral|`
   - Constraint: `0 ≤ h ≤ (adc_peak - threshold)`
   - Method: `scipy.optimize.minimize_scalar` with bounded search

5. **Return optimized parameters**:
   - `time_int_rise`, `time_int_fall`: Intermediate time points (t1, t2)
   - `h_int_rise`, `h_int_fall`: Intermediate heights (threshold + h_opt)
   - `h_opt`: Optimized height above threshold
   - `threshold`: Plane threshold value
   - `offset_area`: Rectangular baseline area

## Visualization

```
adc_peak  ──────────── ● (time_peak, adc_peak)
                      /│\
                     / │ \
threshold + h  ─────●  │  ●───── (t1, t2: intermediate points)
                   /   │   \
threshold  ───────●────┼────●─── (Plateau baseline)
                  │    │    │
              time_start │  time_end
                      time_peak
```

The shaded area below the threshold line (rectangular plateau) is the `offset_area`.
The pentagon vertices V1-V5 define the signal shape above the threshold.

## Differences from Previous Algorithm

### Old Algorithm (Grid Search)
- Used 10×10 grid search to find t1, t2, y1, y2
- Calculated threshold dynamically from offset: `threshold = offset / time_over_threshold`
- Searched entire 2D space for intermediate points
- Target area included offset: `target_area = adc_integral + offset`

### New Algorithm (Plateau + Optimization)
- Fixed intermediate times at 0.5 distance (no search needed)
- Uses explicit plane-specific threshold from parameters
- Optimizes only one variable: height `h` above threshold
- Separates baseline from signal: `total = pentagon_above_threshold + offset_area`
- More physically motivated (matches TP finder behavior)
- Faster convergence with scipy's bounded scalar minimization

## Plane-Specific Thresholds

Thresholds are loaded from `parameters/display.dat`:
- **X plane (collection)**: `display.threshold_adc_x = 60` ADC counts
- **U plane (induction)**: `display.threshold_adc_u = 70` ADC counts  
- **V plane (induction)**: `display.threshold_adc_v = 70` ADC counts

## Implementation Notes

### C++ Version
The C++ code retrieves thresholds directly from parameters:
```cpp
double threshold_adc = 60.0;
if (plane == "U") threshold_adc = params.get("display.threshold_adc_u", 70.0);
if (plane == "V") threshold_adc = params.get("display.threshold_adc_v", 70.0);
if (plane == "X") threshold_adc = params.get("display.threshold_adc_x", 60.0);
```

### Python Version
The Python port follows the same pattern:
```python
threshold_adc = 60.0
if item.plane == 'U':
    threshold_adc = self.params.get('display.threshold_adc_u', 70.0)
elif item.plane == 'V':
    threshold_adc = self.params.get('display.threshold_adc_v', 70.0)
elif item.plane == 'X':
    threshold_adc = self.params.get('display.threshold_adc_x', 60.0)
```

## Dependencies

The new algorithm requires `scipy` for optimization:
```bash
pip install scipy>=1.7.0
```

Added to `requirements.txt`:
```
scipy>=1.7.0
```

## Example Usage

```python
from cluster_display import DrawingAlgorithms

# Calculate pentagon parameters
params = DrawingAlgorithms.calculate_pentagon_params(
    time_start=100.0,
    time_peak=105.0, 
    time_end=110.0,
    adc_peak=200.0,
    adc_integral=800.0,
    threshold=60.0  # X-plane threshold
)

print(f"Optimized height above threshold: {params['h_opt']:.2f}")
print(f"Intermediate times: t1={params['time_int_rise']:.2f}, t2={params['time_int_fall']:.2f}")
print(f"Plateau offset area: {params['offset_area']:.2f}")
```

## Performance

The new algorithm is **faster** than the old grid search:
- Old: O(n²) - 10×10 = 100 area calculations
- New: O(log n) - bounded scalar optimization typically converges in < 20 iterations

## Validation

Unit tests verify:
1. Polygon area calculation (Shoelace formula)
2. Pentagon parameter optimization with various test cases
3. Area matching within numerical tolerance
4. Edge cases (low threshold, high peak, etc.)

Run tests with:
```bash
python3 python/test_cluster_display.py
```
