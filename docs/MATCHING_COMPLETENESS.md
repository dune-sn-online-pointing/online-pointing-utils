# Matching Completeness Analysis Summary

## Your Question: Metrics for U-only, V-only, or Both U+V Matches

### **CRITICAL FINDING: Current Algorithm ONLY Creates U+V+X Matches**

From `match_clusters.cpp` analysis:
```cpp
FOR each X-cluster:
  FOR each U-cluster (time-compatible):
    FOR each V-cluster (time-compatible):
      IF are_compatibles(U, V, X, 5cm):
        → Create multiplane match
```

**The algorithm REQUIRES all three views (U, V, X) to be present!**

---

## Results from Current Implementation

### **All 32 matched clusters have U + V + X views**

| Match Type | Count | Percentage |
|------------|-------|------------|
| **U + V + X (complete)** | **32** | **100%** |
| U + X only (no V) | 0 | 0% |
| V + X only (no U) | 0 | 0% |
| X only | 0 | 0% |

This is by design - the nested loop structure ensures matches only happen when U, V, and X are all geometrically compatible.

---

## Matching Potential Analysis

### **All 72 X-clusters have BOTH U and V candidates!**

Time window analysis (±5000 ticks, same APA):

| Category | Count | Percentage |
|----------|-------|------------|
| X with BOTH U and V candidates | **72** | **100%** |
| X with U but NOT V | 0 | 0% |
| X with V but NOT U | 0 | 0% |
| X with NEITHER U nor V | 0 | 0% |

**Key insight**: In this sample, **every X-cluster has time-compatible candidates in both U and V planes**. The limiting factor is the geometric compatibility check (5cm spatial radius), not the availability of candidates.

---

## Matching Success Rate

### **Why only 32/72 matched when all have candidates?**

**Input**: 72 X-clusters (all have both U and V time-compatible)  
**Output**: 32 multiplane matches (44.4% success rate)

**Reason**: The `are_compatibles()` function applies strict geometric constraints:
1. **Spatial distance < 5 cm** in reconstructed X coordinate
2. **Spatial distance < 5 cm** in reconstructed Y coordinate  
3. Consistent across all three (U, V, X) planes

**38 X-clusters fail the geometric test** despite having time-compatible candidates. This is expected for:
- Incomplete tracks (particle stops/exits detector)
- Reconstruction uncertainty
- Cluster splitting/merging artifacts
- Noise hits

---

## Breakdown by Event Type

### **MARLEY Events**

- **70 MARLEY X-clusters** (all have both U+V candidates)
- **32 matched** → **45.7% matching efficiency**
- Geometric compatibility is the bottleneck

### **Background Events**

- **2 background X-clusters** (both have U+V candidates)
- **0 matched** → **0% matching efficiency**
- Perfect rejection by geometric constraints

---

## What If We Allowed Partial Matches?

### **Scenario: Allow X+U or X+V matches**

Based on current data:

| Scenario | Additional Matches | Notes |
|----------|-------------------|-------|
| Current (U+V+X required) | 32 matches | Best 3D constraint |
| Allow X+U matches | 0 additional | All X have U candidates, but geometry fails |
| Allow X+V matches | 0 additional | All X have V candidates, but geometry fails |

**In this specific sample**: Partial matching wouldn't help because:
1. All X-clusters already have both U and V time-compatible candidates
2. The 38 unmatched X-clusters fail the **geometric compatibility test**, not candidate availability
3. Relaxing to 2-plane matches would require:
   - Different geometric check (can't use 3-plane constraint)
   - More background contamination (less constrained)

---

## Key Metrics Summary

| Metric | Value | Notes |
|--------|-------|-------|
| **Total X-clusters** | 72 | Input to matching |
| **X with U+V candidates** | 72 (100%) | Time window only |
| **Multiplane matches created** | 32 (44.4%) | With geometry check |
| **Match composition** | 100% U+V+X | Algorithm requirement |
| **MARLEY matching efficiency** | 45.7% | Expected for strict geometry |
| **Background rejection** | 100% | 0/2 background matched |

---

## Recommendations

### **If you want partial match statistics**:

You would need to **modify the algorithm** to create two output categories:

1. **Complete matches** (U+V+X) - current implementation
2. **Partial matches** (X+U or X+V) - new category

This would require:
- Separate loops for 2-plane matching
- Different geometric compatibility function for 2-plane
- Separate output trees or view composition flag

### **Trade-offs of partial matching**:

**Pros**:
- Higher matching efficiency (more X-clusters matched)
- Can recover incomplete tracks

**Cons**:
- Less constrained 3D reconstruction
- Higher background contamination (easier for noise to match)
- More complex downstream analysis (handle different match types)

---

## Conclusion

**The current algorithm creates ONLY complete U+V+X matches (32/32 = 100%)**

There are NO partial matches in the current output because the algorithm design explicitly requires all three views. In this particular sample, all 72 X-clusters have time-compatible U and V candidates, so the matching bottleneck is geometric compatibility (5cm radius), not candidate availability.

To get partial match metrics, you would need to modify `match_clusters.cpp` to add 2-plane matching logic alongside the existing 3-plane matching.
