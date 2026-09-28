# Cluster Matching Criteria and Multiple Match Handling

This document describes **what `src/app/match_clusters.cpp` actually does**, as of the
P3/P4 fix (see `docs/three_plane_matching_fix_validation.md` and
`docs/three_plane_matching_efficiency_study.md`).

Earlier revisions of this file described a ±5000 tick window and 5 cm geometric cuts.
Neither of those is in the code; they have been removed here.

---

## 1. Inputs and scope

`match_clusters` reads the per-view cluster trees written by `make_clusters`:

* `clusters/clusters_tree_{U,V,X}` — the clusters that passed `energy_cut`;
* `discarded/clusters_tree_{U,V,X}` — the clusters below `energy_cut`.

Only the **accepted** clusters take part in matching. The discarded ones are copied to
the output with `match_id = -1`, `match_type = -1`.

**Only X-plane clusters flagged `is_main_cluster` seed a match**
(`match_clusters.cpp`, first/second/third pass). A background X cluster can therefore
never carry a `match_id`. `is_main_cluster` is set by `make_clusters` as *the most
energetic MARLEY cluster of that event in that view* (`make_clusters.cpp:295-315`), so
it is a truth-derived flag and it is assigned **independently in each view**.

Before matching, the clusters of each view are stable-sorted by
`(earliest TDC time, event, cluster_id)`; the scan uses a binary search into that order.

## 2. The gates

For every X main-track cluster, a candidate induction cluster must satisfy, in this
order:

| gate | code | meaning |
|---|---|---|
| time overlap | `timesOverlap(range_ind, range_x, time_tolerance_ticks_tdc)` | the two `[first TP start, last TP start + ToT]` ranges must overlap within the tolerance |
| same event | `ind.GetEvent() == x.GetEvent()` | |
| same APA | `GetDetectorChannel()/APA::total_channels` compared | **currently a no-op**, see §6 |

`time_tolerance_ticks` comes from the cat JSON and is expressed in **TPC ticks**; it is
converted to TDC ticks with `toTDCticks()` (× 32). Production value: `10` TPC ticks =
**320 TDC ticks**. This value is close to optimal: widening it *reduces* the 3-plane
efficiency under the legacy partner rule, and narrowing it is much worse
(`three_plane_matching_efficiency_study.md` §9.1).

`are_compatibles(U, V, X, spatial_tolerance_cm)` is called only for complete (3-plane)
matches. It is **not** the 5 cm geometric test the old version of this document
described: `src/clusters/MatchClusters.cpp` reduced it to a same-detector check, with
the wire-crossing geometry disabled. `spatial_tolerance_cm` is read from the JSON but
is not used. `match_with_true_pos()` (which does implement the 5 cm y/x cuts) is never
called from the app.

## 3. Ambiguity resolution — which candidate becomes the partner

Several induction clusters of the same event can overlap a long X cluster in time (the
electron fragments on the induction wires). The rule that picks one of them is:

* **default (since the P3 fix): the candidate with the highest `total_energy`.**
  This is truth-agnostic — it never looks at `is_main_cluster` or any other truth
  flag — and on the development samples it selects the same partner as an explicit
  "prefer the main track" rule.
* **legacy: the first candidate in the scan order**, i.e. the one that *starts
  earliest*. Still available with the CLI flag `--first-in-time-partner`, which
  reproduces the pre-fix products bit-for-bit as far as `match_id` is concerned.

Ties in energy keep the earlier candidate, so the rule is deterministic.

The legacy rule cost ~11 points of 3-plane efficiency, concentrated at high electron
energy (the lowest-energy fragment often starts first). See the validation document
for the measured before/after.

U and V are scanned in two independent passes; there is no U↔V consistency requirement
beyond `are_compatibles`.

## 4. Match topologies and `match_type`

The third pass turns the per-plane choices into matches:

| topology | `match_type` | condition |
|---|---|---|
| X + U + V | `3` | a U and a V partner were found and `are_compatibles` accepted them |
| U + X | `2` | only a U partner was found |
| V + X | `1` | only a V partner was found |
| unmatched | `-1` | no partner in either induction plane |

Partial (2-plane) matches **do** receive a `match_id`, so `match_id != -1` is *not* the
same thing as "3-plane matched". Downstream code that needs a genuine 3-plane match
should require `match_type == 3` (or intersect the `match_id` sets of the three planes,
which is what `refactor-snop-pipeline/python/lib/sample_loader.py` does).

Every match is recorded with its topology **at creation time** (`MatchTopology` in
`match_clusters.cpp`). This matters: `read_clusters_from_tree()` rebuilds
`TriggerPrimitive` objects and calls the `SetView(int)` *channel* overload with `0/1/2`,
so every TP read back from a cluster tree reports `GetView() == "U"`. Any attempt to
recover the plane of a stored cluster from `GetView()` inside `match_clusters` is
therefore wrong — this is what used to route V+X matches into the U map (§6).

## 5. Multiple match handling

Each X main-track cluster produces at most **one** match, so `match_id` values are
unique per X cluster by construction. On the induction side, the first match that
claims a given U (or V) cluster keeps it:

```cpp
if (u_cluster_to_match.find(topo.u_id) == u_cluster_to_match.end())
    u_cluster_to_match[topo.u_id] = match_id;
```

so a single induction cluster is never shared between two match ids. Two X main tracks
of the same event competing for the same induction cluster are resolved by X scan order;
this is rare and has not been measured to matter.

## 6. Known remaining issues (not fixed here)

These were found while implementing the P3/P4 fix. They are **not** addressed, because
fixing them would change existing products beyond the matching decision:

1. **The same-APA gate inside the U/V scan is a no-op.** It compares
   `GetDetectorChannel() / APA::total_channels`, but `GetDetectorChannel()` is already
   the channel *within* an APA (`detector_channel_ = channel_ % APA::total_channels`),
   so the expression is always `0`. The real APA index is `GetDetector()`, which
   `are_compatibles()` does compare — but only for 3-plane matches, so 2-plane matches
   have no APA check at all. In practice the time + event gates leave almost nothing:
   0 different-event and 1 different-APA partner in 15783 X main tracks of the
   50 development cats.
2. **X-plane `total_energy` is wrong in `*_matched.root`.** Because
   `read_clusters_from_tree()` clobbers the TP view to `"U"` (§4), the `Cluster`
   constructor converts ADC to MeV with the *induction* factor (900) instead of the
   collection factor (3600) for X clusters. X energies in the matched products, and
   hence in metadata column 10 of the cluster images, are a factor 4 too large.
   `total_charge` is unaffected, and `clusters/*.root` (from `make_clusters`) is
   correct. Fixing this would shift every X cluster energy in every existing product
   and would require retraining the downstream networks, so it is reported, not fixed.
3. **`energy_cut` removes the induction partner of low-energy electrons.** ~23% of X
   main tracks have their U/V main-track cluster in `discarded/`, with a median energy
   just below the cut. This is a JSON parameter question (P1 of the study), not a code
   one.

## 7. CLI reference (matching-related)

```
build/src/app/match_clusters -j <cat.json> [options]
  --outFolder <dir>            output folder (overrides matched_clusters_folder)
  --skip-files / --max-files   file range, overrides the JSON
  --first-in-time-partner      legacy ambiguity rule (keep the earliest candidate)
  -f                           overwrite existing outputs
  -v / -d                      verbose / debug
```

JSON keys used by this step: `time_tolerance_ticks`, `spatial_tolerance_cm` (unused),
`matched_clusters_folder`, plus the clustering-condition keys that build the folder
names.
