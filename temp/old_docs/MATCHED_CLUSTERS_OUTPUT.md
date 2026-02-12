# Matched Clusters Output Structure

## Overview
The `match_clusters` application now creates `matched_clusters_*` files that mirror the structure of `clusters_*` files but with additional match metadata.

## File Structure

### Output Location
```
output/matched_clusters_tick5_ch2_min2_tot1_e0p0/
    <original_filename>_matched_clusters.root
```

### Tree Organization
Each matched_clusters file contains three separate trees (matching the input clusters structure):
- `clusters/clusters_tree_U` - U-plane clusters with match info
- `clusters/clusters_tree_V` - V-plane clusters with match info  
- `clusters/clusters_tree_X` - X-plane clusters with match info

### New Branches
Two additional branches are added to each tree:

1. **`match_id`** (int):
   - Unique ID for each match group
   - Value `>= 0`: This cluster participates in a match
   - Value `== -1`: This cluster has no match
   - Same match_id across U/V/X trees indicates they belong together

2. **`match_type`** (int):
   - Type of match
   - Value `3`: Three-plane match (U+V+X)
   - Value `2`: Two-plane match (U+X or V+X) [future feature]
   - Value `-1`: No match

## Match Assignment Logic

### Current Behavior
- The matching algorithm finds all geometrically compatible U+V+X combinations
- When multiple matches share clusters, only the **first match** for each cluster is preserved
- This ensures each cluster has at most one match_id

### Example
If the algorithm finds:
```
Match 0: U_225 + V_227 + X_55
Match 1: U_225 + V_228 + X_55
Match 2: U_226 + V_227 + X_55
Match 3: U_226 + V_228 + X_55
```

The result will be:
- U_225: match_id=0 (first occurrence)
- U_226: match_id=2 (first occurrence)
- V_227: match_id=0 (first occurrence)
- V_228: match_id=1 (first occurrence)
- X_55: match_id=0 (first occurrence)

## Configuration

### JSON Format
```json
{
  "input_clusters_file": "path/to/clusters.root",
  "output_file": "path/to/matched_clusters.root"
}
```

### Naming Convention
- Input: `clusters_tick5_ch2_min2_tot1_e0p0/<filename>_clusters.root`
- Output: `matched_clusters_tick5_ch2_min2_tot1_e0p0/<filename>_matched_clusters.root`

## Downstream Usage

### Reading Matched Clusters
```python
import uproot

file = uproot.open("matched_clusters.root")

# Read all three planes
u_tree = file['clusters/clusters_tree_U']
v_tree = file['clusters/clusters_tree_V']
x_tree = file['clusters/clusters_tree_X']

# Get match info
x_match_ids = x_tree['match_id'].array()
x_match_types = x_tree['match_type'].array()

# Filter for matched clusters
matched_mask = x_match_ids >= 0
matched_clusters = x_match_ids[matched_mask]
```

### Joining Planes by match_id
```python
# Get clusters from each plane
u_data = u_tree.arrays(['cluster_id', 'match_id', 'match_type'])
v_data = v_tree.arrays(['cluster_id', 'match_id', 'match_type'])
x_data = x_tree.arrays(['cluster_id', 'match_id', 'match_type'])

# Find a specific match (e.g., match_id=5)
u_cluster = u_data[u_data['match_id'] == 5]
v_cluster = v_data[v_data['match_id'] == 5]
x_cluster = x_data[x_data['match_id'] == 5]
```

## Tools That Need Updating

### High Priority
1. **analyze_clusters** - Should read from matched_clusters files
2. **generate_cluster_arrays.py** - Image generator for clusters
3. **create_volumes.py** - Volume visualization

### JSON Configs
All cluster analysis configs should be updated to use matched_clusters paths:
- `json/es_*.json`
- `json/cc_*.json`
- `output/test_*.json`

## Performance

### Timing (ES validation sample, 72 X-clusters)
- Current implementation: ~415 ms
- Previous (single multiplane tree): ~500 ms
- Improvement due to more efficient writing of 3 trees

### Memory
- No significant change from original clusters file
- Match metadata adds ~2 int branches per cluster (~8 bytes per cluster)

## Future Enhancements

### Two-Plane Matching
Planned addition to include partial matches:
- U+X matches (when no compatible V found)
- V+X matches (when no compatible U found)
- These will have `match_type=2`

### Multiple Matches Per Cluster
Consider allowing clusters to participate in multiple matches:
- Would require `match_id` to be `vector<int>` instead of `int`
- Or duplicate cluster entries for each match
- User feedback needed on preferred approach
