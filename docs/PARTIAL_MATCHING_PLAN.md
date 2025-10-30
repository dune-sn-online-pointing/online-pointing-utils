# Implementation Plan: Add Partial (2-plane) Matching

## Summary
- **Current**: Only U+V+X matches (requires all 3 views)
- **Goal**: Also create U+X and V+X matches for unmatched X-clusters
- **Timing**: Adds ~0.6 seconds per file (2-3x slower, still <1s total)
- **When**: Post-processing step AFTER clustering (in match_clusters)

---

## Code Changes Required

### 1. Add 2-plane geometric compatibility functions

**File**: `src/planes-matching/match_clusters_libs.h`

Add declarations:
```cpp
bool are_compatibles_UX(Cluster& c_u, Cluster& c_x, float radius);
bool are_compatibles_VX(Cluster& c_v, Cluster& c_x, float radius);
```

**File**: `src/planes-matching/match_clusters_libs.cpp`

Add implementations (similar to existing `are_compatibles` but for 2 planes):
```cpp
bool are_compatibles_UX(Cluster& c_u, Cluster& c_x, float radius) {
    // Check same detector
    if (c_u.get_tp(0)->GetDetector() != c_x.get_tp(0)->GetDetector())
        return false;
    
    // Check X coordinate distance (both have X info)
    float x_u = std::abs(c_u.get_true_pos()[0]);
    float x_x = std::abs(c_x.get_true_pos()[0]);
    if (std::abs(x_u - x_x) > radius)
        return false;
    
    // Note: Can't check Y without V-plane info
    // More permissive than 3-plane matching
    return true;
}

bool are_compatibles_VX(Cluster& c_v, Cluster& c_x, float radius) {
    // Similar logic for V+X
    if (c_v.get_tp(0)->GetDetector() != c_x.get_tp(0)->GetDetector())
        return false;
    
    float x_v = std::abs(c_v.get_true_pos()[0]);
    float x_x = std::abs(c_x.get_true_pos()[0]);
    if (std::abs(x_v - x_x) > radius)
        return false;
    
    return true;
}
```

### 2. Add 2-plane join functions

**File**: `src/planes-matching/match_clusters_libs.cpp`

Add:
```cpp
Cluster join_clusters_UX(Cluster& c_u, Cluster& c_x) {
    std::vector<TriggerPrimitive*> tps;
    int common_event = c_x.get_event();
    
    // Collect U TPs
    for (auto* tp : c_u.get_tps()) {
        tp->SetEvent(common_event);
        tps.push_back(tp);
    }
    
    // Collect X TPs
    for (auto* tp : c_x.get_tps()) {
        tp->SetEvent(common_event);
        tps.push_back(tp);
    }
    
    return Cluster(tps);
}

Cluster join_clusters_VX(Cluster& c_v, Cluster& c_x) {
    // Similar for V+X
    std::vector<TriggerPrimitive*> tps;
    int common_event = c_x.get_event();
    
    for (auto* tp : c_v.get_tps()) {
        tp->SetEvent(common_event);
        tps.push_back(tp);
    }
    
    for (auto* tp : c_x.get_tps()) {
        tp->SetEvent(common_event);
        tps.push_back(tp);
    }
    
    return Cluster(tps);
}
```

### 3. Modify main matching loop

**File**: `src/app/match_clusters.cpp`

Add after the existing 3-plane loop (around line 162):

```cpp
    // Track which X-clusters were already matched in 3-plane
    std::set<int> matched_x_indices;
    for (const auto& cluster_triplet : clusters) {
        // Find X cluster index in original list
        for (int i = 0; i < clusters_x.size(); i++) {
            if (clusters_x[i].get_cluster_id() == cluster_triplet[2].get_cluster_id()) {
                matched_x_indices.insert(i);
                break;
            }
        }
    }
    
    LogInfo << "Starting 2-plane matching for unmatched X-clusters..." << std::endl;
    
    // Match U+X for unmatched X-clusters
    for (int i = 0; i < clusters_x.size(); i++) {
        if (matched_x_indices.find(i) != matched_x_indices.end()) {
            continue;  // Already matched in 3-plane
        }
        
        // Search for U matches (same time window logic as before)
        for (int j = 0; j < clusters_u.size(); j++) {
            if (clusters_u[j].get_tps()[0]->GetTimeStart() > clusters_x[i].get_tps()[0]->GetTimeStart() + 5000)
                break;
            if (clusters_u[j].get_tps()[0]->GetTimeStart() < clusters_x[i].get_tps()[0]->GetTimeStart() - 5000)
                continue;
            if (int(clusters_u[j].get_tps()[0]->GetDetectorChannel()/APA::total_channels) != 
                int(clusters_x[i].get_tps()[0]->GetDetectorChannel()/APA::total_channels))
                continue;
            
            if (are_compatibles_UX(clusters_u[j], clusters_x[i], 5)) {
                Cluster c = join_clusters_UX(clusters_u[j], clusters_x[i]);
                multiplane_clusters.push_back(c);
                matched_x_indices.insert(i);  // Mark as matched
                break;  // Take first compatible U match
            }
        }
    }
    
    // Match V+X for still unmatched X-clusters
    for (int i = 0; i < clusters_x.size(); i++) {
        if (matched_x_indices.find(i) != matched_x_indices.end()) {
            continue;  // Already matched (3-plane or U+X)
        }
        
        // Search for V matches
        for (int k = 0; k < clusters_v.size(); k++) {
            if (clusters_v[k].get_tps()[0]->GetTimeStart() > clusters_x[i].get_tps()[0]->GetTimeStart() + 5000)
                break;
            if (clusters_v[k].get_tps()[0]->GetTimeStart() < clusters_x[i].get_tps()[0]->GetTimeStart() - 5000)
                continue;
            if (int(clusters_v[k].get_tps()[0]->GetDetectorChannel()/APA::total_channels) != 
                int(clusters_x[i].get_tps()[0]->GetDetectorChannel()/APA::total_channels))
                continue;
            
            if (are_compatibles_VX(clusters_v[k], clusters_x[i], 5)) {
                Cluster c = join_clusters_VX(clusters_v[k], clusters_x[i]);
                multiplane_clusters.push_back(c);
                matched_x_indices.insert(i);
                break;  // Take first compatible V match
            }
        }
    }
    
    LogInfo << "Total matches (3-plane + 2-plane): " << multiplane_clusters.size() << std::endl;
    LogInfo << "  - 3-plane (U+V+X): " << clusters.size() << std::endl;
    LogInfo << "  - 2-plane (U+X or V+X): " << multiplane_clusters.size() - clusters.size() << std::endl;
```

---

## Testing Plan

1. **Compile**: Rebuild with new functions
2. **Run**: Test on current ES sample
3. **Verify**: Check output has more matches
4. **Analyze**: Use existing scripts to check composition

Expected results on current sample:
- 3-plane: 32 matches (same as before)
- U+X: ~0-10 additional (estimate based on geometry)
- V+X: ~0-10 additional
- Total: 32-52 matches

---

## Analysis Updates Needed

Scripts like `analyze_matching_completeness.py` should show:
- U+V+X matches: 32 (100% → now lower %)
- U+X matches: X (new!)
- V+X matches: Y (new!)

---

## Decision Point

**Ready to implement?** 

Estimated effort:
- Code changes: ~30 minutes
- Testing: ~10 minutes  
- Analysis: ~20 minutes
- Total: ~1 hour

Performance impact: Negligible (+0.6s per file)

Say "yes" to proceed with implementation!
