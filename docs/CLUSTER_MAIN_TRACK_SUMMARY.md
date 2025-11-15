# Quick Reference: Identifying Main Electron Track in MARLEY Events

## What Was Added

Two new fields in cluster ROOT files:
1. **`true_pdg`** (int): PDG code of the dominant particle (11 = electron)
2. **`is_main_cluster`** (bool): TRUE if this is the most energetic cluster in the event

## How to Use

### In ROOT/C++
```cpp
// Open file and get tree
TFile* f = TFile::Open("clusters.root");
TTree* tree = (TTree*)f->Get("clusters/clusters_tree_X");

// Set branch addresses
bool is_main;
int pdg;
tree->SetBranchAddress("is_main_cluster", &is_main);
tree->SetBranchAddress("true_pdg", &pdg);

// Loop and find main electron
for (int i = 0; i < tree->GetEntries(); i++) {
    tree->GetEntry(i);
    if (is_main && pdg == 11) {
        // This is the main electron track!
    }
}
```

### In Python (uproot/pandas)
```python
import uproot
import pandas as pd

# Read clusters
tree = uproot.open("clusters.root")["clusters/clusters_tree_X"]
df = tree.arrays(["event", "is_main_cluster", "true_pdg", 
                  "true_particle_energy"], library="pd")

# Filter main electrons
main_electrons = df[(df.is_main_cluster) & (df.true_pdg == 11)]
print(f"Found {len(main_electrons)} main electron tracks")
```

### In ROOT Command Line
```bash
root -l clusters.root
root [1] TTree* t = (TTree*)_file0->Get("clusters/clusters_tree_X");
root [2] t->Draw("true_particle_energy", "is_main_cluster && true_pdg==11");
```

## Key Points

- **One main cluster per event per view**: Each view (U, V, X) has at most one cluster flagged as main
- **Energy-based selection**: Main cluster = highest `true_particle_energy` in the event
- **Works with existing files**: Just regenerate clusters with the updated code
- **PDG 11 = electron**: For MARLEY events, main electron has `true_pdg == 11`
- **Backward compatible**: Old analysis code still works; new fields are additions

## Regenerate Clusters

To get the new fields in your clusters files:
```bash
./scripts/make_clusters.sh -j json/make_clusters/your_config.json
```

The output ROOT file will now include `true_pdg` and `is_main_cluster` branches.
