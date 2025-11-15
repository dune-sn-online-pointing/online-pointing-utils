# Time Threshold Optimization Results

## Summary

Tested time thresholds from ±25 to ±5000 ticks to find optimal matching parameters.

## Key Findings

### Threshold Groups

**Group 1: Loose (±5000 ticks / 2.56 ms)**
- Geometric matches: 26
- Unique X-clusters matched: 8
- Unique U-clusters matched: 2
- Unique V-clusters matched: 2
- MARLEY purity: 100%
- Efficiency (X/geo): 31%

**Group 2: Tight (±3000 to ±25 ticks / 1.54 ms to 12.8 µs)**
- Geometric matches: 27-28
- Unique X-clusters matched: 4
- Unique U-clusters matched: 1
- Unique V-clusters matched: 2
- MARLEY purity: 100%
- Efficiency (X/geo): 14-15%

## Critical Observation

**There's a discontinuity between ±5000 and ±3000 ticks:**
- ±5000 ticks: 8 X-clusters matched
- ±3000 ticks: 4 X-clusters matched (**50% loss!**)

This suggests that 4 of the 8 X-clusters have their best U/V matches in the 3000-5000 tick window.

## Measured Time Differences (From Data Analysis)

Minimum time differences between MARLEY clusters:
- **U-X**: 32 ticks (16.4 µs)
- **V-X**: 0 ticks (perfectly synchronized!)
- **U-V**: 0 ticks (perfectly synchronized!)

## Recommendations

### Option 1: Maximize Matches (Current)
**Use ±5000 ticks (2.56 ms)**

**Pros:**
- Gets 8 X-cluster matches (vs 4 with tighter thresholds)
- Maintains 100% MARLEY purity
- More robust to timing variations

**Cons:**
- More ambiguous matches (26 geometric → 8 unique = 31% efficiency)
- "First match" strategy may not always pick the best combination

**Best for:** Maximum signal efficiency, production runs where we want to catch all possible matches

### Option 2: Balanced Approach
**Use ±100 ticks (51.2 µs)**

**Pros:**
- Still gets 4 X-cluster matches
- Maintains 100% MARLEY purity
- 200× tighter than current threshold
- More principled (based on measured time sync)

**Cons:**
- Loses 50% of X-cluster matches vs. current
- May be too restrictive for detector timing variations

**Best for:** Studies where purity and temporal precision are more important than efficiency

### Option 3: Ultra-Tight (Minimum Viable)
**Use ±32 ticks (16.4 µs)**

**Pros:**
- Based on measured minimum time difference (U-X = 32 ticks)
- Maintains 100% MARLEY purity
- Minimal ambiguity

**Cons:**
- Same 50% efficiency loss
- No headroom for timing jitter
- Might miss edge cases

**Best for:** Algorithm validation, understanding minimum requirements

## Physical Interpretation

The fact that V and X clusters can be perfectly synchronized (0 tick difference) makes sense:
- Collection plane (X) measures ionization directly
- Induction planes (U, V) see the same charge but with angular projections
- V plane happens to have timing very close to X for these clusters

The 32-tick minimum difference for U clusters (~16 µs) likely reflects:
- Wire geometry differences
- Signal processing in the readout
- Slight variations in drift paths

## Ambiguity Analysis

The low efficiency ratio (14-31%) indicates significant matching ambiguity:
- Multiple U+V combinations satisfy geometric constraints for same X
- This is expected given detector geometry
- Current "first match" strategy resolves ambiguity deterministically

### Potential Improvements to Ambiguity Handling

1. **Charge-based ranking**: Pick U+V combination with best charge correlation
2. **Distance-based ranking**: Pick combination with minimum 3D spatial distance
3. **Energy-based ranking**: Pick combination with most consistent energy
4. **Allow multiple matches**: Track all valid combinations per cluster

## Recommendation: KEEP ±5000 ticks

**Rationale:**
1. **2× better matching efficiency** (8 vs 4 X-clusters)
2. **Still 100% MARLEY purity** (no background contamination)
3. **Robust to timing variations** (important for production data)
4. **Ambiguity is manageable** with current "first match" strategy

The 50% loss in matches with tighter thresholds is too costly. The measured minimum time difference (32 ticks) doesn't mean all matches should be that tight - it just means the best-matched clusters are very synchronized. Other legitimate matches may have slightly larger time differences due to:
- Detector response variations
- Charge diffusion during drift
- Readout timing variations
- Event topology (extended activity)

## Action Items

- [x] Current ±5000 tick threshold is validated
- [ ] Consider implementing match quality metrics for ambiguity resolution
- [ ] Add timing statistics to match output for monitoring
- [ ] Test with CC sample to verify threshold works across interaction types
