# Summary: Clustering Algorithm Bug and Fix

## What You Discovered

You found clusters (like cluster 23, 19, 25, 52) with TPs that were **very far apart in time** (~130 ticks), even though the clustering parameter was `tick_limit=5`.

## The Bug

The clustering algorithm had a **temporal chaining/bridging bug**:

### Old Buggy Logic:
```
For each new TP:
  1. Is it within 5 ticks of the LATEST time in the cluster? ✓
  2. Does it match the channel of ANY TP in the cluster? ✓
  → If both YES, add it to the cluster
```

### Why This Created Problems:
```
Example with tick_limit=5, channel_limit=2:

TP_A: time=4050, channel=10  → New cluster
TP_B: time=4055, channel=12  → Added (Δt=5 from A, Δch=2 from A) ✓
...
TP_C: time=4140, channel=11  → Added (Δt=5 from previous TP, matches channel with B) ✓
...
TP_D: time=4180, channel=10  → Added (Δt=5 from previous TP, matches channel with A) ✓

Result: Cluster spans 130 ticks! (4180 - 4050 = 130)
```

The problem: It checked time with the **latest TP** but channel with **ANY TP**, allowing distant TPs to join via intermediate bridges.

## The Fix

Changed the algorithm to require **both conditions with the SAME TP**:

### New Fixed Logic:
```
For each new TP:
  For each TP already in the cluster:
    Is the new TP within tick_limit AND channel_limit of THIS specific TP?
    → If YES for at least ONE TP, add it to the cluster
```

This prevents temporal chaining while still allowing valid channel bridging for tracks.

## Code Change

**File:** `src/clustering/Clustering.cpp` (lines ~180-205)

**Before:**
- Checked time against cluster maximum time
- Checked channel against any TP in cluster separately

**After:**
- Both checks must pass for the same reference TP
- Prevents temporal chaining while preserving spatial clustering

## Next Steps

### To Apply the Fix to Your Data:

```bash
# The fix is already compiled in your build
cd /home/virgolaema/dune/online-pointing-utils

# Re-run clustering with the fixed algorithm
./build/src/app/make_clusters -j json/make_clusters/es_valid.json

# This will create new cluster files with "_clusters.root" suffix
# The old file will be overwritten

# View the new clusters
./scripts/display.sh -j json/display/example.json
```

### Expected Results:

- **More clusters**: Previously merged clusters will now be separate
- **Tighter temporal grouping**: Clusters will respect the 5-tick limit
- **Better physics separation**: Distinct interactions won't be merged

## Impact on Your Analysis

### Positive:
- ✓ Clusters now properly represent single physics interactions
- ✓ Clustering parameters work as intended
- ✓ Better event reconstruction

### To Consider:
- More clusters means different cluster IDs
- May need to adjust downstream analysis expecting certain cluster counts
- Channel bridging still works (tracks can span many channels)

## Technical Details

See `docs/CLUSTERING_BUG_FIX.md` for detailed technical explanation with code examples.
