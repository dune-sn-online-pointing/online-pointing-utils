# Changelog

## Branch: `algorithmic_3d_reco`

This branch extends the pipeline with an algorithmic 3D reconstruction module and fixes
several bugs in backtracking and analysis that surfaced when working with new input file
formats and non-standard event layouts.

---

## Bug Fixes

### Non-consecutive event iteration (`src/app/backtrack_tpstream.cpp`)

The previous loop assumed events were numbered consecutively starting from `first_event`:

```cpp
for (int iEvent = first_event; iEvent < first_event + n_events; ++iEvent) { ... }
```

Hash-based file splitting produces tpstream files where event numbers are not contiguous
(e.g. events 3, 7, 14 rather than 0, 1, 2). This caused wrong events to be read or
the loop to access out-of-range entries.

**Fix:** a single pass over the `mctruths` tree collects the actual event numbers in
order of first occurrence, and the reconstruction loop iterates over that list. The
redundant second file-open that existed in a previous patch has also been removed.

---

### `truth_id` branch guard (`src/app/analyze_tps.cpp`)

`SetBranchAddress("truth_id", ...)` was called unconditionally. The new embedded-truth
file format omits this branch, causing ROOT to print an error and potentially write
garbage into the variable.

**Fix:** the call is now guarded by `tree->GetBranch("truth_id")`.

---

## New Features

### Per-plane `match_id` schema (`src/app/analyze_matching.cpp`)

The `make_clusters` + `match_clusters` pipeline has two output formats:

| Schema | Description |
|--------|-------------|
| **Legacy** | A dedicated `clusters_tree_multiplane` tree holds matched triplets |
| **Current** | Each per-plane tree (`clusters_tree_U/V/X`) carries a `match_id` branch; clusters with the same `match_id` form a triplet |

`analyze_matching` now auto-detects which schema is present and handles both. Full
U+V+X triplets are reconstructed from the per-plane `match_id` bitmask when the legacy
tree is absent. Truth purity metrics are read from the X-plane tree.

---

### JSON-driven analysis scripts

All three Python analysis scripts now take `-j <config.json>` and derive their input
folder from the same naming convention used by the C++ pipeline, instead of relying on
hardcoded paths:

| Script | Before | After |
|--------|--------|-------|
| `python/ana/analyze_clusters.py` | hardcoded path in source | `-j json/my_config.json` |
| `python/ana/analyze_matched_clusters.py` | `--sample es_valid` + hardcoded base | `-j json/my_config.json` |
| `python/ana/analyze_volumes.py` | hardcoded path in source | `-j json/my_config.json` (was already JSON but path logic was local) |

Folder resolution follows the priority: explicit `clusters_folder` key → derive from
`main_folder` / `signal_folder` / `tpstream_folder` + condition string.

---

### `analyze_volumes.py` plane-split support

Volume images are now written into per-plane subdirectories (`X/`, `U/`, `V/`) by the
pipeline. `analyze_volumes.py` previously found zero files in this layout.

`_resolve_npz_folder()` now detects whether `.npz` files live at the top level (old
layout) or in plane subdirectories (new layout) and picks the right path automatically.

---

### `analyze_matching.sh` overhaul

The script previously required a hand-written JSON pointing directly to a single
`matched_clusters_file`. It now accepts two input formats:

```bash
# Option 1: direct file reference (original behaviour)
./scripts/analyze_matching.sh -j json/my_direct_config.json

# Option 2: full pipeline JSON (infers the matched file automatically)
./scripts/analyze_matching.sh -j json/es_production.json
```

When a full pipeline JSON is supplied, the script derives the matched_clusters folder
from the same naming convention as the rest of the pipeline and picks the first
`*_matched.root` file it finds. It also uses `init.sh` / `findSettings.sh` for
environment setup, consistent with all other scripts in `scripts/`.

---

### `python/reco/` — 3D reconstruction module (new)

A new pure-Python module for algorithmic 3D point-cloud reconstruction from matched
U/V/X clusters. Does not require a compiled ROOT installation at runtime.

See **[docs/3D_RECO_GUIDE.md](docs/3D_RECO_GUIDE.md)** for full usage.

**Scripts added:**

| Script | Purpose |
|--------|---------|
| `python/reco/space_transformations.py` | Detector geometry: wire ID ↔ (x,y,z) conversions with precomputed LUTs |
| `python/reco/load_matched_clusters.py` | Read matched_clusters ROOT files into Python objects (`MatchedGroup`, `MatchedTriplet`) |
| `python/reco/reco_3d.py` | Core reconstruction: wire intersections → 3D candidates → OMP sparse selection |
| `python/reco/reco_display.py` | Interactive per-event viewer: U/V/X waveforms + 3D voxel cloud |
| `python/reco/get_3d_clouds.py` | Batch processor: ROOT → compressed NPZ dataset |
| `python/reco/plot_3d_clouds.py` | Standalone NPZ viewer (no ROOT dependency) |
| `python/reco/style.py` / `style.mplstyle` | Shared matplotlib style |

---

## Refactoring

### Shared Python utilities (`python/lib/utils.py`)

`sanitize()`, `get_clusters_folder()`, and `get_matched_clusters_folder()` were each
defined independently in six Python files. They are now in `python/lib/utils.py` and
imported everywhere else. The six files affected:

- `python/ana/analyze_clusters.py`
- `python/ana/analyze_matched_clusters.py`
- `python/ana/analyze_volumes.py`
- `python/app/create_volumes.py`
- `python/app/generate_cluster_arrays.py`
- `python/app/compute_truth_distances.py`

---

### RAII `TFile` management (`src/backtracking/Backtracking.cpp`, `src/app/backtrack_tpstream.cpp`)

Raw `TFile*` pointers previously used the pattern `file->Close(); delete file;`.
ROOT's `TFile` destructor also calls `Close()`, making this a double-close.

Both files now use `std::unique_ptr<TFile>`, matching the established pattern in the
codebase. Explicit `Close()` and `delete` calls removed.

---

### `true_interaction` → `is_es_interaction` (`python/clusters/cluster_display.py`)

`ClusterViewer` previously read a branch named `true_interaction` (string). The
cluster schema was changed in a prior branch to store a boolean `is_es_interaction`
instead (documented in `docs/TRUTH_CHAIN_SCHEMA.md`). The display script now reads
the correct branch name.

---

## Example Config Added

`json/example.json` is a template showing every key used by the pipeline. Copy it,
rename it, and update `signal_folder`, `bg_folder`, and `products_prefix` before use.

---

## Files Changed

```
json/es_production.json                  reverted local paths → shared EOS paths
json/example.json                        added (template config)
python/ana/analyze_clusters.py           JSON-driven, imports from utils
python/ana/analyze_matched_clusters.py   JSON-driven, imports from utils
python/ana/analyze_volumes.py            plane-split support, imports from utils
python/app/compute_truth_distances.py    imports sanitize from utils
python/app/create_volumes.py             imports from utils, removes local duplicates
python/app/generate_cluster_arrays.py   imports from utils, removes local duplicates
python/clusters/cluster_display.py      branch rename true_interaction → is_es_interaction
python/lib/utils.py                      sanitize(), get_clusters_folder(), get_matched_clusters_folder() added
python/reco/                             new module (7 files)
scripts/analyze_matching.sh              accept full pipeline JSON, use init.sh
src/app/analyze_matching.cpp             per-plane match_id schema support
src/app/analyze_tps.cpp                  truth_id branch guard
src/app/backtrack_tpstream.cpp          non-consecutive event fix, RAII TFile
src/backtracking/Backtracking.cpp       RAII TFile (unique_ptr)
docs/3D_RECO_GUIDE.md                   added (usage guide for python/reco/)
CHANGELOG.md                             added (this file)
```
