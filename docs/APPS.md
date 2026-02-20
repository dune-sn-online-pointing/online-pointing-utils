# Apps and Scripts

This is the authoritative list of what we ship today: C++ executables (built with CMake) plus the bash/Python wrappers that drive the standard workflows.

## The main entrypoint: `scripts/sequence.sh`

`scripts/sequence.sh` is the pipeline runner. It orchestrates backtracking → background overlay → clustering → 3-plane matching → (optional) volume/image creation and triggers a one-time compile via `scripts/compile.sh` at startup.

Key usage
- One JSON settings file via `-j|--json-settings` (found with `scripts/findSettings.sh`).
- `--all` runs the full chain; use step flags (`-bt`, `-ab`, `-mc`, `-mm`, `-vi`) for subsets.
- Example: `./scripts/sequence.sh -s es_valid -j json/example_es_clean.json --all`

## Bash wrappers (core set)

- `scripts/compile.sh`: configure/build in `build/` (portable core-count handling)
- `scripts/backtrack.sh`: C++ `backtrack_tpstream`
- `scripts/add_backgrounds.sh`: C++ `add_backgrounds`
- `scripts/make_clusters.sh`: C++ `make_clusters`
- `scripts/match_clusters.sh`: C++ `match_clusters`
- `scripts/display.sh`: C++ `display` (cluster/TP event display)
- `scripts/create_volumes.sh`: Python `python/app/create_volumes.py`
- `scripts/view_volumes.sh`: quick NPZ visualization
- Analysis helpers: `scripts/analyze_tps.sh`, `scripts/analyze_clusters.sh`, `scripts/analyze_matching.sh`

(Other scripts under `scripts/` are utility/Condor helpers and not part of the main path.)

## C++ executables (built to `build/src/app/`)

- `backtrack_tpstream`: TP truth matching and signal filtering
- `add_backgrounds`: overlay background/noise on signal TPs
- `make_clusters`: 2D clustering with ToT/energy cuts + main-track tagging
- `match_clusters`: 3-plane matching (Pentagon algorithm)
- `match_clusters_truth`: matching validation against truth
- `analyze_tps`: TP-level diagnostics
- `analyze_clusters`: cluster-level diagnostics
- `analyze_matching`: matching-level diagnostics
- `display`: TP/cluster display (ROOT-based)
- `extract_calibration`: calibration quantities
- `extract_energy_cut_stats`: energy-cut statistics
- `diagnose_timing`: timing diagnostics
- `plot_avg_times`: timing/throughput plots
- `split_by_apa`: APA-splitting helper (multi-APA debugging)

## Python entry points

Maintained
- `python/app/create_volumes.py`: build per-cluster volume windows (NPZ, optional PNG overlays)
- `python/ana/analyze_volumes.py`: summarize NPZ outputs
- `python/ana/view_volume_quick.py`: inspect one volume NPZ
- `python/ana/cluster_display.py`: matplotlib display of clusters/events (see [python/README.md](python/README.md))

Legacy / ad-hoc analysis
- Additional scripts under `python/ana/` are older analysis helpers; keep them out of production pipelines unless explicitly needed.

Python dependencies live in `python/requirements.txt`.