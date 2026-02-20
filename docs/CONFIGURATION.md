# Configuration Guide

How to configure and run the current pipeline (matches the wrappers and tracked examples).

## Layers

1) Parameters (`parameters/*.dat`)
- Geometry: `geometry.dat`
- Timing: `timing.dat`
- Conversions: `conversion.dat`
- Detector: `detector.dat`
- Analysis/thresholds: `analysis.dat`, `display.dat`
- Format: `< key = value >    # comment` (see [parameters/PARAMETERS.md](parameters/PARAMETERS.md))

2) JSON settings (`json/*.json`)
- Only example configs + `test_settings.json` are tracked; personal/production configs stay untracked.
- Keys cover inputs, folder auto-generation, clustering/matching knobs, and step toggles. See [json/README.md](json/README.md).

3) Environment
- Sourced automatically by `scripts/init.sh` (called by every wrapper):
  - `HOME_DIR`: repo root
  - `SCRIPTS_DIR`: scripts directory
  - `BUILD_DIR`: build directory (default `build/`)
  - `PARAMETERS_DIR`: defaults to `parameters/` in the repo; override if needed

4) Orchestration
- Use `scripts/sequence.sh -j <json> --all` for the full chain or per-step flags for subsets.
- Use `scripts/compile.sh` for builds (handles macOS/Linux core detection).

## JSON essentials

Typical minimal config
```json
{
  "signal_folder": "/data/prod_es",
  "clusters_folder_prefix": "es_valid",
  "tick_limit": 3,
  "channel_limit": 2,
  "min_tps_to_cluster": 2,
  "tot_cut": 1,
  "energy_cut": 0.0,
  "max_files": 10
}
```

Folder auto-generation (if you do not override explicit paths)
- `tpstreams/` under `signal_folder` or `main_folder`
- `tps/`, `tps_bg/`, `clusters_<prefix>_<conds>/`, `matched_clusters_<prefix>_<conds>/`, `volume_images_<prefix>_<conds>/`, `reports/`
- Conditions string: `tick{N}_ch{N}_min{N}_tot{N}_e{X}` (decimal → `p`)

Discovery logic
- Prefer explicit keys (`tpstream_input_file`, `tps_bg_folder`, `clusters_folder`, etc.).
- Otherwise auto-generate from `signal_folder` / `main_folder` using the rules above.

## Environment setup

Automatic (recommended)
```bash
source scripts/init.sh
```

Manual override example
```bash
export PARAMETERS_DIR=/path/to/params
export BUILD_DIR=/scratch/build
```

## Build and run

- Build: `./scripts/compile.sh [-c|--clean-compile] [-n|--no-compile] [-j N]`
- Full chain: `./scripts/sequence.sh -s <sample> -j json/example_es_clean.json --all`
- Single step example: `./scripts/make_clusters.sh -j json/example_es_clean.json --max-files 5`

## Validation / troubleshooting

- Smoke test: `./test/run_all_tests.sh --clean`
- Check params in use: `echo $PARAMETERS_DIR && ls $PARAMETERS_DIR`
- Verbose script output: add `-v` to the bash wrappers

## Compatibility notes

- Basenames are preserved across stages to keep skip/max consistent.
- Legacy, per-app JSONs are untracked; prefer the single settings file per run and pass it through `sequence.sh`.