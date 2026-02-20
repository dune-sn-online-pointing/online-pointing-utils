# Display Enhancement: Physical Dimension Axes

## Overview

Added secondary X and Y axes to the cluster display showing physical dimensions in centimeters, making it easier to understand the actual spatial and temporal scales of the clusters.

## Changes Made

### 1. Updated Display Code

**File**: `src/app/cluster_display.cpp`

Added code to create and display secondary axes after drawing the main histogram:

```cpp
// Get conversion parameters
double wire_pitch_cm = GET_PARAM_DOUBLE("geometry.wire_pitch_collection_cm");
if (it.plane == "U" || it.plane == "V") {
  wire_pitch_cm = GET_PARAM_DOUBLE("geometry.wire_pitch_induction_diagonal_cm");
}
double time_tick_cm = GET_PARAM_DOUBLE("timing.time_tick_cm");

// Calculate physical ranges
double xmin_cm = xmin * wire_pitch_cm;
double xmax_cm = xmax * wire_pitch_cm;
double ymin_cm = ymin * time_tick_cm;
double ymax_cm = ymax * time_tick_cm;

// Create secondary X axis (top) for channel dimension in cm
TGaxis *axis_x_cm = new TGaxis(xmin, ymax, xmax, ymax, xmin_cm, xmax_cm, 510, "-L");
axis_x_cm->SetTitle("channel [cm]");
// ... styling ...
axis_x_cm->Draw();

// Create secondary Y axis (right) for time dimension in cm
TGaxis *axis_y_cm = new TGaxis(xmax, ymin, xmax, ymax, ymin_cm, ymax_cm, 510, "+L");
axis_y_cm->SetTitle("drift distance [cm]");
// ... styling ...
axis_y_cm->Draw();
```

## Display Features

### Primary Axes (Bottom and Left)
- **X-axis (bottom)**: Channel number (contiguous indexing)
- **Y-axis (left)**: Time in TPC ticks

### Secondary Axes (Top and Right)
- **X-axis (top)**: Physical channel position in cm
- **Y-axis (right)**: Drift distance in cm

## Physical Conversions Used

### Channel to Position (X-axis)
- **Collection plane (X)**: `wire_pitch_collection_cm = 0.479 cm`
- **Induction planes (U/V)**: `wire_pitch_induction_diagonal_cm = 0.4669 cm`
- Formula: `position_cm = channel * wire_pitch_cm`

### Time to Drift Distance (Y-axis)
- **Time tick**: `time_tick_cm = 0.0805 cm`
- Formula: `drift_distance_cm = time_ticks * time_tick_cm`

## Benefits

1. **Physical Context**: Users can immediately see the actual physical size of clusters in the detector
2. **Spatial Understanding**: Easier to relate cluster dimensions to detector geometry
3. **Energy Deposition**: Can better estimate energy deposition density per unit volume
4. **Cross-Referencing**: Easier to compare with geometry specifications and other measurements

## Example Interpretation

For a cluster spanning:
- 10 channels × 100 ticks in units
- Translates to approximately:
  - X: 10 × 0.479 cm ≈ **4.8 cm** across wires
  - Y: 100 × 0.0805 cm ≈ **8.05 cm** drift distance

## Notes
- TGaxis is already included in the ROOT headers via `src/lib/root.h`
- The secondary axes automatically scale with zoom and pan operations
- Axes labels are centered and sized for readability

