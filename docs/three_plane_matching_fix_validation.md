# Three-plane matching fix (P3 + P4) — implementation and validation

Companion to `docs/three_plane_matching_efficiency_study.md`, which diagnosed the
problem. This document records **what was changed in the code** and **what the change
does on 50 development bursts**, measured before and after with truth.

* Repository: `refactor-online-utils`, branch `feature/uv-volume-production`.
* Sample: mixture development cats **623-672** (50 cats, 110 tpstream files each:
  100 `cc_*`, 10 `es_*`), user EOS
  `/eos/user/e/evilla/dune/sn-tps/mixture_dev_samples/cat0006NN/`.
* **No JSON parameter was changed.** `tick_limit 3, channel_limit 2,
  min_tps_to_cluster 2, tot_cut 3, energy_cut 3.0, time_tolerance_ticks 10` are
  identical in the production and in the revalidation configs; only the three output
  folder names differ.
* Only the matching step and everything downstream of it was re-run, from the kept
  `*_clusters_*` intermediates. The volume-image rendering code was used **unchanged**
  (radiological mask off, its default).

---

## 1. Summary

| | before | after | delta |
|---|---|---|---|
| 3-plane main-track match, ES electrons | **74.8%** | **83.8%** | **+9.0 pts** |
| ES electrons entering the pointing pipeline (3-plane, E > 3 MeV) | 11799 | 13219 | +12.0% relative |
| wrong-partner rate (match_id on a non-main cluster) | 11.2% | 2.2% | -9.0 pts |
| match_ids carried by X but present in no other plane | 697 (4.4%) | 0 | fixed |
| `match_type` correctness | all 3 | 3 / 2 / 1 as appropriate | fixed |
| partner from a different event | 0 | 0 | unchanged |
| partner with MARLEY fraction < 0.5 | 104 | 96 | slightly better |
| 3-plane main-track match, CC main tracks (§7) | 66.5% | 87.2% | +20.7 pts |

The study predicted **+11.8 points** (74.8% -> 86.6%). The measured gain is **+9.0
points** (74.8% -> 83.8%): the fix recovers **82%** of the wrong-partner population,
not 100%. See §5 for why, honestly.

The gain is exactly where the study said it would be — at high electron energy:
+42 points above 30 MeV, +26 points at 20-30 MeV, +13 points at 10-20 MeV, and
nothing below 5 MeV (those losses are `energy_cut` losses, untouched by this fix).

The change is provably confined to what it was meant to change: running the new binary
with `--first-in-time-partner` over a whole cat and comparing `match_id` cluster by
cluster against the production products gives **118 differences out of 22833 clusters,
all of them V-plane clusters going from `match_id = -1` to a real id** — i.e. exactly
the 118 V+X partial matches that the P4a bug used to drop, and nothing else (§4.1).

---

## 2. Code changes

Three changes, all in the matching step. `git diff` against `a7c5b71`.

### P3 — ambiguity resolution: keep the best candidate, not the first in time

`src/app/match_clusters.cpp`, U pass (was `:301-303`, now `:312-322`) and V pass
(was `:348-350`, now `:366-374`):

```cpp
// Found a matching U candidate - keep the BEST one.
// Default rule: highest total_energy (truth-agnostic proxy for the main track);
// ties and the legacy rule keep the first candidate in the scan order.
auto existing_u = x_to_u_match.find(x_id);
if (existing_u == x_to_u_match.end()) {
    x_to_u_match[x_id] = int(j);
} else if (!first_in_time_partner) {
    if (clusters_u[j].get_total_energy() > clusters_u[existing_u->second].get_total_energy()) {
        existing_u->second = int(j);
    }
}
```

The decision uses **only `total_energy`**. `is_main_cluster` and every other truth
flag are never consulted, so the rule is usable on data. Ties keep the earlier
candidate, so the result is deterministic. Within one view `total_energy` is a fixed
multiple of `total_charge`, so this is equivalent to "highest integrated ADC".

The legacy rule is still reachable:

```
build/src/app/match_clusters -j <cat.json> --first-in-time-partner
```

The flag is a CLI trigger (`match_clusters.cpp:76-78`, read at `:123`, logged at
`:135-137`) — no JSON key, no recompilation. Verified to reproduce the production
`match_id` assignment exactly (§4.1).

### P4a — V+X partial matches now reach the V cluster

`src/app/match_clusters.cpp:378-390` and `:531-552` (was `:521-533`).

The old code decided whether a 2-plane match was `U+X` or `V+X` by asking the stored
clusters for their view. **That can never work here**: `read_clusters_from_tree()`
rebuilds `TriggerPrimitive`s and calls the `SetView(int)` *channel* overload with
`0/1/2` (`Clustering.cpp`, "Set view from parameter"), and `SetView(int)` interprets
its argument as a channel number — `0`, `1` and `2` are all below
`APA::induction_channels`, so **every TP read back from a cluster tree reports
`GetView() == "U"`**. Consequently `c1_is_u` was always true, every 2-plane match was
routed into the U branch, and a V+X match wrote the **V** cluster id into the **U**
map, where nothing could ever find it (cluster ids are unique across views). The X
cluster ended up with a `match_id` present in neither induction plane.

The fix records the topology of each match **when the match is created**, so no view
sniffing is needed at all:

```cpp
struct MatchTopology {
    int type;   // 3 = X+U+V, 2 = U+X, 1 = V+X
    int x_id;
    int u_id;   // -1 if absent
    int v_id;   // -1 if absent
};
std::vector<MatchTopology> match_topologies;
```

populated alongside `matches` in the third pass, and consumed by the match_id
assignment loop, which becomes a straightforward walk over `match_topologies`.

### P4b — write the real `match_type`

`src/clusters/Clustering.h:31-35` and `src/clusters/Clustering.cpp:703-705`,
`:839-851`. `write_clusters_with_match_id()` gained an optional trailing parameter
`std::map<int,int>* match_id_to_type`; the hard-coded

```cpp
match_type = 3;  // Currently only 3-plane matches
```

became a lookup in that map, falling back to `3` when the caller does not supply one
(so the other two call sites, and any external user, keep the old behaviour).
`match_clusters.cpp:571-573` now passes `&match_type_map` for all three views.

Downstream code can now select genuine 3-plane matches with `match_type == 3` instead
of intersecting the three `match_id` sets.

### Documentation

`docs/MATCHING_CRITERIA_AND_HANDLING.md` was rewritten to describe the code as it is:
the real ±320 TDC tick window (10 TPC ticks from the JSON), the actual gates
(time overlap / same event / a same-APA test that is a no-op), the fact that
`are_compatibles()` is a same-detector stub and that `spatial_tolerance_cm` is unused,
the new partner rule and its flag, the `match_type` values, and a "known remaining
issues" section.

### Analysis tooling (new, `python/ana_evilla/`)

| file | purpose |
|---|---|
| `matchfix_validation.py` | ROOT-level before/after metrics straight from `*_matched.root` (3-plane fraction, energy bins, wrong-partner breakdown, orphan match_ids, match_type, pointing yield) |
| `matchfix_tables.py` | turns two of its json outputs into the tables below |
| `three_plane_matching_efficiency.py` | the study's npz-level classifier; gained `--images-suffix` / `--conditions` so it can be pointed at the `_matchfix` cluster images |

---

## 3. How the products were regenerated

Per-cat configs `json/mixture_dev/matchfix/cat_0006NN_matchfix.json` are byte-identical
to `json/mixture_dev/cat_0006NN.json` except for three added keys:

```json
"matched_clusters_folder": ".../cat0006NN_matched_clusters_tick3_ch2_min2_tot3_e3p0_matchfix",
"cluster_images_folder":   ".../cat0006NN_cluster_images_tick3_ch2_min2_tot3_e3p0_matchfix",
"volume_images_folder":    ".../cat0006NN_volume_images_tick3_ch2_min2_tot3_e3p0_matchfix"
```

All three keys were already honoured by the existing code
(`InputOutput.cpp:getOutputFolder`, `generate_cluster_arrays.py`,
`create_volumes.py`), so **no driver change was needed**: `sequence.sh` already
selects steps with `-mm -gi -gv`.

```
condor/mixture_dev/proc_matchfix_cats623to672.{sub,args}        # in refactor-snop-pipeline
  -> 50 jobs (one per cat), cluster 15931951, ~40-60 min each, all exit 0
condor/mixture_dev/proc_matchfix_fill_cats623to672.{sub,args}
  -> 50 short jobs, cluster 15931954, all exit 0 (see below)
  -> logs in refactor-snop-pipeline/condor/logs/mixture_dev/matchfix/
```

A second, small pass was needed because of a pre-existing bug **outside** the matcher:
`python/lib/utils.py:126` does `all_files = all_files[:max_files]`, and the cat JSONs
carry `"max_files": -1`, so when `--max-files` is not given on the command line the
Python steps (`-gi`, `-gv`) silently **drop the last file of every cat** — here
`es_000009`, an ES file. The production waves always passed `--max-files 20`
explicitly, so this never showed up before. The fill-in pass simply re-runs `-gi -gv`
with `--max-files 200` and no `-f`, so only the missing file per cat is produced. This
file is *not* a parameter and the bug was left in place (it is not in the matcher);
it is flagged here so the next production wave keeps passing `--max-files`
explicitly.

Nothing existing was overwritten or deleted; the `_matchfix` folders are new and sit
next to the originals, with the same internal layout (`{U,V,X}/` for images and
volumes, flat `*_matched.root` for the matched clusters), so the pointing pipeline can
consume them by changing only the folder name.

Final coverage, verified after both passes: **5497 matched-cluster files, 5497
cluster-image triplets, 5497 volume-image triplets** — i.e. every available input.
The nominal count would be 5500; the three missing ones (cats 628, 636, 649, one file
each) are absent from the **production** `*_clusters_*` intermediates as well, so
before and after are compared on exactly the same input set. All 100 condor jobs
returned 0.

---

## 4. Results — 50 cats, ES electrons (`es_*` files)

### Overall — ES (es_* files), 50 cats

| quantity | before | after | delta |
|---|---|---|---|
| X main-track clusters | 15783 | 15783 | +0 |
| `match_id != -1` | 14709 (93.2%) | 14714 (93.2%) | +5 (+0.0 pts) |
| **3-plane main-track match** | 11800 (74.8%) | 13220 (83.8%) | +1420 (+9.0 pts) |
| wrong partner (match_id on a non-main cluster) | 1768 (11.2%) | 355 (2.2%) | -1413 (-9.0 pts) |
| ... in U | 1103 (7.0%) | 211 (1.3%) | -892 (-5.7 pts) |
| ... in V | 1139 (7.2%) | 253 (1.6%) | -886 (-5.6 pts) |
| orphan match_id (in X only, no U/V) | 697 (4.4%) | 0 (0.0%) | -697 (-4.4 pts) |
| 3-plane and E_true > 3 MeV (pointing input) | 11799 (74.8%) | 13219 (83.8%) | +1420 (+9.0 pts) |

### 3-plane match fraction vs true electron energy

| E_true [MeV] | N | before | after | delta |
|---|---|---|---|---|
| 3-5 | 2389 | 1527 (63.9%) | 1526 (63.9%) | -0.0 pts |
| 5-10 | 5484 | 4491 (81.9%) | 4597 (83.8%) | +1.9 pts |
| 10-20 | 6110 | 4645 (76.0%) | 5442 (89.1%) | +13.0 pts |
| 20-30 | 1514 | 989 (65.3%) | 1387 (91.6%) | +26.3 pts |
| 30+ | 285 | 147 (51.6%) | 267 (93.7%) | +42.1 pts |
| **all** | 15783 | 11800 (74.8%) | 13220 (83.8%) | +9.0 pts |

### Partner purity (the partner actually written into the match)

| quantity | before | after |
|---|---|---|
| partner from a different event | 0 | 0 |
| partner with MARLEY fraction < 0.5 | 104 | 96 |
| partner on a different APA | 1 | 1 |

(Denominator: ~29k chosen induction partners. The same table for the much larger CC
sample is in §7.)

Breakdown of the wrong-partner (non-main) cases:

| quantity | before | after |
|---|---|---|
| different event | 0 | 0 |
| non-MARLEY cluster | 20 | 8 |
| MARLEY fragment of the same event | 2222 | 456 |
| different APA | 1 | 1 |

### match_type distribution (clusters with `match_id != -1`)

| plane | before | after |
|---|---|---|
| X | {'3': 14709} | {'1': 697, '2': 485, '3': 13532} |
| U | {'3': 14012} | {'2': 485, '3': 13532} |
| V | {'3': 13527} | {'1': 697, '3': 13532} |

### Per cat

| cat | X main | 3-plane before | 3-plane after | pointing before | pointing after |
|---|---|---|---|---|---|
| 623 | 317 | 260 (82.0%) | 288 (90.9%) | 259 | 287 |
| 624 | 308 | 240 (77.9%) | 274 (89.0%) | 240 | 274 |
| 625 | 324 | 228 (70.4%) | 248 (76.5%) | 228 | 248 |
| 626 | 307 | 200 (65.1%) | 227 (73.9%) | 200 | 227 |
| 627 | 307 | 221 (72.0%) | 242 (78.8%) | 221 | 242 |
| 628 | 318 | 253 (79.6%) | 288 (90.6%) | 253 | 288 |
| 629 | 323 | 216 (66.9%) | 243 (75.2%) | 216 | 243 |
| 630 | 318 | 247 (77.7%) | 279 (87.7%) | 247 | 279 |
| 631 | 309 | 200 (64.7%) | 233 (75.4%) | 200 | 233 |
| 632 | 309 | 248 (80.3%) | 281 (90.9%) | 248 | 281 |
| 633 | 322 | 259 (80.4%) | 291 (90.4%) | 259 | 291 |
| 634 | 331 | 253 (76.4%) | 287 (86.7%) | 253 | 287 |
| 635 | 316 | 235 (74.4%) | 264 (83.5%) | 235 | 264 |
| 636 | 320 | 262 (81.9%) | 298 (93.1%) | 262 | 298 |
| 637 | 317 | 237 (74.8%) | 264 (83.3%) | 237 | 264 |
| 638 | 314 | 248 (79.0%) | 290 (92.4%) | 248 | 290 |
| 639 | 317 | 255 (80.4%) | 284 (89.6%) | 255 | 284 |
| 640 | 301 | 225 (74.8%) | 252 (83.7%) | 225 | 252 |
| 641 | 316 | 212 (67.1%) | 237 (75.0%) | 212 | 237 |
| 642 | 311 | 257 (82.6%) | 292 (93.9%) | 257 | 292 |
| 643 | 324 | 258 (79.6%) | 299 (92.3%) | 258 | 299 |
| 644 | 312 | 227 (72.8%) | 256 (82.1%) | 227 | 256 |
| 645 | 325 | 211 (64.9%) | 233 (71.7%) | 211 | 233 |
| 646 | 298 | 222 (74.5%) | 238 (79.9%) | 222 | 238 |
| 647 | 330 | 258 (78.2%) | 299 (90.6%) | 258 | 299 |
| 648 | 317 | 253 (79.8%) | 284 (89.6%) | 253 | 284 |
| 649 | 314 | 255 (81.2%) | 284 (90.4%) | 255 | 284 |
| 650 | 308 | 174 (56.5%) | 198 (64.3%) | 174 | 198 |
| 651 | 317 | 190 (59.9%) | 217 (68.5%) | 190 | 217 |
| 652 | 319 | 227 (71.2%) | 242 (75.9%) | 227 | 242 |
| 653 | 323 | 233 (72.1%) | 247 (76.5%) | 233 | 247 |
| 654 | 311 | 235 (75.6%) | 273 (87.8%) | 235 | 273 |
| 655 | 312 | 256 (82.1%) | 282 (90.4%) | 256 | 282 |
| 656 | 305 | 239 (78.4%) | 278 (91.1%) | 239 | 278 |
| 657 | 323 | 267 (82.7%) | 302 (93.5%) | 267 | 302 |
| 658 | 310 | 244 (78.7%) | 272 (87.7%) | 244 | 272 |
| 659 | 310 | 241 (77.7%) | 268 (86.5%) | 241 | 268 |
| 660 | 317 | 213 (67.2%) | 238 (75.1%) | 213 | 238 |
| 661 | 325 | 258 (79.4%) | 294 (90.5%) | 258 | 294 |
| 662 | 309 | 208 (67.3%) | 232 (75.1%) | 208 | 232 |
| 663 | 309 | 240 (77.7%) | 261 (84.5%) | 240 | 261 |
| 664 | 311 | 234 (75.2%) | 252 (81.0%) | 234 | 252 |
| 665 | 310 | 240 (77.4%) | 274 (88.4%) | 240 | 274 |
| 666 | 315 | 227 (72.1%) | 253 (80.3%) | 227 | 253 |
| 667 | 329 | 264 (80.2%) | 287 (87.2%) | 264 | 287 |
| 668 | 323 | 263 (81.4%) | 296 (91.6%) | 263 | 296 |
| 669 | 327 | 218 (66.7%) | 243 (74.3%) | 218 | 243 |
| 670 | 320 | 226 (70.6%) | 239 (74.7%) | 226 | 239 |
| 671 | 298 | 219 (73.5%) | 240 (80.5%) | 219 | 240 |
| 672 | 327 | 244 (74.6%) | 277 (84.7%) | 244 | 277 |
| **total** | 15783 | 11800 (74.8%) | 13220 (83.8%) | 11799 | 13219 |

### 4.1 Regression check: the legacy flag reproduces production

On three ES files of cat 623, matched with the new binary and
`--first-in-time-partner`, against the production products:

| | production | new binary, `--first-in-time-partner` | new binary, default |
|---|---|---|---|
| X main tracks | 96 | 96 | 96 |
| `match_id != -1` | 95 | 95 | 95 |
| 3-plane main-track | 83 | **83** | **90** |
| wrong partner | 8 | **8** | 1 |
| orphan match_id | 3 | 0 (P4a) | 0 |
| `match_type` in X | all 3 | 3:91, 2:1, 1:3 | 3:91, 2:1, 1:3 |

The legacy flag reproduces the production `match_id` assignment exactly; the two
differences (orphans, match_type) are the P4 bug fixes, which are always on.

**Full-cat proof.** The whole of cat 623 (110 files) was rematched with
`--first-in-time-partner` into a scratch folder and every cluster's `match_id`
compared with the production file, in all three views:

| comparison | differing clusters (out of 22833) |
|---|---|
| production vs `--first-in-time-partner` | **118** |
| production vs new default rule | 2551 |

and all 118 are of one single kind: `V` clusters with `match_id = -1` in production and
a real `match_id` in the new output. 118 is exactly the number of V+X partial matches
the app reports for that cat. So with the legacy flag the *only* change relative to
production is the P4a fix; the partner rule itself is bit-identical.

```
./build/src/app/match_clusters -j json/mixture_dev/cat_000623.json \
    --outFolder <scratch> -f --first-in-time-partner --max-files 200
```

### 4.2 Loss classification, before and after

Same categories as the study (worst failure of the two induction planes):

| category | before | after |
|---|---|---|
| MATCHED_3PLANE | 11800 (74.8%) | **13220 (83.8%)** |
| `partner_bg` — match_id sits on a non-main cluster | 1727 (10.9%) | **312 (2.0%)** |
| `D` — X main track itself unmatched | 1074 (6.8%) | 1069 (6.8%) |
| `no_cluster` — plane has no cluster for the event | 931 (5.9%) | 931 (5.9%) |
| `main_unmatched` — main exists in the plane, `match_id = -1` | 138 (0.9%) | 138 (0.9%) |
| `no_main` — clusters exist, none is main | 113 (0.7%) | 113 (0.7%) |

Every category except `partner_bg` is unchanged, which is exactly the intended
behaviour: P3 only changes *which* partner is chosen, never *whether* one exists.
(`D` moves by 5 out of 15783 — see the caveat in §6.)

---

## 5. Did the +11.8 point prediction hold? Not quite: +9.0

The study estimated the gain at the npz level as "X is already matched, both planes do
have a main-track cluster, but the wrong partner was picked" = 11.8% of all X main
tracks, and quoted a ceiling of 87.0% for a perfect partner choice.

Measured here: the wrong-partner population before the fix is 10.9% (`partner_bg`);
after the fix 2.0% survives. So

* recovered: 8.9 points out of the 10.9 available — **82% of the population**;
* the study's number is an **upper bound** that assumes the rule always finds the
  main-track cluster.

Why the residual 2.0%: `is_main_cluster` is *the most energetic MARLEY cluster of the
event in that view*, chosen over **all** clusters of the event, whereas the matcher
ranks only the candidates that overlap this particular X cluster in time. The two
disagree when
1. a radiological cluster in the window is more energetic than the electron's main
   induction cluster, or
2. the main induction cluster is not in the time window at all (it is then counted in
   `main_unmatched`, 0.9%).

A truth-based "prefer `is_main_cluster`" rule would close part of this, but it is not
usable on data, and the study explicitly asked for the truth-agnostic proxy. The
remaining 2.0% is a genuine limit of an energy-only proxy on this sample.

---

## 6. Purity cost

Essentially none: every purity metric except one moves in the *right* direction, and
the one exception is 4 partners in 100000.

* **Different-event partners: 0 before, 0 after.** The same-event gate is enforced in
  the scan, so this cannot happen by construction.
* **Non-MARLEY partners: 104 before, 96 after** (out of ~29k chosen induction
  partners) — a slight *improvement*, because the electron's own induction clusters
  are usually the most energetic ones in the window.
* **Different-APA partners: 1 before, 1 after** for ES; **0 before, 14 after** for the
  much larger CC sample (out of ~365k chosen partners, i.e. 4 in 100000). This is the
  one place where the new rule is very slightly worse: the same-APA gate inside the
  scan is a no-op (see `MATCHING_CRITERIA_AND_HANDLING.md` §6), so a more energetic
  cluster on another APA can now win a race it previously lost on time order. Fixing
  that gate (use `GetDetector()`) would remove even these; it was left out of scope
  because it changes matching behaviour beyond the requested P3/P4.
* Among the *wrong* (non-main) partners that survive, the non-MARLEY ones drop from 20
  to 8, and the rest are MARLEY fragments of the same event — i.e. the residual
  mistakes are "wrong fragment of the right electron", not background contamination.

One second-order effect: `match_id != -1` moved from 14709 to 14714 (+5 out of 15783,
+0.03%). A complete 3-plane match is only accepted if `are_compatibles(U,V,X)` passes;
that function compares the detector of the three clusters, so changing which U/V
cluster is proposed can flip a handful of cases either way. Five out of 15783 is
noise, but it is why the `D` category moves by 5 in §4.2.

---

## 7. Charged-current main tracks (`cc_*` files) — side effect

The mixture cats also contain 100 CC files each, used for channel tagging. They are
affected by the same fix, and much more strongly, because CC tracks are longer and
fragment more on the induction wires: **66.5% -> 87.2%, +20.7 points**, with the
wrong-partner rate falling from 27.1% to 6.7% and the non-MARLEY partner count
improving from 1397 to 1227.

### Overall — CC (cc_* files), 50 cats

| quantity | before | after | delta |
|---|---|---|---|
| X main-track clusters | 187336 | 187336 | +0 |
| `match_id != -1` | 182150 (97.2%) | 182219 (97.3%) | +69 (+0.0 pts) |
| **3-plane main-track match** | 124596 (66.5%) | 163301 (87.2%) | +38705 (+20.7 pts) |
| wrong partner (match_id on a non-main cluster) | 50708 (27.1%) | 12535 (6.7%) | -38173 (-20.4 pts) |
| ... in U | 32113 (17.1%) | 7486 (4.0%) | -24627 (-13.1 pts) |
| ... in V | 33914 (18.1%) | 8721 (4.7%) | -25193 (-13.4 pts) |
| orphan match_id (in X only, no U/V) | 4003 (2.1%) | 0 (0.0%) | -4003 (-2.1 pts) |
| 3-plane and E_true > 3 MeV (pointing input) | 123744 (66.1%) | 162364 (86.7%) | +38620 (+20.6 pts) |

### 3-plane match fraction vs true electron energy

| E_true [MeV] | N | before | after | delta |
|---|---|---|---|---|
| 3-5 | 5617 | 3733 (66.5%) | 3942 (70.2%) | +3.7 pts |
| 5-10 | 28924 | 21364 (73.9%) | 23749 (82.1%) | +8.2 pts |
| 10-20 | 89286 | 61173 (68.5%) | 77469 (86.8%) | +18.3 pts |
| 20-30 | 46536 | 29076 (62.5%) | 42498 (91.3%) | +28.8 pts |
| 30+ | 15625 | 8398 (53.7%) | 14706 (94.1%) | +40.4 pts |
| **all** | 187336 | 124596 (66.5%) | 163301 (87.2%) | +20.7 pts |

### Partner purity (the partner actually written into the match)

| quantity | before | after |
|---|---|---|
| partner from a different event | 0 | 0 |
| partner with MARLEY fraction < 0.5 | 1397 | 1227 |
| partner on a different APA | 0 | 14 |

Breakdown of the wrong-partner (non-main) cases:

| quantity | before | after |
|---|---|---|
| different event | 0 | 0 |
| non-MARLEY cluster | 415 | 186 |
| MARLEY fragment of the same event | 65612 | 16021 |
| different APA | 0 | 13 |

### match_type distribution (clusters with `match_id != -1`)

| plane | before | after |
|---|---|---|
| X | {'3': 182150} | {'1': 4003, '2': 3689, '3': 174527} |
| U | {'3': 178147} | {'2': 3689, '3': 174527} |
| V | {'3': 174458} | {'1': 4003, '3': 174527} |

---

## 8. Cross-check at the npz (cluster-image) level

The pointing pipeline reads the cluster images, not the ROOT files, so the same
measurement was repeated with the study's own npz classifier
(`three_plane_matching_efficiency.py`, `--images-suffix _matchfix`):

The two tools are independent (one reads the ROOT trees, the other the npz metadata
written by `generate_cluster_arrays.py`) and they agree to the unit:

| category (study's definition) | before | after |
|---|---|---|
| MATCHED_3PLANE | 11800 (74.8%) | **13220 (83.8%)** |
| C3 — partner is a background/fragment cluster | 1727 (10.9%) | **312 (2.0%)** |
| D — X main track unmatched | 1074 (6.8%) | 1069 (6.8%) |
| A — no cluster in U and/or V | 931 (5.9%) | 931 (5.9%) |
| C1 — main exists in the plane but unmatched | 138 (0.9%) | 138 (0.9%) |
| B — clusters exist but none is main | 113 (0.7%) | 113 (0.7%) |
| C2 — main exists with a different id | 0 | 0 |

3-plane efficiency in the study's finer energy bins:

| E_true [MeV] | N | before | after |
|---|---|---|---|
| 0-5 | 2390 | 63.9% | 63.9% |
| 5-10 | 5484 | 81.9% | 83.8% |
| 10-15 | 4035 | 78.2% | 88.3% |
| 15-20 | 2075 | 71.9% | 90.5% |
| 20-25 | 1083 | 67.0% | 90.9% |
| 25-30 | 431 | 61.0% | 93.5% |
| 30-40 | 246 | 52.4% | 93.5% |
| 40-60 | 37 | 45.9% | 94.6% |
| >60 | 2 | 50.0% | 100.0% |

The script's own recovery estimator, run on the two sets:

| | before | after |
|---|---|---|
| `frac_if_partner_rule_fixed` (what a perfect partner choice would give) | **86.56%** | 86.59% |
| `frac_ceiling_main_available` (a main-track cluster exists in both planes) | 86.97% | 86.97% |
| wrong-partner tracks | 1768 | 355 |

The first row of the "before" column, **86.56%**, *is* the study's +11.8 point
prediction, reproduced on exactly this sample. The achieved 83.8% is 2.8 points below
it, and the ceiling (86.97%) has not moved, so 446 tracks remain where a better
partner choice would still help.

---

## 8b. Reproducing the numbers

```bash
source /cvmfs/sft.cern.ch/lcg/views/LCG_104/x86_64-el9-gcc11-opt/setup.sh
B=/eos/user/e/evilla/dune/sn-tps/mixture_dev_samples

# ROOT-level, before and after (swap --prefix cc_ for the CC tables)
python3 python/ana_evilla/matchfix_validation.py --cats 623-672 --base $B \
        --prefix es_ --suffix ""          --out before_es.json
python3 python/ana_evilla/matchfix_validation.py --cats 623-672 --base $B \
        --prefix es_ --suffix "_matchfix" --out after_es.json
python3 python/ana_evilla/matchfix_tables.py --before before_es.json \
        --after after_es.json --per-cat

# npz-level cross-check with the study's own classifier
python3 python/ana_evilla/three_plane_matching_efficiency.py --cats 623-672 \
        --base $B --prefix es_ --out before_npz.json
python3 python/ana_evilla/three_plane_matching_efficiency.py --cats 623-672 \
        --base $B --prefix es_ --images-suffix _matchfix --out after_npz.json
```

---

## 9. Product folders

For each cat `cat0006NN`, `NN = 23..72`, under
`/eos/user/e/evilla/dune/sn-tps/mixture_dev_samples/cat0006NN/`:

```
cat0006NN_matched_clusters_tick3_ch2_min2_tot3_e3p0_matchfix/   *_matched.root
cat0006NN_cluster_images_tick3_ch2_min2_tot3_e3p0_matchfix/     {U,V,X}/*_matched_plane?.npz
cat0006NN_volume_images_tick3_ch2_min2_tot3_e3p0_matchfix/      {U,V,X}/*_bg_plane?.npz
```

The originals (`..._tick3_ch2_min2_tot3_e3p0` without the suffix) are untouched.

---

## 10. Caveats

1. **The +11.8 prediction did not fully materialise: +9.0 measured.** See §5. The
   study's figure was an upper bound.
2. **Below 5 MeV nothing improves** (63.9% before and after). Those losses are
   `energy_cut = 3.0` losses (P1 of the study) and need a parameter decision, which was
   explicitly out of scope here.
3. **Three pre-existing bugs were found and deliberately *not* fixed**, because fixing
   them would change existing products beyond the matching decision:
   * X-plane `total_energy` in `*_matched.root` (and metadata column 10 of the
     cluster images) is a factor 4 too large, because the clobbered TP view makes the
     `Cluster` constructor use the induction ADC->MeV factor for X clusters.
     `total_charge` is correct, and `*_clusters_*/*.root` from `make_clusters` is
     correct.
   * The same-APA gate inside the U/V scan compares
     `GetDetectorChannel() / APA::total_channels`, which is always 0 (the real APA is
     `GetDetector()`). This is the origin of the 14 cross-APA CC partners in §6.
   * `python/lib/utils.py:126` drops the last file of a cat when `max_files = -1`
     (`all_files[:-1]`). This bit the `-gi`/`-gv` steps here and needed the fill-in
     pass of §3; the production waves hide it by always passing `--max-files`
     explicitly.
   The first two are documented in `MATCHING_CRITERIA_AND_HANDLING.md` §6. All three
   are unchanged by this work, so the before/after comparison is unaffected.
4. **`is_main_cluster` is truth-derived** and is still what defines "3-plane matched"
   in this measurement, as in the study and in `sample_loader.py`. The *matching* is
   truth-agnostic; the *metric* is not.
5. The downstream effect of the fix (pointing resolution, channel-tagging performance)
   has **not** been measured. What is shown here is that 12.0% more ES electrons and
   31.2% more CC main tracks reach the pipeline, with unchanged partner purity.
   The channel-tagging and direction networks were trained on products made with the
   old rule; whether they need retraining on the new ones is an open question.
6. The `match_type` branch now carries real values (1/2/3). Any downstream code that
   assumed "matched implies `match_type == 3`" will now see 1 and 2 as well. The only
   reader in either repository is `python/app/create_volumes.py:504`, which copies it
   into a per-cluster dict and never branches on it, and
   `refactor-snop-pipeline/python/lib/sample_loader.py` does not read it at all — so
   nothing breaks today, but it is a schema-visible change.
7. Only cats 623-672 were reprocessed. The production samples under
   `/eos/project-e/ep-nu/...` were not touched.
