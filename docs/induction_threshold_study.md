# A lower clustering `energy_cut` on the induction planes only

**Question answered.** The 3.0 MeV clustering cut on the **collection (X)** plane stays.
What happens if only the **induction (U, V)** planes are clustered down to 2.5 or 2.0 MeV?
How many extra ES electrons enter the pointing pipeline, and how many of them survive the
downstream analysis cuts (reco X energy > 5 MeV, channel tagging CT v80 >= 0.80)?

Short answer, measured on the 50 mixture-dev cats (623-672), per 330-generated-ES burst:

| | reference (3.0 all views) | X 3.0 / ind 2.5 | X 3.0 / ind 2.0 |
|---|---|---|---|
| ES events entering the pipeline (3-plane, main-track partners) | **219.3** | **233.8** (+6.6%) | **242.9** (+10.8%) |
| of which recovered (new) | - | 14.5 | 23.6 |
| recovered events surviving E_X >= 5 MeV | - | 7.6 (52%) | 12.6 (53%) |
| recovered events also passing CT v80 >= 0.80 (measured) | - | **3.3** | **5.2** |
| deployed selection (CT >= 0.8, E >= 5), ES part | 60.6 | 63.9 (+5.4%) | 65.8 (+8.6%) |

So **about half of the recovered events are lost again at E > 5 MeV, and the CT then keeps
only ~43% of the survivors**: of 14.5 (2.5 MeV) / 23.6 (2.0 MeV) recovered ES electrons
per burst, **3.3 / 5.2 reach the deployed selection**, i.e. 23% / 22% of them. The
"perfect-CT" scenarios keep essentially all of them (see §5).

Author: analysis run on 2026-09-28 (lxplus/HTCondor, E. Villa's samples).
Nothing existing was modified: the code change is additive and **default-off**, all new
products live in new folders with a distinct suffix, and no existing product, model,
report or cat in 400-621 was touched.

---

## 1. Where `energy_cut` acts, and the per-view override

### 1.1 How the single cut is applied today

`energy_cut` (MeV, JSON) is used in **one** place that matters,
`src/app/make_clusters.cpp`, and only when splitting `clusters/` from `discarded/`:

* the in-clustering ADC cut is **disabled** (`src/clusters/Clustering.cpp:352-361`,
  commented out), so the `adc_cut` vector passed to `make_cluster()` has no effect;
* after the main-track flag is set (per view, most energetic MARLEY cluster,
  `make_clusters.cpp:295-315`), each cluster's charge is converted to MeV with a
  **per-view** factor from `parameters/conversion.dat`
  (`adc_to_energy_factor_collection = 3600`, `adc_to_energy_factor_induction = 900`
  ADC/MeV, i.e. 10800 / 2700 ADC at 3.0 MeV) and compared with the **single**
  `energy_cut` for all three views;
* `match_clusters` reads only the `clusters/` tree (`match_clusters.cpp:239-242` reads
  `discarded/` but never matches it), and `generate_cluster_arrays.py` likewise. So
  lowering the cut is *exactly* equivalent to moving clusters from `discarded/` back
  into `clusters/`, and the induction threshold is the only thing that decides whether
  an X main track can find a main-track partner at all.

Nothing else in `src/` or `python/` applies `energy_cut` as a physics cut; the other
occurrences (`src/io/InputOutput.cpp`, `python/app/create_volumes.py`,
`python/app/generate_cluster_arrays.py`, `python/ana/analyze_volumes.py`) only use it to
**compose folder names** (`..._e3p0`).

### 1.2 The change (additive, default-off)

New optional JSON keys, resolved from generic to specific, each defaulting to the
previous level, so that a JSON without them behaves exactly as before:

```
energy_cut              -> all three views          (unchanged meaning)
energy_cut_induction    -> U and V                  (default = energy_cut)
energy_cut_collection   -> X                        (default = energy_cut)
energy_cut_u / _v / _x  -> that single view         (default = induction / collection)
```

Diff (`src/app/make_clusters.cpp`, the only file changed):

```diff
-    float adc_integral_cut_col = energy_cut * ...factor_collection");
-    float adc_integral_cut_ind = energy_cut * ...factor_induction");
+    auto read_float_key = [&j](const std::string& key, float fallback) {
+        if (!j.contains(key)) return fallback;
+        try { return j.at(key).get<float>(); }
+        catch (const std::exception&) { return static_cast<float>(j.at(key).get<double>()); }
+    };
+    float energy_cut_induction  = read_float_key("energy_cut_induction",  energy_cut);
+    float energy_cut_collection = read_float_key("energy_cut_collection", energy_cut);
+    float energy_cut_u = read_float_key("energy_cut_u", energy_cut_induction);
+    float energy_cut_v = read_float_key("energy_cut_v", energy_cut_induction);
+    float energy_cut_x = read_float_key("energy_cut_x", energy_cut_collection);
+    std::map<std::string, float>  energy_cut_by_view  = {{"U",energy_cut_u},{"V",energy_cut_v},{"X",energy_cut_x}};
+    std::map<std::string, double> adc_per_mev_by_view = {{"U",adc_per_mev_ind},{"V",adc_per_mev_ind},{"X",adc_per_mev_col}};
...
-                    if (APA::views.at(iView) == "X")  cluster_energy_mev = charge / factor_collection;
-                    else                              cluster_energy_mev = charge / factor_induction;
-                    if (cluster_energy_mev >= energy_cut) {
+                    const std::string& view_name = APA::views.at(iView);
+                    float cluster_energy_mev = cluster.get_total_charge() / adc_per_mev_by_view.at(view_name);
+                    if (cluster_energy_mev >= energy_cut_by_view.at(view_name)) {
```

plus: the per-view thresholds are written into the `clustering_metadata` tree as new
branches `energy_cut_u/_v/_x`, the per-view ADC cuts passed to `make_cluster()` are now
`{U, V, X}` instead of `{ind, ind, col}`, and a `PER-VIEW energy cuts ACTIVE` line is
logged when the new keys are in use. Folder-name composition was **not** touched
(`..._e3p0` still refers to `energy_cut`), so the variant products are separated by an
explicit `clusters_folder` / `matched_clusters_folder` / `cluster_images_folder` in the
cat JSON, with the suffixes `_indcut25` and `_indcut20`.

**Closure test of the default-off property**: re-running the chain on 10 cats with the
new binary and a JSON *without* the new keys (suffix `_indcutref`) reproduces the
existing `_matchfix` ES cluster images — see §6.

Full diff: `git diff src/app/make_clusters.cpp` on branch `snop-fixes-2026-09`.

---

## 2. What was produced, and how

Regenerated for all **50 dev cats 623-672**, from `tps_bg/` (backgrounds already merged),
steps `-mc -mm -gi` (clustering -> matching -> cluster images; the matcher's default
most-energetic-partner rule of `docs/three_plane_matching_fix_validation.md` is used
throughout):

* `..._clusters/matched_clusters/cluster_images_tick3_ch2_min2_tot3_e3p0_indcut25`
  (`"energy_cut": 3.0, "energy_cut_induction": 2.5`)
* `..._indcut20` (`"energy_cut_induction": 2.0`)

Only the **ES input files** were reprocessed (`skip_files: 100, max_files: 9` selects
tpstream indices 100-108 = `es_000000..es_000008`), because the pipeline's ES burst
budget is the first 330 **generated** ES events = files 1-8 complete + events 1-10 of
file 9. This keeps the footprint at 3.9 GB / 4200 files.

**Volume images were not regenerated**: they are X-plane only and are built for *every*
X main-track cluster whether it is matched or not
(`python/app/create_volumes.py:773`), and the X accepted-cluster set is unchanged
because the collection cut stays at 3.0 MeV. The existing `_matchfix` X volume image of
the same event is therefore the *correct* CT input for a recovered event, and was used
as such (matched by event number).

JSONs: `json/mixture_dev/indcut/cat_0006NN_{indcut25,indcut20,indcutref}.json`,
`json/mixture_dev/ccprobe/`.
Condor: `refactor-snop-pipeline/condor/mixture_dev/proc_indcut_cats623to672.{sub,args}`,
`indcut_infer.{sub,args}`, `indcut_fits.{sub,args}`, `ccprobe.{sub,args}`,
`proc_indcutref.{sub,args}`, `indcut_validate.sub`; logs under
`refactor-snop-pipeline/condor/logs/mixture_dev/{indcut,indcut_infer,indcut_fits,ccprobe}`.
Analysis products: `/eos/project-e/ep-nu/evilla/sn-online-pointing/pipeline-dev/induction_threshold/`.
Scripts: `python/ana_evilla/induction_threshold_{scan,join_ref,recovered,ccprobe,report}.py`
(this repo) and `refactor-snop-pipeline/python/ana/induction_threshold_{infer,fits,validate}.py`.

---

## 3. Event counts per burst (50 cats, 330-ES budget)

Selection replicated exactly from `refactor-snop-pipeline/python/lib/sample_loader.py`
(`load_all_planes=True`, `event_budget_mode='generated'`, `events_per_file=40`): an X
main-track cluster with a `match_id` that is also carried by a **main-track** cluster in
U *and* in V.

| quantity (per burst, mean of 50 cats) | ref 3.0 | ind 2.5 | ind 2.0 |
|---|---|---|---|
| X main tracks in the budget (denominator, identical by construction) | 261.2 | 261.2 | 261.2 |
| X main tracks with any `match_id` | 244.1 | 252.8 | 257.6 |
| **3-plane with main-track partners (enter the pipeline)** | **219.3** | **233.8** | **242.9** |
| 3-plane efficiency w.r.t. X main tracks | 83.9% | 89.5% | 93.0% |
| recovered (present only with the lower cut) | - | **14.5** | **23.6** |
| lost (present only at 3.0) | - | **0.00** | **0.00** |

**Nothing is ever lost.** Lowering the induction cut can only *add* candidates below
3.0 MeV, and the matcher keeps the most energetic candidate, so a partner that won at
3.0 MeV still wins. Verified directly: for every event common to reference and variant
the X, U and V cluster images are **byte-identical** (checked on 40 common events in
each of 20 cats, 0 mismatches). The two products differ only by the added events.

The denominator is unchanged because the X cut is unchanged - unlike the all-plane scan
of `three_plane_matching_efficiency_study.md` §9.3, where lowering `energy_cut`
everywhere also added ~4-8% more X main tracks. The gain here is therefore smaller than
the +22% / +38% quoted there (that number was an all-plane change measured on a 4-file
subset of one training cat with the old first-in-time partner rule and a baseline of
59.7%; with the matcher fix the baseline is already 83.9%, so there is much less left to
recover).

### Match purity (truth-based, per burst)

| | ref 3.0 | ind 2.5 | ind 2.0 |
|---|---|---|---|
| X mains with a U partner | 232.7 | 245.8 | 253.8 |
| ... partner is the U **main** cluster | 229.2 (98.5%) | 241.5 (98.2%) | 248.9 (98.1%) |
| ... partner is **background** (not MARLEY) | 2.8 (1.2%) | 2.9 (1.2%) | 3.1 (1.2%) |
| X mains with a V partner | 235.8 | 247.0 | 254.4 |
| ... partner is the V **main** cluster | 231.7 (98.3%) | 241.9 (97.9%) | 248.2 (97.6%) |
| ... partner is **background** | 0.9 (0.4%) | 1.0 (0.4%) | 1.1 (0.4%) |

The wrong-partner rate grows by only 0.3-0.7 points and the **background-partner rate
does not grow at all** (1.2% U, 0.4% V at every threshold). The most-energetic-partner
rule protects against the multiplying low-energy background clusters, because a 2-3 MeV
radiological never outranks the electron's own induction cluster when that cluster
exists. Events whose partner is not the main-track cluster are rejected by the loader
anyway.

## 4. The recovered events

### 4.1 X-energy spectrum and survival

| | ind 2.5 | ind 2.0 |
|---|---|---|
| recovered / burst | 14.52 | 23.62 |
| median reco X energy | 5.30 MeV | 5.39 MeV |
| E_X >= 3 MeV | 98.9% | 98.9% |
| **E_X >= 5 MeV** | **7.60 / burst (52.3%)** | **12.58 / burst (53.3%)** |
| E_X >= 5 **and** CT v80 >= 0.80 (measured) | **3.30 / burst** | **5.24 / burst** |

Spectrum of recovered events (counts over 50 cats), reco X energy:

| E_X [MeV] | 0-3 | 3-4 | 4-5 | 5-6 | 6-7 | 7-8 | 8-10 | 10-12 | 12-15 | 15-20 | 20-30 | >30 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| ind 2.5 | 8 | 239 | 99 | 59 | 47 | 47 | 75 | 57 | 41 | 37 | 16 | 1 |
| ind 2.0 | 13 | 374 | 165 | 91 | 86 | 81 | 128 | 98 | 63 | 57 | 24 | 1 |

They are strongly concentrated at **3-5 MeV** (46% at 2.5, 46% at 2.0): a low-energy
electron makes a low-energy induction cluster. That is exactly the population the
E > 5 MeV cut removes.

The CT survivors were obtained two ways, which agree: applying the reference events' own
CT pass rate per energy bin to the recovered spectrum predicts 3.60 / 6.07 survivors per
burst, while **running CT v80 on the recovered events' actual X volume images gives
3.30 / 5.24**. The direct number is used everywhere above; the small over-estimate of the
bin-extrapolation is the 8-12 MeV effect of §4.2.

### 4.2 CT v80 pass rate and ED v63 direction quality, recovered vs standard

CT v80 was run **for real** (not extrapolated) on the recovered events' existing X
volume images; ED v63 was run on their **new** three-plane cluster images. Standard
(reference) events use the scores and directions stored in the dev-pipeline products
`pipeline-dev/ed_retrain/R4_v63_ownpdf_matchfix/` (scenario 4 holds every loaded event).

ind 2.0 (ind 2.5 is statistically identical, see below):

| E_X [MeV] | n rec | n std | CT>=0.8 rec | CT>=0.8 std | <cos(reco,e-)> rec | std | <cos(reco,burst)> rec | std |
|---|---|---|---|---|---|---|---|---|
| 3-4 | 374 | 546 | 93.9% | 95.4% | 0.325 | 0.278 | 0.282 | 0.271 |
| 4-5 | 165 | 711 | 93.9% | 94.5% | 0.501 | 0.376 | 0.461 | 0.347 |
| 5-6 | 91 | 781 | 87.9% | 90.9% | 0.390 | 0.509 | 0.370 | 0.490 |
| 6-7 | 86 | 781 | 79.1% | 82.2% | 0.482 | 0.546 | 0.465 | 0.529 |
| 7-8 | 81 | 758 | 72.8% | 74.4% | 0.471 | 0.549 | 0.437 | 0.526 |
| 8-10 | 128 | 1476 | 38.3% | 53.5% | 0.555 | 0.566 | 0.547 | 0.551 |
| 10-12 | 98 | 1371 | 6.1% | 18.8% | 0.572 | 0.579 | 0.564 | 0.569 |
| 12-15 | 63 | 1594 | 0.0% | 4.1% | 0.493 | 0.594 | 0.481 | 0.588 |
| 15-20 | 57 | 1574 | 0.0% | 0.2% | 0.590 | 0.616 | 0.577 | 0.610 |
| **all** | 1181 | 10964 | 66.1% | 38.6% | 0.449 | 0.560 | 0.421 | 0.549 |
| **E>=5** | 629 | 9696 | 41.7% | 31.3% | 0.510 | 0.590 | 0.495 | 0.579 |

ind 2.5, the same two summary rows: all 726 events, CT 67.4%, cos(e-) 0.461,
cos(burst) 0.434; E>=5 380 events, CT 43.4%, cos(e-) 0.515, cos(burst) 0.497.

Three things to read off this table.

1. **The CT is not the problem at low energy, it is the problem at high energy.** The
   v80 pass rate at 0.80 falls from ~94% at 3-5 MeV to ~0% above 12 MeV, because the
   tagger's job is to separate low-energy ES from high-energy CC. The recovered events
   are low-energy, so their *inclusive* CT pass rate (66-67%) is much **better** than the
   standard events' (38.6%). Of the recovered events that pass E>5, the CT keeps 43.4%
   (2.5) / 41.7% (2.0); the ~57% it rejects are the ones above ~8 MeV.
2. **Bin by bin, recovered events are as taggable as standard ones** up to 8 MeV
   (94/94, 88/91, 79/82, 73/74%); above 8 MeV they are somewhat less taggable
   (38% vs 54%, 6% vs 19%) - a real population difference, on 100-200 events per bin.
3. **The ED direction quality of recovered events is comparable to standard events at
   the same energy**, and clearly better below 5 MeV (cos to the true electron 0.33/0.50
   vs 0.28/0.38 in the 3-4 and 4-5 MeV bins) - the standard sub-5-MeV population is
   itself contaminated by badly reconstructed events. In the 5-8 MeV bins the recovered
   events are 0.06-0.12 worse in cos; averaged over the E>=5 selection, 0.510 vs 0.590
   (electron) and 0.495 vs 0.579 (burst). So a recovered event is a somewhat weaker
   direction measurement, but a perfectly usable one: **v63 handles the newly accepted
   low-energy induction clusters without retraining**, which is expected since v63 was
   trained on a pool clustered at 2.0 MeV in all planes (e2p0), i.e. the lower induction
   cut moves the burst products *towards* its training conditions.


---

## 5. Net effect on pointing

Offline fits with the pipeline's own `select_electrons_from_run` +
`reconstruct_burst_direction` (emcee, 128 walkers x 500 steps, discard 100, uniform prior,
`prior_sigma_deg = 10`, `likelihood_kappa = 25`, seed 42 - the `ed_r3_eval.py` settings),
on the three event sets **ref / ind25 / ind20** built on top of the deployed
`R4_v63_ownpdf_matchfix` per-event products, with the recovered events carrying their own
ED v63 direction and CT v80 score.

Closure of that construction: re-running the study's ED v63 and CT v80 code on the
reference events reproduces the stored products **exactly** - over 670 events in cats
623-625, cos(my ED direction, stored ED direction) = 1.000000 for every event and
max |my CT score - stored CT score| = 0.0.

Tables used: `cosine_energy_pdf.npz` of the v63 model dir for the perfect-CT scenarios,
`cosine_energy_pdf_mixture_ctsel_global_flatcc.npz` for the deployed selection.
`theta68` is the cat-level containment
`degrees(arccos(quantile(cos(reco burst, true burst), 0.32)))` over the 50 cats, and the
interval is a 2000-sample **paired** bootstrap over cats (`RandomState(23)`), exactly as
`python/ana/ed_r3_report.py` does.

| scenario | set | mean n_selected | theta68 [deg] | median theta [deg] | d(theta68) vs ref [68% CI] |
|---|---|---|---|---|---|
| scenario 2 (true ES, E >= 3) | ref | 219.1 | 10.26 | 7.63 | - |
| | ind 2.5 | 233.4 | 10.39 | 7.46 | +0.13 [-0.16, +0.32] |
| | ind 2.0 | 242.4 | 10.23 | 7.91 | -0.03 [-0.18, +0.72] |
| scenario 6 (true ES, E >= 5) | ref | 193.9 | 10.10 | 7.71 | - |
| | ind 2.5 | 201.5 | 10.15 | 7.65 | +0.05 [-0.40, +0.26] |
| | ind 2.0 | 206.5 | 10.04 | 7.75 | -0.06 [-0.41, +0.38] |
| **deployed (CT >= 0.8, E >= 5)** | ref | 145.3 | 16.42 | 10.07 | - |
| | ind 2.5 | 148.6 | 16.39 | 9.83 | -0.02 [-0.58, +0.79] |
| | ind 2.0 | 150.5 | **14.88** | 10.18 | **-1.53 [-1.93, +0.64]** |

**Read this table conservatively.** With 50 cats the paired 68% intervals are 0.5-2 deg
wide, which is the same size as the effect we are looking for, and every interval above
spans zero. The honest statement is:

* the **event counts** change significantly and reproducibly (+6.6% / +10.8% into the
  pipeline, +5.4% / +8.6% into the deployed selection);
* the **pointing resolution does not measurably change** on 50 cats. The nominal
  -1.53 deg of the deployed selection at 2.0 MeV is the largest movement seen, but its
  68% interval reaches +0.64 deg, so it is not established. In the perfect-CT scenarios,
  where all recovered events are kept, the effect is flat to within +-0.15 deg.
* this is consistent with the arithmetic: adding 3.3-5.2 events of *somewhat worse*
  direction quality to a selection of 145 should improve the statistical part of the
  resolution by roughly sqrt(145/150) ~ 1.6%, i.e. ~0.2 deg on a 14-16 deg number -
  far below what 50 cats can resolve.

To resolve a change of this size the variants would need to be produced for several
hundred cats (the deployed 1000-cat campaigns reach ~0.3 deg intervals).

---

## 6. Cost

### 6.1 Cluster multiplicity (radiologicals)

Per burst (the 9 ES input files = 360 generated events), all clusters written to
`clusters/` and, separately, those with `marley_tp_fraction <= 0.5` (i.e. background,
overwhelmingly radiological):

| | ref 3.0 | ind 2.5 | ind 2.0 |
|---|---|---|---|
| U clusters (all) | 349.5 | 537.5 (x1.54) | 1026.6 (x2.94) |
| V clusters (all) | 420.9 | 694.8 (x1.65) | 1340.2 (x3.18) |
| X clusters (all) | 355.7 | 355.7 (x1) | 355.7 (x1) |
| **U background clusters** | **33.7** | **186.4 (x5.5)** | **638.7 (x19.0)** |
| **V background clusters** | **88.6** | **327.6 (x3.7)** | **936.4 (x10.6)** |
| X background clusters | 16.5 | 16.5 (x1) | 16.5 (x1) |

Per generated event that is 0.09 -> 0.52 -> 1.77 background U clusters and
0.25 -> 0.91 -> 2.60 background V clusters. The absolute numbers stay modest (a couple
of background induction clusters per event at 2.0 MeV), which is why the
most-energetic-partner rule still wins (§3).

The same measured on CC files (10 cats x 20 `cc_*` files, i.e. 8000 generated CC events,
`matched_clusters_ccprobe_*` products):

| variant | CC X mains | CC 3-plane | eff | U clusters/cat | V clusters/cat | U bg | V bg |
|---|---|---|---|---|---|---|---|
| e3p0 | 7509 | 6488 | 86.4% | 1391 | 1573 | 134 | 249 |
| ind 2.5 | 7509 | 6611 | **88.0%** | 1879 | 2267 | 477 | 792 |
| ind 2.0 | 7509 | 6696 | **89.2%** | 3068 | 3805 | 1492 | 2140 |

CC events gain much less than ES (+1.9% / +3.2% relative, against +6.6% / +10.8% for ES),
because a CC electron is energetic enough that its induction cluster was already above
3 MeV. Consequence for the deployed selection: the **CC contamination grows by about
+1.6 / +2.7 events per burst** (scaling the reference 84.6 CC events/burst by the
3-plane efficiency ratio), against +3.3 / +5.2 ES events, so the ES purity of the
deployed selection improves marginally, from 41.7% to ~42.6% (2.5) / ~43.0% (2.0).

### 6.2 Data volume, file counts and CPU

Measured on the 9 ES files of each cat (three planes):

| | ref 3.0 | ind 2.5 | ind 2.0 |
|---|---|---|---|
| cluster-image npz files | 27/cat (unchanged) | 27/cat | 27/cat |
| cluster-image volume | 18.5 MB/cat | 26.1 MB/cat (x1.41) | 44.8 MB/cat (x2.42) |
| chain CPU (`-mc -mm -gi`, 9 files) | 35.0 s/cat | 37.5 s/cat (+7%) | 41.1 s/cat (+17%) |

The **number of files is unchanged** (one npz per input file per plane), only their size
grows. Extrapolated to a full 110-file cat (CC multiplicity grows by x1.35 / x2.2, ES by
x1.41 / x2.42), the cluster-image product goes from ~360 MB/cat to roughly 470 MB (2.5)
or 760 MB (2.0); for a 1000-cat campaign that is ~360 GB -> ~470 / ~760 GB.

What does **not** change:
* **volume images** (X plane only, X accepted-cluster set identical) - no regeneration,
  no extra disk, and the CT input is bit-identical;
* **CT inference cost** (same number of X volumes);
* **ED inference cost** - `pipeline.py` runs the ED on main-track clusters only and the
  number of X main tracks is unchanged; only the fraction that ends up with three planes
  grows;
* **matching CPU** is included in the +7% / +17% above.

### 6.3 Closure test of the default-off property

The patched binary was re-run on 10 cats (623-632) with a JSON containing **no** per-view
keys (`_indcutref`). The resulting ES cluster images are **bit-identical** to the existing
`_matchfix` products: 270 npz compared (11269 clusters), **0 differing** in either
`images` or `metadata`. The products of that test were then deleted.

Additional closure test of the analysis chain: re-running the study's own ED v63 and the
pipeline's own fit machinery on the **reference** event set reproduces the published
`R4_v63_ownpdf_matchfix` per-cat numbers exactly (cat000623: scenario 2 n=241,
cos=0.9992, q68=2.86 vs 2.864 in the report; scenario 6 n=214, cos=0.9993, q68=2.839 vs
2.861; deployed selection n=148, identical to the report's scenario 3 count).

---

## 7. Recommendation

**Yes to a lower induction cut, and 2.5 MeV is the value to take.** Concretely:
set `"energy_cut": 3.0` and `"energy_cut_induction": 2.5` in the cat JSONs. The X plane
keeps its 3.0 MeV cut, as required.

Why 2.5 rather than 2.0:

| | ind 2.5 | ind 2.0 |
|---|---|---|
| ES events into the pipeline | +6.6% | +10.8% |
| ES events into the deployed selection | +3.3/burst (+5.4%) | +5.2/burst (+8.6%) |
| induction **background** clusters | x5.5 (U), x3.7 (V) | **x19 (U), x10.6 (V)** |
| cluster-image volume | x1.41 | x2.42 |
| chain CPU | +7% | +17% |
| wrong-partner rate | +0.3 pt | +0.5 pt |
| pointing | flat within the intervals | flat within the intervals |

Going from 2.5 to 2.0 buys +1.9 deployed events per burst (+1.3%) for a **3.5x larger
induction background population**, a 1.7x larger cluster-image product and a measurably
larger wrong-partner rate, with no demonstrated pointing gain. That is a poor trade for a
system that has to run online. 2.5 MeV captures the bulk of the recoverable events -
the recovered spectrum peaks at 3-4 MeV in **both** cases (the extra events at 2.0 are
mostly the same low-energy population, just with a weaker induction partner) - at a
background cost that is still manageable.

**Answering the question directly**: of the events the lower induction cut recovers,
**you lose about half at E_X > 5 MeV and a further ~57% of the survivors at the CT cut, so
roughly 22% of them reach the deployed selection** - 3.3 per burst at 2.5 MeV. The loss is
not because the recovered events are bad: it is because they are *low-energy*, and both
the 5 MeV cut and (above 8 MeV) the CT are energy cuts. In the perfect-CT scenarios,
essentially all of them (98.9% at E > 3 MeV) are kept, so the full +6.6% would become
available if the deployed threshold is ever lowered or the tagger improved. This is not
a symptom of a mismatched induction/collection threshold in the *matcher*: the
background-partner rate is flat at 1.2% (U) / 0.4% (V) at every threshold, and no event
is ever lost.

### What deployment would take

* **Code**: already done and default-off (§1.2), one file, closure-tested (§6.3).
  Nothing to change in `match_clusters`, `create_volumes`, the loader, or the pipeline.
* **Reprocessing campaign: yes, but only from clustering onward.** `tps_bg/` is
  untouched, so the chain runs `-mc -mm -gi` and *not* `-gv` (volume images are X-only
  and bit-identical, so they are reused as they are). Measured cost: 37.5 s CPU per cat
  for 9 files; for a 1000-cat campaign with all 110 files that is roughly 130 CPU-hours
  plus ~470 GB of new cluster images. That is the same order as the `_matchfix`
  revalidation campaign.
* **CT retrain: no.** The CT input (X volume images) is bit-identical. The measured pass
  rate of the recovered events matches the standard events bin by bin up to 8 MeV.
* **ED retrain: not required.** v63 was fine-tuned on a pool clustered at **2.0 MeV in
  all planes**, so a lower induction cut moves the burst products *towards* its training
  conditions, and the measurement confirms it: the recovered events' direction quality is
  comparable to standard events of the same energy, and better below 5 MeV (§4.2). A
  fine-tune would be *optional*, aimed at the 0.06-0.12 cos deficit in the 5-8 MeV bins.
* **Do rebuild the likelihood table.** `cosine_energy_pdf.npz` (and the mixture/ctsel
  variants) should be regenerated from the new products with
  `electron_direction/ana/build_cosine_energy_pdf.py`, because the recovered population
  is slightly worse at fixed energy than what the current table assumes. This is cheap
  and is the one thing that should not be skipped.
* **Before committing**, if the pointing gain matters rather than just the event count,
  produce the 2.5 MeV variant for a few hundred cats and rerun the deployed-scenario fit;
  50 cats cannot resolve it.

---

## 8. Files produced by this study

Code (additive, default-off):
* `src/app/make_clusters.cpp` - the per-view `energy_cut` override (the only source file
  touched; branch `snop-fixes-2026-09`, not committed).
* `python/ana_evilla/induction_threshold_scan.py` - replicates the pipeline's 3-plane
  main-track selection and the 330-generated-ES budget on any product set; writes the
  per-event list plus cluster-multiplicity and match-purity counters.
* `python/ana_evilla/induction_threshold_join_ref.py` - inverts the pipeline's seed-42
  shuffle and attaches the deployed R4 CT v80 score and ED v63 direction to every
  reference ES event.
* `python/ana_evilla/induction_threshold_recovered.py` - the recovered-event tables.
* `python/ana_evilla/induction_threshold_ccprobe.py` - the CC-side 3-plane rate.
* `python/ana_evilla/induction_threshold_report.py` - the pointing table with paired
  bootstrap intervals.
* `refactor-snop-pipeline/python/ana/induction_threshold_infer.py` - ED v63 + CT v80 on
  the recovered events (and the image-identity check on the common events).
* `refactor-snop-pipeline/python/ana/induction_threshold_fits.py` - the offline fits.
* `refactor-snop-pipeline/python/ana/induction_threshold_validate.py` - ED/CT closure
  against the R4 products.

Configuration:
* `json/mixture_dev/indcut/cat_0006NN_{indcut25,indcut20}.json` (50 + 50),
  `json/mixture_dev/ccprobe/cat_0006NN_ccprobe_{e3p0,indcut25,indcut20}.json` (30).
* `refactor-snop-pipeline/condor/mixture_dev/proc_indcut_cats623to672.{sub,args}`,
  `indcut_infer.{sub,args}`, `indcut_fits.{sub,args}`, `indcut_validate.sub`,
  `ccprobe.{sub,args}`, `proc_indcutref.{sub,args}`.

Products (new folders only, 3.9 GB / 4200 files under
`/eos/user/e/evilla/dune/sn-tps/mixture_dev_samples/cat0006NN/`):
* `cat0006NN_{matched_clusters,cluster_images}_tick3_ch2_min2_tot3_e3p0_indcut{25,20}`
* `cat0006NN_matched_clusters_ccprobe_{e3p0,indcut25,indcut20}` (cats 623-632)
* the `clusters_*` intermediates were deleted after matching, and the `_indcutref`
  closure products after the comparison.

Analysis outputs in
`/eos/project-e/ep-nu/evilla/sn-online-pointing/pipeline-dev/induction_threshold/`:
`scan_all.npz`, `ref_products.npz`, `infer_indcut{25,20}_<range>.npz`,
`fits_<range>.npz`, `validate_623_625.npz`.

## 9. Honest limits of this study

* **50 cats.** A paired bootstrap on 50 cats gives ~1-2 deg 68% intervals on theta68 in
  the deployed scenario, which is larger than the effect we are looking for. The event
  counts (§3, §4) are solid (thousands of events); the *pointing* numbers are indicative
  only. To resolve a sub-degree pointing change the variants would have to be produced
  for several hundred cats.
* **ES files only.** Clusters/matching/images were regenerated only for the 9 ES input
  files of each cat. The CC side was measured separately on 20 `cc_*` files of 10 cats
  (counts only, no images), so the extra CC contamination is an *estimate* scaled from
  the 3-plane efficiency ratio, and the extra CC events are **not** included in the
  pointing fits. They amount to +1.6 / +2.7 events on a base of 145, i.e. a 1-2%
  perturbation of the background, much smaller than the ES gain, so the deployed-scenario
  fits are marginally optimistic.
* **The likelihood tables were not rebuilt.** The v63 `cosine_energy_pdf*` tables were
  built from products clustered at e3p0. Since a recovered event has slightly worse
  direction quality than a standard event of the same energy (§4.2), the table is
  slightly optimistic about them. Rebuilding the table on the new products
  (`build_cosine_energy_pdf.py`) is cheap and would remove this.
* **Reco X energy** is the image-derived `metadata[:, 10]`, which is what the pipeline
  cuts on; it differs slightly from the `total_charge`-based quantity the clustering cut
  uses, which is why a handful of recovered events sit just below 3 MeV in §4.1.
* **Only one matching rule** was used (the current default, most energetic partner).
  A lower induction cut interacts with the partner rule; with the legacy first-in-time
  rule the gains would be smaller and the purity worse (cf.
  `three_plane_matching_efficiency_study.md` §9.3).
* **Not measured**: the effect on the trigger/online timing budget, the behaviour on real
  detector noise rather than simulated radiologicals, and whether a 2.5 MeV induction cut
  is compatible with the TP-level thresholds actually achievable online.
