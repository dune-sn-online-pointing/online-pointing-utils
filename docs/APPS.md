# Apps and Scripts

This project is driven by a small set of C++ executables (built with CMake) and a set of bash scripts that orchestrate the typical workflows using JSON settings.

## The main entrypoint: `scripts/sequence.sh`

`scripts/sequence.sh` is the “pipeline runner”. It can execute multiple steps in the standard order (backtrack → backgrounds → clusters → matching → images/volumes) and it compiles the C++ code once at the beginning.

Key points:
- It expects a **single JSON settings file** via `-j|--json-settings`.
- It locates JSON files via `scripts/findSettings.sh` (by default, settings are searched under the `json/` directory unless you provide an absolute path).
- Use `--all` to run the whole chain, or step flags (e.g. `-bt`, `-mc`, `-mm`) to run subsets.

Example:
```bash
./scripts/sequence.sh -s es_valid -j json/example_es_clean.json --all
```

## Bash scripts (wrappers)

Most scripts are thin wrappers around the compiled executables in `build/src/app/` (or around Python entry points), plus common folder-structure and settings-handling logic.

The most commonly used ones are:
- `scripts/init.sh`: sets repo environment variables used by the scripts.
- `scripts/compile.sh`: configures/builds the C++ code.
- `scripts/backtrack.sh`: runs C++ `backtrack_tpstream`.
- `scripts/add_backgrounds.sh`: runs C++ `add_backgrounds`.
- `scripts/make_clusters.sh`: runs C++ `make_clusters`.
- `scripts/match_clusters.sh`: runs C++ `match_clusters`.
- `scripts/analyze_tps.sh`: runs C++ `analyze_tps`.
- `scripts/analyze_clusters.sh`: runs C++ `analyze_clusters`.
- `scripts/analyze_matching.sh`: runs C++ `analyze_matching`.
- `scripts/display.sh`: runs C++ `display`.
- `scripts/create_volumes.sh`: runs Python `python/app/create_volumes.py`.
- `scripts/view_volumes.sh`: quick visualization helpers for volume outputs.

(There are additional utility/scan/Condor helper scripts in `scripts/` which are generally not part of the “core” pipeline.)

## C++ executables

Executables are built under `build/src/app/` (or installed to `bin/` if you run `cmake --install`).

From `src/app/CMakeLists.txt`:
- `backtrack_tpstream`: reads TPStreams and enriches/filters using truth/backtracking.
- `add_backgrounds`: overlays background/noise on top of signal.
- `make_clusters`: builds 2D clusters from trigger primitives.
- `match_clusters`: performs 3-plane matching.
- `match_clusters_truth`: matching validation/diagnostics with truth.
- `analyze_tps`: TP-level diagnostics.
- `analyze_clusters`: cluster-level diagnostics.
- `analyze_matching`: matching-level diagnostics.
- `display`: visualization of TPs/clusters.
- `extract_calibration`: extracts calibration-related quantities.
- `extract_energy_cut_stats`: collects energy-cut statistics.
- `diagnose_timing`: timing diagnostics.
- `plot_avg_times`: timing/throughput plots.
- `split_by_apa`: splits inputs by APA (useful for multi-APA studies and debugging).

## Python entry points

Python scripts live under `python/`.

Common ones:
- `python/app/create_volumes.py`: generates per-cluster volume windows and saves them as NPZ (+ optional PNG overlays).
- `python/ana/analyze_volumes.py`: post-processes volume outputs and produces summary text/PDF-style plots.
- `python/ana/view_volume_quick.py`: quick inspection of a single volume output.

Python dependencies are listed in `python/requirements.txt`.