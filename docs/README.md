# Online Pointing Utils

End-to-end toolkit for the DUNE supernova online pointing chain: backtracking TPStreams, overlaying backgrounds, clustering, 3-plane matching, and producing analysis/visualization products (cluster displays and volume images).

This page is the expanded, single-stop README. It replaces scattered docs and mirrors the current state of the code and scripts.

## 1) Quickstart

Prerequisites
- C++17 toolchain, CMake, ROOT
- Python 3.9+ with `python/requirements.txt` (only needed for the Python utilities)

Build (use the wrapper to stay portable across macOS/Linux)
```bash
./scripts/compile.sh          # uses build/; add -c to clean, -n to skip build
```

Run the standard pipeline
```bash
./scripts/sequence.sh -s es_valid -j json/example_es_clean.json --all
```

Smoke test (what CI uses locally)
```bash
./test/run_all_tests.sh --clean
```

## 2) Pipeline at a Glance

1. Backtrack TPStreams → `*_tps.root` (signal-only with truth)
2. Add backgrounds → `tps_bg/*_tps_bg.root`
3. Make clusters → `clusters_<prefix>_<conds>/*_clusters.root`
4. Match clusters (3-plane) → `matched_clusters_<prefix>_<conds>/*_matched.root`
5. Optional Python volumes → `volume_images_<prefix>_<conds>/*.npz`

The `scripts/sequence.sh` wrapper runs the steps above in order and recompiles once at the start. Use `--all` for the full chain or the per-step flags (`-bt`, `-ab`, `-mc`, `-mm`, `-vi`).

## 3) Configuration

- **JSON settings**: keep user configs under `json/` (only the examples + `test_settings.json` are tracked). See [json/README.md](json/README.md) for allowed keys and folder auto-generation.
- **Parameters**: `.dat` files in `parameters/` (documented in [parameters/PARAMETERS.md](parameters/PARAMETERS.md)). Set `PARAMETERS_DIR` if you keep them elsewhere.
- **Environment**: source `scripts/init.sh` (done automatically by the wrappers) to define `HOME_DIR`, `SCRIPTS_DIR`, and `BUILD_DIR`.

## 4) Apps and Scripts

- C++ executables live in `src/app/` and are built to `build/src/app/` (or `bin/` if installed). See [APPS.md](APPS.md) for the full per-app rundown.
- Primary wrappers (in `scripts/`): `sequence.sh`, `compile.sh`, `backtrack.sh`, `add_backgrounds.sh`, `make_clusters.sh`, `match_clusters.sh`, `display.sh`, `create_volumes.sh`, `view_volumes.sh`.
- Event displays: `display` (C++) via `scripts/display.sh` and the Python `cluster_display.py` (see [python/README.md](python/README.md) for options).

## 5) Folder and Naming Conventions

- Basenames are preserved across stages for traceability.
- Default layout under your `tpstream_folder` (or `signal_folder`):
	- `tpstreams/` → inputs
	- `tps/` → backtracked signal
	- `tps_bg/` → backgrounds mixed in
	- `clusters_<prefix>_<conds>/` → single-plane clusters
	- `matched_clusters_<prefix>_<conds>/` → 3-plane matches
	- `volume_images_<prefix>_<conds>/` and `cluster_images_.../` → Python/plot products
	- `reports/` → analysis summaries
- The conditions string is `tick{N}_ch{N}_min{N}_tot{N}_e{X}` (e.g., `tick3_ch2_min2_tot1_e0p0`).

## 6) Python Utilities (maintained set)

- Volume workflow: `python/app/create_volumes.py`, `python/ana/analyze_volumes.py`, `python/ana/view_volume_quick.py`
- Displays: `python/ana/cluster_display.py` (see [python/README.md](python/README.md))
- Other scripts under `python/ana/` are legacy/analysis helpers; keep them out of automated pipelines unless you know they are needed.

Install deps (isolated env recommended)
```bash
python -m venv .venv && source .venv/bin/activate
pip install -r python/requirements.txt
```

## 7) Testing

- `test/run_all_tests.sh` runs the smoke pipeline with `json/test_settings.json` on one file. It recompiles unless you pass `--no-compile` through to the wrapper.

## 8) Documentation Map

- [APPS.md](APPS.md): per-app and per-script details
- [CONFIGURATION.md](CONFIGURATION.md): environment + JSON + parameters (current keys)
- [json/README.md](json/README.md): JSON conventions and examples
- [parameters/PARAMETERS.md](parameters/PARAMETERS.md): parameter files/keys
- [code-structure.md](code-structure.md): concise architecture and folder map

Part of the DUNE DAQ Application Framework. See COPYING for licensing details.

## 9) Contacts

DUNE Supernova Online Pointing Group
- Repository: [dune-sn-online-pointing/online-pointing-utils](https://github.com/dune-sn-online-pointing/online-pointing-utils)
- DUNE Collaboration: [dunescience.org](https://www.dunescience.org/)
