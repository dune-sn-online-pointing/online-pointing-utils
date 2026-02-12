# Background Cluster Energy Issue - Root Cause Analysis

## Problem
Background clusters were showing unrealistically high energy values (up to 200+ MeV) in the analysis plots, when they should be much lower (typically < 10 MeV).

## Root Cause
The issue was in the `add_backgrounds.cpp` pipeline:

1. **SimIDE Energy Accumulation**: When background TPs are copied from background files and merged with signal events, they retain their original `simide_energy` values.

2. **Cluster Energy Calculation**: The `Cluster::update_cluster_info()` function calculates `total_energy_` by summing up `tp->GetSimideEnergy()` for all TPs in the cluster:
   ```cpp
   total_energy_ += static_cast<float>(tp->GetSimideEnergy());
   ```

3. **The Problem**: Background TPs from different background events are being merged into the same signal event. Each background TP carries its SimIDE energy from its original context, and these energies get summed up in the cluster, leading to artificially inflated energy values.

## Why This Happens
- Background files are cycled through and merged with signal events
- Multiple background events can contribute TPs to the same signal event
- Each background TP has a SimIDE energy value from its original event
- When clustering happens, all these SimIDE energies are summed, even though they come from different physics contexts

## Solution
Reset the SimIDE energy to 0 for background TPs when they are copied in `add_backgrounds.cpp`:

```cpp
tp.SetSimideEnergy(0.0);
```

This ensures that:
1. Background TPs don't contribute to the `total_energy` field
2. Only signal (Marley) TPs contribute true energy
3. Background TPs still contribute to ADC integrals and charge measurements
4. The energy plots accurately reflect only signal energy

## Alternative Approaches Considered
1. **Use ADC-based energy instead**: Would require changing energy calculation throughout the pipeline
2. **Filter by generator in energy calc**: Would add complexity to Cluster class
3. **Current solution**: Clean, simple, and preserves the meaning of `total_energy` as "true deposited energy from signal"

## Files Modified
- `src/app/add_backgrounds.cpp`: Added `tp.SetSimideEnergy(0.0)` after copying background TPs

## Impact
- Background clusters will now show realistic energy values (near 0, as expected)
- Signal clusters retain accurate energy measurements
- Mixed clusters will only count energy from signal TPs
- This properly reflects the physics: background is noise, signal carries the true energy
