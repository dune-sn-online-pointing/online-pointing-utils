# Clustering Algorithm Bug Fix - Chain/Bridge Effect

## Problem Description

The clustering algorithm had a **chain/bridge effect bug** that allowed TPs to be added to clusters even when they were much farther apart than the configured `ticks_limit` parameter.

### Example
With `ticks_limit=5` and `channel_limit=2`, we observed clusters like **Cluster 23** with TPs spanning **~130 ticks** instead of the expected maximum of ~5-10 ticks.

## Root Cause

The original algorithm (lines 180-200 in `Clustering.cpp`) worked as follows:

```cpp
// OLD BUGGY CODE:
for (auto& candidate : buffer) {
    // 1. Calculate the maximum time in the current candidate
    double max_time = 0;
    for (auto& tp2 : candidate) {
        max_time = std::max(max_time, tp2->GetTimeStart() + ...);
    }

    // 2. Check if tp1 is within ticks_limit of the LATEST time
    if ((tp1->GetTimeStart() - max_time) <= ticks_limit) {
        // 3. Check channel with ANY TP in the candidate
        for (auto& tp2 : candidate) {
            if (channel_condition_with_pbc(tp1, tp2, channel_limit)) {
                candidate.push_back(tp1);  // Add to cluster!
                break;
            }
        }
    }
}
```

### The Bug
The algorithm separated the **time check** from the **channel check**:
1. First, it checked if tp1 was close in time to the **latest** TP in the cluster
2. Then, it checked if tp1 was close in channel to **ANY** TP in the cluster

This created a **chaining effect**:
- TP_A at (time=100, channel=10)
- TP_B at (time=104, channel=12) → added (close to A in both time and channel)
- TP_C at (time=108, channel=14) → added (close to B in both time and channel)
- TP_D at (time=112, channel=10) → **INCORRECTLY added** (close to C in time, matches channel with A)

Even though TP_D is 12 ticks away from TP_A, it gets added because:
- It's within 5 ticks of the cluster's maximum time (C)
- It matches the channel condition with an older TP (A)

## The Fix

The fixed algorithm ensures that **both conditions must be satisfied with the SAME TP**:

```cpp
// NEW FIXED CODE:
for (auto& candidate : buffer) {
    bool can_append = false;
    
    // For each TP in the candidate
    for (auto& tp2 : candidate) {
        double tp2_end_time = tp2->GetTimeStart() + tp2->GetSamplesOverThreshold() * TPC_sample_length;
        double time_diff = tp1->GetTimeStart() - tp2_end_time;
        
        // Check if tp1 is close in time to THIS SPECIFIC tp2
        if (time_diff <= ticks_limit && time_diff >= 0) {
            // Also check channel condition with THIS SAME tp2
            if (channel_condition_with_pbc(tp1, tp2, channel_limit)) {
                can_append = true;
                break;
            }
        }
    }
    
    if (can_append) {
        candidate.push_back(tp1);
        appended = true;
        break;
    }
}
```

## Impact

### Before Fix
- Clusters could have TPs spanning 100+ ticks (20x the parameter)
- Multiple separate physics interactions could be merged into one cluster
- Clustering quality degraded significantly

### After Fix
- Clusters respect the `ticks_limit` parameter more strictly
- Each TP must be within `ticks_limit` AND `channel_limit` of at least one TP already in the cluster
- Much better separation of distinct physics interactions

## Testing

To test with the new algorithm:

```bash
# 1. Rebuild the code (already done above)
cd /home/virgolaema/dune/online-pointing-utils/build && make -j4

# 2. Re-run clustering on the same input data
# (This will create new cluster files with the fix applied)

# 3. Compare the new clusters with display tool
./scripts/display.sh -j json/display/example.json
```

## Note on Channel Bridging

**Channel bridging is still allowed and correct** - a cluster can span many channels if there are intermediate TPs. The fix only addresses the **temporal chaining** issue, not the spatial one.

For example, this is still valid:
- TP at (t=100, ch=10)
- TP at (t=102, ch=12) → added (Δt=2, Δch=2)
- TP at (t=104, ch=14) → added (Δt=2, Δch=2)
Result: cluster spans 4 channels, which is correct for a track.
