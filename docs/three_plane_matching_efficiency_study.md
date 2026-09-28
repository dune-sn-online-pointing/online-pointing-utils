# Three-plane (X/U/V) main-track matching efficiency

Study of why ~25% of reconstructable ES electrons are lost at the cluster-matching
step, i.e. why an X-plane main-track cluster often does not end up with the same
`match_id` on a **main-track** cluster in *all three* planes, which is what the
evaluation pipeline requires
(`refactor-snop-pipeline/python/lib/sample_loader.py:261-266`, `load_all_planes=True`).

Author: analysis run on 2026-09-03 (E. Villa's sample production, lxplus/EOS).
**Nothing in `json/` or in `src/` was modified for this study.** All new material is in
`python/ana_evilla/` plus this document; the one regeneration was done for a *training*
cat into a scratch folder on EOS.

---

## 1. Executive summary

1. **The loss is not new.** Old cats (1-60, produced 2025-11-19), training cats
   (400-459) and the new cats (623-682, produced 2026-08-24) all give the same
   3-plane main-track fraction: **76.3% / 75.4% / 74.8%** of X main tracks
   (medians per cat 77.1% / 76.1% / 76.2%). cat000623 (82.3%) is simply a
   lucky cat, ~6 points above the median. The matching code is unchanged
   between the two productions (only header renames, see §6).

2. **The gap is between two different definitions of "matched".** In the code's own
   sense (`match_id != -1`) matching *is* working very well: **93-94%** of X main
   tracks are matched. The pipeline's stricter requirement (same `match_id` on a
   **main-track** cluster in X *and* U *and* V) is satisfied only for **~75%**.
   The ~19 point gap is the whole story, and it is made of three effects:

   | effect | share of X main tracks |
   |---|---|
   | 2-plane "partial" matches (X+U or X+V) that still get a `match_id` | 8.5% (U+X) + 7.8% (V+X) on the test subset of §8 |
   | 3-plane match whose U/V partner is **not** the main-track cluster | 11.0% (60-cat average) |
   | X main track with no induction partner at all (`match_id = -1`) | 6.8% (60-cat average) |

3. **Dominant single cause: the induction-plane partner is chosen as the *first
   cluster in time order*, not the main-track one.** In ~11% of all X main tracks
   the `match_id` ends up on a lower-energy MARLEY *fragment* of the same electron
   (98% of the wrong partners are MARLEY, 98% are lower in energy than the main
   cluster). This grows steeply with electron energy: 0.2% below 5 MeV, 21% at
   15-20 MeV, **44% above 30 MeV** - exactly the fragmentation behaviour expected
   for longer tracks on induction wires.

4. **Second cause: `energy_cut = 3.0` MeV removes the electron's induction cluster.**
   Measured directly on the regenerated intermediates, **23% of X main tracks have
   their U (and V) main-track cluster in the `discarded/` tree**, with a median energy
   of 2.3-2.4 MeV - just below the cut. This dominates at low energy: below 5 MeV,
   35% of X main tracks (categories A + D) are lost for lack of any induction cluster.

5. **Two code bugs found** (reported, not fixed): V+X partial matches never give the
   `match_id` to the V cluster (`src/app/match_clusters.cpp:521-533`), and
   `match_type` is hard-coded to 3 for every matched cluster
   (`src/clusters/Clustering.cpp:841`), so downstream code cannot tell a real
   3-plane match from a 2-plane one.

6. **`docs/MATCHING_CRITERIA_AND_HANDLING.md` is stale**: it documents a +-5000 tick
   window and 5 cm geometric cuts. The actual window is `time_tolerance_ticks = 10`
   TPC ticks (= 320 TDC ticks) from the cat JSON, and the geometric compatibility
   function `are_compatibles()` is a stub that only checks "same APA"
   (`src/clusters/MatchClusters.cpp:59-70`).

7. **Parameter-only recovery available now**: `energy_cut` 3.0 -> 2.0 raises the
   3-plane fraction from 59.7% to 75.7% on the test subset (+38% more usable
   electrons) at the price of ~4x more clusters everywhere. See §9 - **not applied**.

---

## 2. What the pipeline asks for, and what the code produces

`sample_loader.py` builds, per file triplet, the sets of `match_id` values carried by
**main-track** clusters in each plane and keeps the intersection:

```python
match_ids_x = {int(m): i for i, m in enumerate(meta_x[:, 13]) if is_main_x[i] and m != -1}
...
common_ids = set(match_ids_x) & set(match_ids_u) & set(match_ids_v)     # sample_loader.py:261-266
```

The matching app, on the other hand:

* only ever starts from **X main-track** clusters (`match_clusters.cpp:263, 310, 360`),
  so no background X cluster can ever carry a `match_id` (verified: 0 out of 84
  non-main X clusters in cat000623);
* accepts **partial** matches (X+U or X+V) and gives them a `match_id` all the same
  (`match_clusters.cpp:386-412`, `517-519`);
* imposes **no main-track preference on the induction side**: it keeps the first
  U (then V) cluster found while scanning in time order (`match_clusters.cpp:301-303,
  348-350`).

Anatomy of the 129 X main tracks of the regenerated test subset (§8):

| | N | % |
|---|---|---|
| X main tracks | 129 | 100.0 |
| `match_id != -1` ("matched" in the code's sense) | 105 | 81.4 |
| ... partner in both U and V | 84 | 65.1 |
| ... ... **both partners are main-track -> survives the pipeline** | **77** | **59.7** |
| ... ... at least one partner is a fragment/background cluster | 7 | 5.4 |
| ... U+X partial match only | 11 | 8.5 |
| ... V+X partial match, `match_id` lost by the bug of §6.7 | 10 | 7.8 |
| no partner at all (`match_id = -1`) | 24 | 18.6 |

---

## 3. Data and tools

Samples (per cat: 10 ES tpstream files, 400 generated ES events):

* new cats `623-682`, training cats `400-459`, old cats `1-60`
  (`/eos/project-e/ep-nu/evilla/sn-online-pointing/sn-burst-samples/catXXXXXX/catXXXXXX_cluster_images_tick3_ch2_min2_tot3_e3p0/{X,U,V}/es_*_bg_matched_plane?.npz`).

New analysis code (this study only, nothing else touched):

| file | purpose |
|---|---|
| `python/ana_evilla/three_plane_matching_efficiency.py` | classifies every X main track from the cluster-image npz files, per-cat and aggregated; energy/position tables; recovery ceiling |
| `python/ana_evilla/emulate_matching_rules.py` | reads the intermediate `*_matched.root`, **reproduces the production matching exactly**, and scans alternative ambiguity rules / time windows / energy cuts offline |
| `python/ana_evilla/matching_study_cat000450.json` | scratch config for the one regeneration (identical physics parameters to `json/cats/cat_000450.json`, only the folders differ) |

Results (json/npz) are kept at
`/eos/project-e/ep-nu/evilla/sn-online-pointing/matching-study-scratch/results/`.

Example:

```bash
python3 python/ana_evilla/three_plane_matching_efficiency.py --cats 623-682 --out new.json
source /cvmfs/sft.cern.ch/lcg/views/LCG_104/x86_64-el9-gcc11-opt/setup.sh   # uproot
python3 python/ana_evilla/emulate_matching_rules.py \
    --folder /eos/project-e/ep-nu/evilla/sn-online-pointing/matching-study-scratch/cat000450study/cat000450study_matched_clusters_tick3_ch2_min2_tot3_e3p0 \
    --tol-ticks 10
```

### Loss classification used below

For every X main-track cluster with `match_id = m`:

* **MATCHED_3PLANE** - `m` is carried by a main-track cluster in U *and* V.
* **A - no cluster in plane** - that plane has **no cluster at all** for this event
  (below the energy cut, hence absent from the npz, or never clustered).
* **B - cluster but not main** - the plane has clusters for this event but none is
  flagged main track (no MARLEY cluster survived there).
* **C1 - main exists, unmatched** - the plane's main-track cluster exists with
  `match_id = -1` (it was not the cluster the algorithm picked).
* **C2 - main exists with a different id** - never observed (0 cases in 55k tracks).
* **C3 - partner is a background/fragment cluster** - `m` *is* present in that plane
  but sits on a **non-main** cluster: the wrong partner was chosen.
* **D - X main track itself unmatched** (`match_id = -1`).

---

## 4. Loss-category table

### cat000623 (the case that triggered the study) - reproduces the reported numbers

400 generated ES events -> 328 events with an X cluster -> **317 X main tracks** ->
308 with `match_id != -1` (97.2%) -> **261 with a 3-plane main-track match (82.3%)**.

| category | N | % of X main |
|---|---|---|
| MATCHED_3PLANE | 261 | 82.3 |
| C3 partner is background/fragment | 32 | 10.1 |
| A no cluster in U and/or V | 13 | 4.1 |
| D X main track unmatched | 9 | 2.8 |
| B clusters exist but none is main | 2 | 0.6 |

per-plane status (U / V): `ok` 282/272, `partner_bg` 16/21, `no_cluster` 10/3,
`main_unmatched` 0/10, `no_main` 0/2, X unmatched 9/9.

### Aggregates over 60 cats each (~18.5k X main tracks per group)

| category | OLD 1-60 | TRAIN 400-459 | NEW 623-682 |
|---|---|---|---|
| MATCHED_3PLANE | 76.3% | 75.4% | **74.8%** |
| C3 partner is background/fragment | 11.3% | 11.2% | 11.0% |
| D X main track unmatched | 6.2% | 6.5% | 6.8% |
| A no cluster in U and/or V | 5.1% | 5.5% | 5.9% |
| C1 main exists but unmatched | 0.6% | 0.7% | 0.8% |
| B clusters exist but none is main | 0.6% | 0.7% | 0.7% |
| (`match_id != -1`, code's own definition) | 93.8% | 93.5% | 93.2% |

Note the strong U/V asymmetry inside category C1/A: the V plane fails with
`main_unmatched` (916 cases in the new cats) far more often than U (72), and U fails
with `no_cluster` more often than V. Both are consequences of the algorithm being
sequential (U first, then V) and of the V+X bug (§6.7).

### Per cat, relative to the 400 generated ES events (median of full 10-file cats)

| | OLD | TRAIN | NEW |
|---|---|---|---|
| X main tracks | 311 (77.8%) | 310 (77.5%) | 314 (78.6%) |
| `match_id != -1` | 294 (73.5%) | 293 (73.2%) | 294 (73.5%) |
| **3-plane main-track** | **240 (60.0%)** | **235 (58.8%)** | **238 (59.5%)** |

---

## 5. Energy and position dependence

### Category vs true electron energy (new cats 623-682, row %)

| E_true [MeV] | N | MATCHED | C3 wrong partner | A no cluster | D unmatched | B |
|---|---|---|---|---|---|---|
| 0-5 | 2860 | 63.8 | 0.2 | 15.2 | 19.4 | 1.3 |
| 5-7.5 | 3387 | 81.6 | 1.9 | 6.2 | 8.9 | 0.9 |
| 7.5-10 | 3172 | 81.5 | 5.1 | 5.5 | 6.4 | 0.6 |
| 10-15 | 4806 | 78.3 | 12.9 | 3.9 | 3.2 | 0.5 |
| 15-20 | 2473 | 72.3 | 20.9 | 2.9 | 2.2 | 0.4 |
| 20-25 | 1312 | 67.0 | 28.0 | 2.4 | 1.1 | 0.2 |
| 25-30 | 509 | 63.3 | 34.2 | 1.4 | 0.6 | 0.0 |
| >30 | 362 | 52.5 | **44.5** | 1.4 | 1.1 | 0.0 |

The old cats give the same shape (64.8% / 83.8% / 79.8% / 72.7% / 66.8% / 63.3% /
56.9% matched in 5-MeV-wide bins), confirming again that nothing changed.

Two opposite regimes:

* **below ~7 MeV**: the loss is "no induction cluster" (A + D = 35% at <5 MeV) -
  the electron's U/V cluster is below `energy_cut`;
* **above ~10 MeV**: the loss is the wrong partner (C3), growing to 44% above 30 MeV -
  longer tracks fragment into several induction clusters and the earliest fragment
  wins the match. **This preferentially removes the most informative, highest-energy
  electrons from the direction fit.**

### Position dependence (new cats)

| \|x_drift\| [cm] | N | 3-plane eff |
|---|---|---|
| 0-25 | 1126 | 80.2% |
| 25-50 | 1342 | 80.0% |
| 50-100 | 2638 | 79.7% |
| 100-150 | 2718 | 76.4% |
| 150-200 | 2584 | 75.5% |
| 200-250 | 2622 | 72.9% |
| 250-300 | 2570 | 71.8% |
| 300-350 | 2589 | 68.1% |
| >350 | 692 | 71.1% |

A clean ~12-point fall with drift distance, consistent with attenuation/diffusion
weakening the induction signal (more sub-threshold and more fragmented clusters).

| distance to APA edge in z [cm] | N | 3-plane eff |
|---|---|---|
| 0-2 | 166 | 67.5% |
| 2-5 | 429 | 73.0% |
| 5-10 | 803 | 75.8% |
| 10-20 | 1578 | 75.8% |
| 20-40 | 3209 | 74.7% |
| 40-60 | 3426 | 73.3% |
| 60-80 | 3268 | 74.5% |
| 80-116 | 6002 | 75.7% |

Only the first 2 cm from an APA boundary show a visible (-8 point) drop; APA
edges/gaps are **not** a leading cause. There is no significant dependence on y.

---

## 6. The rules responsible, with code references

**6.1 The main-track flag is assigned independently in each view**, as the *most
energetic MARLEY cluster of that event in that view*
(`src/app/make_clusters.cpp:295-315`):

```cpp
for (auto& cluster : clusters) {
    if (cluster.get_true_label() != "marley") continue;   // make_clusters.cpp:296
    float energy = cluster.get_total_energy();
    if (energy > max_energy) { max_energy = energy; main_cluster = &cluster; }
}
main_cluster->set_is_main_cluster(true);                  // make_clusters.cpp:315
```

It is truth-seeded (MARLEY label from backtracking) but *energy-ranked per plane*, so
when an electron splits into two induction clusters, only the more energetic one is
"main" - and the other one is a perfectly legitimate matching partner as far as the
matching code is concerned. This is the root of category **C3**.

**6.2 The energy cut is applied per view, after the main flag is set**
(`make_clusters.cpp:336-348`), with different conversion factors
(`parameters/conversion.dat`: collection 3600, induction 900 ADC/MeV):

```cpp
cluster_energy_mev = cluster.get_total_charge() / adc_to_energy_factor_{collection,induction};
if (cluster_energy_mev >= energy_cut) accepted_clusters.push_back(cluster);
else                                  discarded_clusters.push_back(cluster);
```

A main-track cluster can therefore end up in `discarded/`, where the matching never
looks (it reads only the `clusters/` directory) and where the image generator never
looks either (`python/app/generate_cluster_arrays.py:688`). This is the root of
categories **A**, **B** and most of **D**.

**6.3 Only X main-track clusters seed a match** (`match_clusters.cpp:263, 310, 360`),
which is why no background X cluster ever carries a `match_id`.

**6.4 The gates are: time overlap, same event, same APA** - no geometry at all
(`match_clusters.cpp:286-298` for U, `333-345` for V):

```cpp
if (u_time_range.first > x_time_range.second + time_tolerance_ticks_tdc) break;   // :286
if (!timesOverlap(u_time_range, x_time_range, time_tolerance_ticks_tdc)) continue;// :287
if (u_event != x_event) continue;                                                 // :290
if (u_apa != x_apa) continue;                                                     // :298
```

with `time_tolerance_ticks` read from the cat JSON (`match_clusters.cpp:117-118`;
value 10 TPC ticks = **320 TDC ticks**, not the +-5000 of the old doc), and
`timesOverlap` comparing the full [first TP start, last TP start+ToT] ranges
(`match_clusters.cpp:40-62`). `are_compatibles()`, which the doc describes as the
5 cm geometric test, is a stub that returns true whenever the three clusters are on
the same APA (`src/clusters/MatchClusters.cpp:59-70`); the y/x projection code
(`match_with_true_pos`) is never called from the app.

**6.5 Ambiguity is resolved by "first in the scan order"** (`match_clusters.cpp:301-303`
and `348-350`), where the scan order is the stable sort by
(earliest TDC time, event, cluster_id) applied at `match_clusters.cpp:136-211`:

```cpp
if (x_to_u_match.find(x_id) == x_to_u_match.end()) x_to_u_match[x_id] = j;   // first wins
```

So among all induction clusters of the same event/APA that overlap the X cluster in
time, the one that **starts earliest** is taken, with no regard for energy or for the
main-track flag. Measured on the regenerated subset: whenever the main induction
cluster is among the candidates it is the earliest in 92/93 (U) and 87/91 (V) cases -
but in the remaining cases, and in the 11% C3 population across cats, an earlier
fragment steals the match. Typical stolen matches (dt = start-time difference):

```
V event 33: picked E=4.5 MeV (marley, not main) at t=41824, main E=4.8 MeV at t=42816, dt=992 TDC
U event  5: picked E=3.9 MeV (marley 0.67)     at t= 2624, main E=12.4 MeV at t= 3424, dt=800 TDC
V event  5: picked E=3.2 MeV (marley)          at t= 2752, main E=14.1 MeV at t= 3424, dt=672 TDC
```

Note these are all *inside* the 320 TDC tick tolerance only because `timesOverlap`
compares ranges, not starts: a long X cluster overlaps both fragments.

**6.6 Partial (2-plane) matches receive a `match_id` too**
(`match_clusters.cpp:386-412` builds them, `:517-519` gives the X cluster an id).
They are indistinguishable downstream from complete matches, and they can never
satisfy the pipeline's 3-plane requirement.

**6.7 BUG - the V+X partial match never reaches the V cluster**
(`match_clusters.cpp:521-533`). For a V+X match the pair is pushed as
`{clusters_v[k], clusters_x[i]}` (`:403`), so `c1` is the V cluster and the test

```cpp
if (c1_is_u || (!c1_is_x && c2.get_tps()[0]->GetView() == "X")) {   // :521 - true for V+X too
    int u_id = c1_is_u ? c1.get_cluster_id() : c2.get_cluster_id(); // :523 - this is the X id!
    u_cluster_to_match[u_id] = match_id;                            // :524 - written into the U map
    match_type_map[match_id] = 2;                                   // :526 - labelled U+X
```

routes V+X into the U+X branch and stores the **X cluster's own id** in the U map
(cluster ids are unique across views, so the entry matches nothing). The `else` branch
that would handle V+X is dead code. Consequence: the X cluster carries a `match_id`
that exists in **neither** U nor V. Verified on the regenerated subset: 10 such
"orphan" match_ids, and in all 10 there was no U candidate but a V candidate existed.
These show up in the npz analysis as the `U:no_cluster | V:main_unmatched` pattern
(10 cases in cat000623, 916 `V main_unmatched` cases across the 60 new cats).

**6.8 BUG - `match_type` is hard-coded** (`src/clusters/Clustering.cpp:841`):

```cpp
match_type = 3;  // Currently only 3-plane matches
```

The `match_type_map` computed in the app is never passed to
`write_clusters_with_match_id`, so every matched cluster is written as type 3.
Verified: all 105 matched X clusters of the test subset have `match_type == 3`,
although 21 of them are 2-plane matches. This is why the pipeline has to
intersect `match_id` sets instead of simply requiring `match_type == 3`.

**6.9 Side effect on the images**: for U/V clusters the APA flip/geometry is taken
from the matched X cluster and falls back to a default when `match_id == -1`
(`generate_cluster_arrays.py:787-794`), so unmatched induction clusters may also be
drawn with the wrong orientation.

---

## 7. Is the loss new? No.

| group | production date | cats | X main tracks | 3-plane main match | `match_id != -1` |
|---|---|---|---|---|---|
| old | 2025-11-19 | 1-60 | 18281 | **76.3%** (median/cat 77.1%) | 93.8% |
| training | 2025-11-19 | 400-459 | 18496 | **75.4%** (median/cat 76.1%) | 93.5% |
| new | 2026-08-24 | 623-682 | 18881 | **74.8%** (median/cat 76.2%) | 93.2% |

Per-cat spread is large (57-84%), which is why a single cat (623 at 82.3%, 450 at
67.3%) can look much better or worse than the campaign.

Code-wise, `git diff 51ef160(2025-11-18) HEAD -- src/app/match_clusters.cpp` contains
only `#include` renames; `make_clusters.cpp` gained an APA filter option and a
corrupted-output check; `Clustering.cpp` was refactored for speed (PR #15). The
matching algorithm and all JSON parameters (`json/cats/cat_000001.json` and
`cat_000623.json` are identical: `tick_limit 3, channel_limit 2, min_tps_to_cluster 2,
tot_cut 3, energy_cut 3.0, time_tolerance_ticks 10`) are the same in both productions.

**What the user remembers as "matching working very well" is the code's own metric
(`match_id != -1`, 93-94%, and MARLEY purity), which is still true. The pipeline's
3-plane main-track requirement has always been at ~75%.**

---

## 8. Direct check on regenerated intermediates (one training cat)

The intermediate `tps/`, `clusters/` and `matched_clusters/` products were pruned for
all finished cats, so the first 4 ES files of **training cat 450** were reprocessed
(condor job 15918476, `-bt -ab -mc -mm`) with **unchanged parameters** into

```
/eos/project-e/ep-nu/evilla/sn-online-pointing/matching-study-scratch/cat000450study/
```

(config `python/ana_evilla/matching_study_cat000450.json`, submit files
`condor/evilla/matching_study_cat450.{sub,args}`; nothing under `sn-burst-samples/`
was written).

**Validation** - the regeneration is faithful and the offline emulator is exact:

| file | X main (prod / regen) | 3-plane (prod npz) | 3-plane (emulator on regen) |
|---|---|---|---|
| es_000000 | 34 / 34 | 18 | 18 |
| es_000001 | 29 / 29 | 18 | 18 |
| es_000002 | 33 / 33 | 20 | 20 |
| es_000003 | 33 / 33 | 21 | 21 |
| total | 129 / 129 | 77 (59.7%) | 77 (59.7%) |

With the intermediates in hand, the two dominant causes can be measured directly:

* **fate of the induction main-track cluster** of each X main track's event:
  accepted 76.7% (U) / 76.0% (V); **removed by `energy_cut` 23.3% (U) / 23.3% (V)**;
  clusters present in the event but none flagged main 0% (U) / 0.8% (V); no cluster at
  all in the event 0% / 0%.
  The removed main clusters have energies just below the cut:
  median 2.27 MeV (U), 2.44 MeV (V), 90th percentile 2.84/2.90, max 2.97.
* **rank of the main induction cluster in the scan order**: rank 0 in 92/93 (U) and
  87/91 (V) of the cases where it is a candidate at all - i.e. when the electron's
  main induction cluster survives the energy cut, the "first wins" rule usually picks
  it; the C3 losses come from the events where a fragment starts earlier.

---

## 9. Rule and parameter scans (offline, nothing applied)

All numbers below are from `emulate_matching_rules.py` on the regenerated 4-file
subset of cat 450 (129 X main tracks, baseline 59.7%). This subset sits below the
campaign median, so treat the *deltas*, not the absolute values, as the message.

### 9.1 Time tolerance (`time_tolerance_ticks`, currently 10 TPC ticks)

| tolerance [TPC ticks] | no U candidate | current rule | main-preferring rule |
|---|---|---|---|
| 1 | 59.7% | 24.8% | 26.4% |
| **10 (production)** | 26.4% | **59.7%** | 62.8% |
| 50 | 24.8% | 59.7% | 64.3% |
| 200 | 24.0% | 58.9% | 65.1% |
| 1000 | 21.7% | 58.9% | 67.4% |
| 5000 (value in the old doc) | 21.7% | **55.0%** | 68.2% |

Widening the window **hurts** with the current rule (more background clusters win the
"first in time" race) and only helps if the partner choice is fixed first. Narrowing
it is catastrophic. **The current value of 10 is close to optimal for the current
algorithm** - the window is not the problem.

### 9.2 Ambiguity resolution rule (code change, quantified here for reference)

| rule | 3-plane fraction |
|---|---|
| current: first cluster in time order | 59.7% |
| prefer the main-track candidate | 62.8% |
| take the most energetic candidate (truth-agnostic, equivalent here) | 62.8% |
| take the candidate closest in time centroid | 57.4% |

On this subset the gain is small because most losses here are energy-cut losses.
Across the full 60-cat samples the npz-level estimate of the same fix is much larger:
**11.8% of all X main tracks** are "X already matched, both planes do have a
main-track cluster, but the wrong partner was picked", i.e.

| | OLD | TRAIN | NEW |
|---|---|---|---|
| now | 76.3% | 75.4% | 74.8% |
| with a main/most-energetic-preferring partner rule | **88.1%** | **87.3%** | **86.6%** |

(The ceiling with a perfect partner choice - "a main-track cluster exists in both
planes" - is 88.7% / 87.9% / 87.0%; the rest needs the energy cut.)

### 9.3 `energy_cut` (currently 3.0 MeV, JSON parameter)

Because the in-clustering ADC cut is disabled (`Clustering.cpp:352-361`, commented
out) and the cut is applied only when splitting accepted/discarded, lowering
`energy_cut` is *exactly* equivalent to moving clusters from `discarded/` back into
`clusters/` - which is what the emulator does.

Applied to the induction planes only (keeps the denominator fixed at 129):

| energy_cut U/V | no U candidate | current rule | main-preferring rule |
|---|---|---|---|
| **3.0 (production)** | 26.4% | **59.7%** | 62.8% |
| 2.5 | 16.3% | 72.1% | 76.7% |
| 2.0 | 9.3% | **76.7%** | 82.9% |
| 1.5 | 3.1% | 75.2% | 89.1% |
| 1.0 | 2.3% | 73.6% | 89.9% |

Applied to all planes, as the single JSON parameter actually does:

| energy_cut | X main tracks | clusters U/V/X | 3-plane, current rule | vs the 129 baseline mains |
|---|---|---|---|---|
| **3.0** | 129 | 124 / 157 / 161 | 77 (**59.7%**) | 59.7% |
| 2.5 | 134 | 212 / 305 / 250 | 94 (70.1%) | 72.9% |
| 2.0 | 140 | 447 / 597 / 637 | 106 (75.7%) | 82.2% |
| 1.5 | 142 | 873 / 1083 / 1161 | 106 (74.6%) | 82.2% |

Lowering the cut both **adds** reconstructable electrons (129 -> 140 X main tracks at
2.0) and **recovers** their induction partners (77 -> 106 three-plane matches, +38%),
but multiplies the number of clusters by ~4 - almost all of them radiological
background - which is the cost paid downstream (bigger npz, more background clusters
to reject in channel tagging, slower volume building). With the current
first-in-time rule the optimum is around 2.0-2.5; going below 1.5 starts to lose
again because background clusters win the race.

---

## 10. Proposed parameter changes (NOT applied)

Nothing here has been applied. `json/cats/*.json`, `parameters/*.dat` and `src/` are
untouched.

### P1 - `energy_cut`: 3.0 -> 2.5 (conservative) or 2.0 (aggressive)

* **File/key**: `json/cats/cat_XXXXXX.json`, `"energy_cut"` (and the same key in
  `condor/evilla/proc_wave.sh`, which writes those JSONs).
* **Current value**: `3.0` (MeV, all three views; corresponds to 10800 ADC on
  collection and 2700 ADC on induction via `parameters/conversion.dat`).
* **Proposed value**: `2.5`, or `2.0` if the extra background is acceptable.
* **Expected effect** (measured, §9.3): at 2.5, +22% more 3-plane matched ES
  electrons (77 -> 94 on the test subset, 59.7% -> 70.1%); at 2.0, +38%
  (77 -> 106, 59.7% -> 75.7%). Roughly +4% (2.5) / +8.5% (2.0) more X main tracks as
  a bonus, since sub-3-MeV electrons re-enter the sample.
* **Risks**: cluster multiplicity grows ~2x (2.5) or ~4x (2.0) in every plane, nearly
  all radiological background. This means larger cluster-image sets, a harder
  background-rejection job for the channel tagger, more wrong-partner matches
  (the ambiguity rule of §6.5 degrades as candidates multiply - visible in §9.3 where
  the current rule stops improving below 2.0), and longer processing/more disk.
  Re-training of the CT and ED networks would be required, and the low-energy
  clusters that come back are the noisiest ones (2-3 MeV, few TPs).
* **Recommendation**: if only one parameter is to be changed, `2.5` gives most of the
  gain at half the background cost of `2.0`.

### P2 - `time_tolerance_ticks`: keep at 10

* **File/key**: `json/cats/cat_XXXXXX.json`, `"time_tolerance_ticks"`.
* **Current value**: `10` TPC ticks (= 320 TDC ticks).
* **Proposed value**: **unchanged**. Explicitly *not* the +-5000 ticks of
  `docs/MATCHING_CRITERIA_AND_HANDLING.md`: at 5000 the 3-plane fraction *drops* to
  55.0% (§9.1).
* **Expected effect of changing it**: any increase costs efficiency under the current
  first-in-time rule; a decrease is much worse (24.8% at 1 tick).
* **Note**: if the partner-selection rule is ever fixed (P3), the window becomes
  worth revisiting - with a main-preferring rule, 200-1000 ticks would add
  +2 to +5 points.

### P3 - code change (not a parameter): prefer the main-track / most energetic partner

* **Where**: `src/app/match_clusters.cpp:301-303` and `:348-350` - instead of keeping
  the first candidate, keep the best one (main-track flag if present, otherwise
  highest `total_energy`).
* **Expected effect**: +11.8 points on the full samples (74.8% -> 86.6% for the new
  cats, §9.2), concentrated in the high-energy electrons that matter most for
  pointing (C3 is 44% of losses above 30 MeV).
* **Risks**: `is_main_cluster` is truth-derived, so using it directly would make the
  matching truth-dependent (unusable on data); the truth-agnostic proxy
  "most energetic candidate" gives the same result on this sample and should be used.
  Purity: the wrong partners being replaced are 98% MARLEY clusters of the same
  event, so this is about picking the right fragment, not about background rejection;
  a small risk of picking a genuinely different MARLEY particle (e.g. a de-excitation
  gamma) remains.

### P4 - code fixes (not parameters), independent of the above

* `src/app/match_clusters.cpp:521-533`: route V+X partial matches to the V map
  (currently they are labelled U+X and the V partner never gets the `match_id`).
  Effect: ~8% of X main tracks stop carrying a match_id that exists in no other
  plane. It does **not** by itself recover 3-plane matches (those events have no U
  cluster at all), but it makes `match_id` bookkeeping and any 2-plane fallback
  correct.
* `src/clusters/Clustering.cpp:841`: write the real `match_type` instead of the
  constant 3, so the pipeline can select genuine 3-plane matches directly.
* `docs/MATCHING_CRITERIA_AND_HANDLING.md`: update - the documented +-5000 tick window
  and 5 cm geometric cuts do not correspond to the code
  (`MatchClusters.cpp:59-70` is a same-APA stub).

### P5 - if a 2-plane fallback is acceptable downstream

~8% of X main tracks are genuine U+X matches with a main-track U cluster and no V
cluster at all. If the direction network could run on X+U (or X+V) pairs, most of
these would be recovered without touching any cut. This is a pipeline design
question, not a parameter.

---

## 11. Files produced by this study

| path | content |
|---|---|
| `python/ana_evilla/three_plane_matching_efficiency.py` | npz-level classifier (new) |
| `python/ana_evilla/emulate_matching_rules.py` | ROOT-level matching emulator and scans (new) |
| `python/ana_evilla/matching_study_cat000450.json` | scratch regeneration config (new) |
| `condor/evilla/matching_study_cat450.{sub,args}` | the single condor job used (new) |
| `docs/three_plane_matching_efficiency_study.md` | this document |
| `/eos/.../matching-study-scratch/cat000450study/` | regenerated tps/clusters/matched_clusters (scratch) |
| `/eos/.../matching-study-scratch/results/` | json/npz outputs of all the tables above |

No existing file was modified and nothing was deleted.
