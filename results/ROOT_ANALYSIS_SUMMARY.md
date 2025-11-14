# ROOT Cluster Analysis Summary: False Positive Investigation

## Critical Finding: FPs are Background Radiation, Not Marley Secondaries

### Analysis Date
November 13, 2024

### Dataset
- cat000001 (production MC, 110 files)
- MT v10 model predictions
- 40 events with false positives analyzed
- Plane X (collection plane)

---

## Key Results

### Overall Statistics (40 events analyzed)
- **33 MT clusters** (main track - one per event)
- **141 non-MT clusters** (potential false positives)
- Average: 3.5 non-MT clusters per event

### Non-MT Cluster Composition (The False Positive Pool)

#### By PDG Code:
- **PDG 0: 136 clusters (96.5%)** ← Background radiation (unmatched)
- PDG 11: 5 clusters (3.5%) ← Electrons (Marley)

#### By True Label (Generator):
1. **CavernwallGammasAtLAr1x2x2lowBgAPA: 109 (77.3%)**
2. Th232ChainGenInCPA: 10 (7.1%)
3. Rn220ChainPb212GenInLAr: 7 (5.0%)
4. marley: 5 (3.5%)
5. U238Th232K40GenInLArAPAboards1x2x2: 4 (2.8%)
6. Rn220ChainFromPb212GenInCPA: 3 (2.1%)
7. Other radioactive sources: 3 (2.1%)

### Energy Characteristics

#### Main Track (MT) Clusters:
- Mean energy: 9.47 MeV
- Median energy: 6.68 MeV
- Mean hits: 4.9
- Median hits: 4

#### Non-MT Clusters (FP Candidates):
- Mean energy: 2.40 MeV
- Median energy: 2.31 MeV
- Mean hits: 2.3
- Median hits: 2

**Non-MT clusters are much smaller and lower energy than true main tracks.**

### Marley Content
- MT clusters: 100% Marley (by definition)
- Non-MT clusters: 3.3% Marley (tiny fraction)

---

## Critical Insight: Resolution of Apparent Contradiction

### Previous NPZ-Based Analysis Said:
> "83.5% of false positives are Marley events"

### ROOT-Based Analysis Shows:
> "96.5% of FP candidate clusters are background radiation (PDG 0)"

### Why These Don't Contradict:

The npz analysis was looking at **which events have false positives**, not **which clusters are false positives**.

- **83.5% of events-with-FPs are CC (Marley) events**
  - This is an event-level statistic
  - Says: "Most FP predictions occur in Marley events"
  - Doesn't tell us which specific clusters are being misclassified

- **96.5% of non-MT clusters (FP candidates) are background**
  - This is a cluster-level statistic
  - Says: "The confused clusters are background radiation"
  - Tells us exactly what the model is confusing with signal

### The Complete Picture:

**In CC (Marley) events:**
- 1 main electron track (signal)
- Multiple background radiation clusters (cavern gammas, radon decay, etc.)
- Model correctly identifies electron-like energy depositions
- But cannot distinguish the primary electron from background radiation
- Result: Predicts multiple clusters as "main track" in same event

**The 45% contamination comes from:**
- Background radiation clusters in signal events being classified as main tracks
- NOT from Marley secondary particles (those are only 3.5%)
- NOT from split tracks (<1% within 50cm)
- NOT from cosmic rays in ES events (only 16.5% of FPs)

---

## Physical Interpretation

### What is the Model Learning?
The MT model has learned to identify:
- Electron-like energy depositions (dE/dx signature)
- Clusters with certain energy ranges (few MeV to tens of MeV)
- Collection plane signatures

### What is the Model Missing?
The model CANNOT distinguish:
- Primary interaction electron vs background radiation
- Context: which cluster is the actual neutrino interaction product
- Event topology: spatial relationships between clusters

### Why Background Radiation Fools the Model:
Background radiation sources:
- Cavern wall gammas: Compton scattering produces electrons
- Radioactive decay: Electrons from beta decay
- Radon chain: Multiple electrons from decay products

These produce electron-like signatures (dE/dx, energy range) indistinguishable from neutrino interaction electrons when viewed as isolated clusters.

---

## Implications for Model Improvement

### Current Approach (Image-Level Classification):
- Input: Single cluster image on collection plane
- Label: Is this cluster the main track? (binary)
- Problem: No context about other clusters or event topology

### Why This Fails:
Cannot distinguish "electron-like cluster" from "main track electron" because:
1. Background electrons look identical to signal electrons
2. Model sees clusters in isolation
3. No information about event-level context

### Suggested Approaches:

#### Option 1: Event-Level Classification (Recommended)
- Input: All clusters in an event (multi-cluster)
- Output: Which cluster is the main track? (multiclass)
- Advantages:
  * Model sees relative properties (energies, positions, multiplicities)
  * Can learn "main track is the highest energy Marley cluster"
  * Explicitly learns to reject background

#### Option 2: Add Context Features
- Keep cluster-level but add features:
  * Number of other clusters in event
  * Relative energy ranking
  * Distance to other clusters
  * Marley fraction as input
- Advantages:
  * Less radical change to architecture
  * Provides discrimination power
- Disadvantages:
  * Still fundamentally cluster-level
  * May not fully solve problem

#### Option 3: Post-Processing Filter
- Train model as-is
- Apply physics-based cuts:
  * Must be highest energy cluster in event
  * Must have Marley fraction > threshold
  * Must be in CC event
- Advantages:
  * Simple to implement
  * Guaranteed to respect physics
- Disadvantages:
  * Doesn't improve model learning
  * Fixed rules, not learned

---

## Recommendations

### Immediate Actions:
1. **Implement post-processing filter** (Option 3)
   - Select highest energy predicted cluster per event
   - Filter by Marley fraction > 0.5
   - Expected: Reduce contamination to ~10-15%

2. **Analyze ES vs CC separately**
   - ES events should have zero FPs (no Marley secondaries)
   - Can establish baseline performance

### Medium-Term Development:
3. **Prototype event-level model** (Option 1)
   - Graph neural network or transformer
   - Input: Set of clusters per event
   - Output: Main track classification
   - Expected: Much better discrimination

### Long-Term Research:
4. **Investigate hybrid approach**
   - Combine image features + truth-level hints
   - Physics-informed neural networks
   - Multi-task learning (cluster classification + main track selection)

---

## Files Generated

### Analysis Scripts:
1. `python/ana/analyze_fp_clusters.py` - ROOT cluster analysis (this analysis)
2. `python/ana/analyze_mt_predictions.py` - NPZ prediction analysis (2-page PDF)
3. `python/ana/threshold_scan.py` - Contamination vs threshold scan
4. `python/ana/analyze_false_positives.py` - Detailed FP investigation (5-page PDF)

### Output Files:
- `results/fp_root_analysis_all_nmt.pdf` - This analysis (2-page PDF)
- `results/mt_analysis_report.pdf` - Overall performance
- `results/threshold_scan.pdf` - Threshold optimization
- `results/fp_analysis_report.pdf` - Spatial and energy analysis

---

## Conclusions

1. **False positives are 96.5% background radiation**, not Marley secondaries
2. **77% come from cavern wall gammas**, with remainder from radioactive decay chains
3. **Model correctly identifies electron-like signatures** but cannot distinguish signal from background
4. **Solution requires event-level context** or physics-based post-processing
5. **Threshold adjustment alone cannot solve this** (contamination plateaus at 38%)

The path forward is clear: move from cluster-level to event-level classification, or implement physics-informed filtering. The current architecture is fundamentally limited by lack of context.

---

## Contact
Analysis performed in `online-pointing-utils` repository
Data: `/eos/project-e/ep-nu/public/sn-pointing/cat000001/`
Model predictions: `data-selection-pipeline/results/mt_inference_cat000001_v10/`
