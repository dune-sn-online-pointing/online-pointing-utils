# Main Cluster Identification Feature

## Summary
Added functionality to identify and mark the main (most energetic) cluster in each event, particularly useful for MARLEY events where the primary electron track needs to be distinguished from other activity.

## Changes Made

### 1. Cluster.h - Added New Fields
**Location:** `src/objects/Cluster.h`

Added two new private member variables:
```cpp
int true_pdg_ {0};              // PDG code of the dominant true particle
bool is_main_cluster_ {false};  // Flag indicating this is the most energetic cluster in the event
```

Added corresponding getters and setters:
```cpp
int get_true_pdg() const { return true_pdg_; }
bool get_is_main_cluster() const { return is_main_cluster_; }
void set_true_pdg(int pdg) { true_pdg_ = pdg; }
void set_is_main_cluster(bool is_main) { is_main_cluster_ = is_main; }
```

### 2. Cluster.cpp - Populate PDG Code
**Location:** `src/objects/Cluster.cpp`

Updated `update_cluster_info()` to extract and store the PDG code from the dominant true particle:
```cpp
true_pdg_ = dominant_particle->GetPdg();
```

### 3. make_clusters.cpp - Identify Main Cluster
**Location:** `src/app/make_clusters.cpp`

Added logic after cluster creation to identify the most energetic cluster per view per event:
```cpp
// Identify the main cluster (most energetic) in each view for this event
for (size_t iView=0; iView<APA::views.size(); ++iView) {
    auto& clusters = clusters_per_view.at(iView);
    if (clusters.empty()) continue;
    
    // Find cluster with highest true particle energy
    Cluster* main_cluster = nullptr;
    float max_energy = -1.0f;
    for (auto& cluster : clusters) {
        float energy = cluster.get_true_particle_energy();
        if (energy > max_energy) {
            max_energy = energy;
            main_cluster = &cluster;
        }
    }
    
    // Mark the main cluster
    if (main_cluster != nullptr) {
        main_cluster->set_is_main_cluster(true);
    }
}
```

### 4. Clustering.cpp - Save New Fields
**Location:** `src/clustering/Clustering.cpp`

Updated `write_clusters()` function to:
- Add new branch variables
- Create branches in new trees
- Set branch addresses for existing trees
- Fill the new values when writing clusters

New branches in ROOT file:
```cpp
clusters_tree->Branch("true_pdg", &true_pdg, "true_pdg/I");
clusters_tree->Branch("is_main_cluster", &is_main_cluster, "is_main_cluster/O");
```

## Usage

### Identifying Main Electron Track in MARLEY Events

For MARLEY (supernova neutrino) events, you can now identify the primary electron track cluster using:

```cpp
// Read cluster tree
TTree* tree = file->Get("clusters/clusters_tree_X");  // or U, V
bool is_main_cluster;
int true_pdg;
float true_particle_energy;
tree->SetBranchAddress("is_main_cluster", &is_main_cluster);
tree->SetBranchAddress("true_pdg", &true_pdg);
tree->SetBranchAddress("true_particle_energy", &true_particle_energy);

for (int i = 0; i < tree->GetEntries(); i++) {
    tree->GetEntry(i);
    
    // Check if this is the main cluster
    if (is_main_cluster) {
        // This is the most energetic cluster in this event
        
        // For MARLEY events, check if it's the primary electron
        if (true_pdg == 11) {  // 11 = electron
            // This is the main electron track!
            std::cout << "Found main electron with energy: " 
                      << true_particle_energy << " MeV" << std::endl;
        }
    }
}
```

### Python Analysis Example

```python
import uproot
import numpy as np

# Open clusters file
file = uproot.open("clusters.root")
tree = file["clusters/clusters_tree_X"]

# Read relevant branches
df = tree.arrays(["event", "is_main_cluster", "true_pdg", 
                  "true_particle_energy", "true_label"], library="pd")

# Filter for main electron clusters in MARLEY events
main_electrons = df[
    (df["is_main_cluster"] == True) & 
    (df["true_pdg"] == 11) &
    (df["true_label"] == "marley")
]

print(f"Found {len(main_electrons)} main electron clusters")
print(f"Energy range: {main_electrons['true_particle_energy'].min():.1f} - "
      f"{main_electrons['true_particle_energy'].max():.1f} MeV")
```

## Benefits

1. **Clear Identification**: No ambiguity about which cluster represents the main physics signal
2. **Event-by-Event**: Works on a per-event basis, correctly handling multi-event files
3. **Per-View**: Identifies main cluster separately for each detector view (U, V, X)
4. **Backward Compatible**: Existing analysis code continues to work; new fields are optional
5. **Truth-Based**: Uses true particle energy for reliable identification

## PDG Codes Reference

Common PDG codes for MARLEY events:
- `11` = electron (e-)
- `-11` = positron (e+)
- `22` = photon (γ)
- `2112` = neutron
- `2212` = proton
- `13` = muon (μ-)

## Notes

- The `is_main_cluster` flag is set based on `true_particle_energy`, which comes from backtracking
- For events without truth information, all clusters will have `is_main_cluster = false`
- For background (non-signal) events, the most energetic cluster is still marked as main
- In each view (U, V, X), there is at most one main cluster per event

## Testing

To verify the new fields are populated correctly:

```bash
# Generate clusters
./scripts/make_clusters.sh -j json/make_clusters/example.json

# Inspect with ROOT
root -l output/clusters.root
root [1] TTree* t = (TTree*)_file0->Get("clusters/clusters_tree_X");
root [2] t->Print();  // Should show true_pdg and is_main_cluster branches
root [3] t->Draw("true_particle_energy", "is_main_cluster==1");  // Plot main cluster energies
root [4] t->Draw("true_pdg", "is_main_cluster==1");  // Check PDG codes of main clusters
```

## Compilation

All changes compiled successfully:
```bash
cd build
make -j4
```

All targets built without errors or warnings.
