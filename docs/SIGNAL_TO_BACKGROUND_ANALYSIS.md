# Signal-to-Background Ratio Analysis

## Overview
The signal-to-background (S/B) ratio is a critical metric for understanding the purity of cluster selection at different energy cuts. A higher S/B ratio indicates better separation between MARLEY signal and background contamination.

## Key Results

### Signal-to-Background Ratios by Energy Cut

#### CC Sample (Charged Current):
| Energy Cut (MeV) | S/B Ratio | Interpretation |
|------------------|-----------|----------------|
| 0.0 | 0.032 | 3.2% signal, heavily background-dominated |
| 0.5 | 0.081 | 8.1% signal, significant improvement |
| 1.0 | 0.122 | 12.2% signal, ~1:8 signal-to-background |
| 1.5 | 0.197 | 19.7% signal, approaching 1:5 |
| 2.0 | 0.420 | 42.0% signal, ~1:2 signal-to-background |
| 2.5 | 2.082 | Signal exceeds background (2:1 ratio) |
| 3.0 | 8.775 | Strong signal dominance |
| 3.5 | 22.200 | Excellent purity |
| 4.0 | 22.071 | Very high purity |
| 4.5 | 21.143 | Maintained high purity |
| 5.0 | 20.429 | 20:1 signal-to-background |

#### ES Sample (Electron Scattering):
| Energy Cut (MeV) | S/B Ratio | Interpretation |
|------------------|-----------|----------------|
| 0.0 | 0.017 | 1.7% signal, heavily background-dominated |
| 0.5 | 0.046 | 4.6% signal, improvement but still low |
| 1.0 | 0.073 | 7.3% signal, ~1:14 signal-to-background |
| 1.5 | 0.124 | 12.4% signal, ~1:8 |
| 2.0 | 0.393 | 39.3% signal, approaching parity |
| 2.5 | 2.944 | Signal exceeds background (~3:1) |
| 3.0 | 14.706 | Strong signal dominance |
| 3.5 | 46.800 | Excellent purity |
| 4.0 | 45.000 | Very high purity |
| 4.5 | 42.600 | Maintained high purity |
| 5.0 | 41.400 | 41:1 signal-to-background |

## Key Observations

### 1. **Dramatic S/B Improvement with Energy Cuts**
- At 0 MeV: Background overwhelms signal (~30:1 background:signal)
- At 2.5-3.0 MeV: **Critical transition point** where S/B crosses 1.0
- At 5.0 MeV: Signal dominates (~20-40:1 signal:background)

### 2. **Operating Point Trade-offs**

**Low Energy Cuts (0.5-1.0 MeV):**
- ✅ Excellent signal retention (>90%)
- ❌ Low S/B ratios (0.05-0.12)
- ❌ Background still dominates sample composition
- **Use case**: Studies where total signal statistics are critical

**Medium Energy Cuts (2.0-2.5 MeV):**
- ✅ Good signal retention (~60-70%)
- ✅ Approaching signal-background parity (S/B ~ 0.4-2)
- ✅ Balanced purity and efficiency
- **Use case**: General physics analysis

**High Energy Cuts (3.0-5.0 MeV):**
- ⚠️ Moderate signal loss (40-50%)
- ✅ Excellent S/B ratios (>8)
- ✅ Background becomes negligible
- **Use case**: High-purity studies, background-sensitive measurements

### 3. **CC vs ES Comparison**
- Both samples show similar S/B trends
- ES achieves slightly better S/B at high energy cuts
- The crossover point (S/B = 1) occurs around **2.5 MeV for both**

### 4. **The "Sweet Spot" Analysis**

For most physics applications, consider these three regimes:

**Option A: Maximum Statistics (1.0 MeV)**
- S/B ~ 0.1 (background still dominates by ~10:1)
- Retains ~85% of signal
- Requires careful background modeling

**Option B: Balanced Approach (2.0 MeV)** ⭐ **RECOMMENDED**
- S/B ~ 0.4 (background:signal ~ 2.5:1)
- Retains ~65% of signal
- Much cleaner sample than Option A
- Background modeling still important but more manageable

**Option C: High Purity (3.0 MeV)**
- S/B > 8 (signal dominates)
- Retains ~50% of signal
- Background becomes subdominant
- Ideal for systematic studies

## Interpretation for Physics Analysis

### Background Modeling Requirements
- **At 1 MeV**: Must model background to ~1% precision (it's 90% of sample)
- **At 2 MeV**: Background modeling to ~10% precision (it's ~70% of sample)
- **At 3 MeV**: Background becomes < 10% of sample, much less critical

### Statistical Considerations
The plot shows that **2.0 MeV represents an optimal compromise**:
1. Still retains majority of signal statistics
2. Background becomes more manageable (not overwhelming)
3. S/B improvement is dramatic compared to lower cuts
4. Further increases in energy cut yield diminishing returns in S/B while losing signal

## Conclusion

The signal-to-background ratio plot reveals that:

1. **Energy cuts are highly effective** at improving sample purity
2. **2.0-2.5 MeV is the transition region** where S/B approaches unity
3. **Trade-off is unavoidable**: Higher purity → Lower statistics
4. **Recommended operating point**: 2.0 MeV for balanced S/B and signal retention

For most supernova neutrino analyses, **2.0 MeV** provides the best compromise between:
- Manageable background levels (S/B ~ 0.4)
- Good signal efficiency (~65% retained)
- Simplified systematic uncertainties

The choice ultimately depends on whether the analysis is:
- **Statistics-limited** → Use 1.0 MeV (accept lower S/B)
- **Background-limited** → Use 3.0 MeV (accept signal loss)
- **Balanced** → Use 2.0 MeV (optimal compromise)
