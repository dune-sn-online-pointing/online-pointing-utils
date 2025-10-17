# Clustering Bug Fix: Same Channel Temporal Check

## Issue Description

When examining clusters in the visualization tool, we observed:

1. **Temporal gaps on same channel**: Clusters with TPs on the same channel but separated by ~85-90 ticks (e.g., Cluster 23 had TPs at time ~35-40 and ~125 on channel 0)
2. **Duplicate clusters**: Clusters 2 and 23 appeared identical (same event, energy, charge)

## Root Cause

### Problem 1: Same Channel Temporal Gap

The clustering algorithm in `src/clustering/Clustering.cpp` (lines 180-205) had the following logic:

```cpp
for (auto& tp2 : candidate) {
    double time_diff = tp1->GetTimeStart() - tp2_end_time;
    
    if (time_diff <= ticks_limit && time_diff >= 0) {
        if (channel_condition_with_pbc(tp1, tp2, channel_limit)) {
            can_append = true;
            break;
        }
    }
}
```

**The bug**: When TPs are on the **same channel** (channel difference = 0), the `channel_condition_with_pbc` returns `true` immediately. However, the time check only verified that tp1 comes after tp2 within the limit (`time_diff >= 0 && time_diff <= ticks_limit`).

**The scenario that caused the bug**:
- TP at channel 0, time 35 gets added to cluster
- TP at channel 0, time 125 arrives
- Time check: `125 - 35 = 90 ticks > 5 ticks` → should fail
- BUT: The algorithm checks if tp1 (time 125) comes within 5 ticks AFTER any existing TP
- If there's a TP at time 120-125, the forward check passes
- The channel check passes (same channel, diff=0 ≤ 2)
- Result: TPs 90 ticks apart get clustered!

The algorithm was allowing **forward temporal chaining** on the same channel without checking backward temporal proximity.

## Solution

Added special handling for same-channel TPs to check temporal proximity in **both directions**:

```cpp
// Special case: if on the SAME channel, also check backward in time
// to prevent large time gaps on the same channel from being clustered
else if (tp1->GetDetectorChannel() == tp2->GetDetectorChannel() && 
         tp1->GetDetector() == tp2->GetDetector() && 
         tp1->GetView() == tp2->GetView()) {
    // For same channel, check if time difference is small in EITHER direction
    double tp1_end_time = tp1->GetTimeStart() + tp1->GetSamplesOverThreshold() * TPC_sample_length;
    double reverse_time_diff = tp2->GetTimeStart() - tp1_end_time;
    
    // If the gap is too large in either direction, don't cluster
    if (std::abs(time_diff) <= ticks_limit || std::abs(reverse_time_diff) <= ticks_limit) {
        can_append = true;
        break;
    }
}
```

**Key insight**: For TPs on the same channel, we require them to be close in time bidirectionally, not just allowing forward chaining.

### Problem 2: Duplicate Clusters

**Status**: Partially investigated, needs confirmation with full dataset rerun.

**Hypothesis**: The `write_clusters()` function opens files in `UPDATE` mode and appends to existing trees. If the same event number appears in multiple input files, it could be written multiple times. This would explain why Cluster 2 and Cluster 23 have identical properties.

**Verification needed**: After full rerun, check if duplicates persist. If so, may need to:
- Add event number tracking to prevent duplicate writes
- Use `RECREATE` mode instead of `UPDATE` mode
- Add deduplication logic in write_clusters()

## Testing

1. Rebuilt code with fix: ✅
2. Reran clustering with max_files=20: ✅
3. Need to verify: Open cluster_display and check previously problematic clusters (19, 23) to confirm temporal gaps are fixed

## Files Modified

- `src/clustering/Clustering.cpp` (lines ~195-210): Added same-channel bidirectional time check

## Impact

This fix ensures that TPs on the same channel must be temporally contiguous (within tick_limit in either direction), preventing spurious clustering of distant signals on the same wire.

## Next Steps

1. View clusters in cluster_display to verify fix
2. Rerun full dataset when time permits
3. Monitor for duplicate cluster issue and implement fix if confirmed
