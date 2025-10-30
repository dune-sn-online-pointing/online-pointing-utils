# Cluster Matching Criteria and Multiple Match Handling

## Current Matching Criteria

### Temporal Window
**Time tolerance**: ±5000 ticks (~2.56 ms)
- Each tick = 0.512 µs
- 5000 ticks = 2,560 µs = 2.56 ms
- U-cluster must be within ±5000 ticks of X-cluster
- V-cluster must be within ±5000 ticks of U-cluster (not X directly)

**Consideration**: This is quite permissive. For supernova neutrino interactions, signals are expected to be more temporally coincident. Reducing to ±1000 ticks (~512 µs) might improve specificity.

### Spatial Criteria (from `are_compatibles()`)

1. **Same APA Requirement**
   - All three clusters must be on the same APA module
   - APA determined by: `detector_channel / APA::total_channels`

2. **Y-Coordinate Matching** (≤ 5 cm)
   - Project U and V clusters to X-cluster's z-position
   - Calculate predicted y-coordinates: `y_pred_u` and `y_pred_v`
   - Require: `|y_pred_u - y_pred_v| ≤ 5 cm`

3. **X-Coordinate Matching** (≤ 5 cm)
   - Extract x-coordinates from all three clusters (using abs value)
   - Calculate maximum pairwise distance: max(|x_u - x_x|, |x_v - x_x|, |x_u - x_v|)
   - Require: `max_distance ≤ 5 cm`

### Matching Algorithm Flow

```cpp
FOR each X-cluster (collection plane drives the matching):
    FOR each U-cluster within time window of X:
        FOR each V-cluster within time window of U:
            IF are_compatibles(U, V, X, radius=5cm):
                CREATE MATCH(U, V, X)
```

This creates **ALL geometrically compatible combinations**.

## Multiple Match Handling

### The Problem
When multiple U/V combinations are geometrically compatible with the same X-cluster, the algorithm creates multiple matches. This leads to:
- 32 geometric matches found
- But only 8 unique X-clusters involved
- Only 2 unique U-clusters involved
- Only 2 unique V-clusters involved

**Example from ES validation sample**:
```
Match 0: U_225 + V_227 + X_55
Match 1: U_225 + V_228 + X_55  <- Same X and U as Match 0
Match 2: U_226 + V_227 + X_55  <- Same X and V as Match 0
Match 3: U_226 + V_228 + X_55  <- Same X as Match 0
```

### Current Solution: Keep First Match Only

The current implementation assigns **only the first match** to each cluster:

```cpp
for (size_t match_id = 0; match_id < clusters.size(); match_id++) {
    int u_id = clusters[match_id][0].get_cluster_id();
    int v_id = clusters[match_id][1].get_cluster_id();
    int x_id = clusters[match_id][2].get_cluster_id();
    
    // Only assign if not already matched (keep first match)
    if (u_cluster_to_match.find(u_id) == u_cluster_to_match.end()) {
        u_cluster_to_match[u_id] = match_id;
    }
    // Same for V and X...
}
```

**Result**:
- Each cluster appears in at most one match
- Ambiguous cases are resolved by order (first found wins)
- 32 geometric matches → 8 unique matched clusters in X-plane

### Alternative Approaches (Not Implemented)

1. **Best Match Selection**
   - Rank matches by quality metric (e.g., geometric distance, charge correlation)
   - Keep only the best match per X-cluster
   - **Pros**: More principled selection
   - **Cons**: Requires defining "best" metric

2. **Allow Multiple Matches**
   - Change `match_id` from `int` to `vector<int>`
   - Each cluster stores all matches it participates in
   - **Pros**: Preserves all information
   - **Cons**: More complex downstream analysis

3. **Duplicate Cluster Entries**
   - Write each cluster once per match it participates in
   - Different `match_id` for each entry
   - **Pros**: Simpler structure (still single int match_id)
   - **Cons**: Larger file size, potential confusion

4. **Stricter Matching Criteria**
   - Reduce time window (e.g., ±1000 ticks instead of ±5000)
   - Reduce spatial tolerance (e.g., 3 cm instead of 5 cm)
   - **Pros**: Fewer ambiguous matches
   - **Cons**: Might miss legitimate matches due to detector effects

## Tuning Recommendations

### Reduce Time Window
**Current**: ±5000 ticks (2.56 ms)  
**Suggested**: ±1000 ticks (512 µs) or ±500 ticks (256 µs)

**Rationale**:
- Electron drift time across 3m TPC: ~1.5 ms
- Collection plane signals should be nearly synchronous
- Induction planes have ~100 µs offset due to wire geometry
- ±500-1000 ticks should be sufficient for true coincidences

**Testing approach**:
```cpp
// In match_clusters.cpp, line 107 and 133:
// Change from:
if (clusters_u[j].get_tps()[0]->GetTimeStart() > clusters_x[i].get_tps()[0]->GetTimeStart() + 5000)
// To:
if (clusters_u[j].get_tps()[0]->GetTimeStart() > clusters_x[i].get_tps()[0]->GetTimeStart() + 1000)
```

### Make Matching Criteria Configurable

Add to JSON configuration:
```json
{
  "input_clusters_file": "...",
  "output_file": "...",
  "matching_params": {
    "time_tolerance_ticks": 1000,
    "spatial_tolerance_cm": 5.0,
    "require_unique_matches": true
  }
}
```

### Performance vs. Purity Trade-off

| Time Window | Expected Effect |
|-------------|----------------|
| ±5000 ticks | Current: High recall, some ambiguity |
| ±1000 ticks | Balanced: Good recall, less ambiguity |
| ±500 ticks  | Conservative: Lower recall, high purity |

Recommend starting with ±1000 ticks and adjusting based on:
- MARLEY matching efficiency (should stay >90%)
- Background rejection (should improve)
- Number of ambiguous matches (should decrease significantly)

## Validation Steps

After tuning matching criteria:

1. **Run with new parameters**
   ```bash
   ./build/src/app/match_clusters -j output/test_es_valid_match_config.json
   ```

2. **Check match statistics**
   ```python
   # Count unique X-clusters with matches vs total geometric matches
   # Should see ratio improve (closer to 1:1)
   ```

3. **Verify MARLEY purity**
   ```python
   # All matched clusters should still be MARLEY (100% purity)
   # If purity drops, criteria may be too loose
   ```

4. **Measure efficiency**
   ```python
   # Fraction of MARLEY X-clusters that get matched
   # Should remain >90% with tighter criteria
   ```

## Implementation Status

### ✅ Completed
- Match assignment logic (first match only)
- Output structure (matched_clusters files with U/V/X trees)
- Metadata branches (match_id, match_type)
- Updated tools to read matched_clusters:
  - `analyze_clusters` - Recognizes and logs match_id presence
  - `generate_cluster_arrays.py` - Includes match_id in metadata (indices 15-16)
  - `create_volumes.py` - Includes match_id in cluster_info dict

### ⏳ Future Enhancements
- Make matching criteria configurable via JSON
- Implement match quality metrics
- Add 2-plane matching (U+X, V+X for partial matches)
- Provide statistics on ambiguous matches in output
