# Code Structure (current)

Concise map of the codebase after the latest source cleanup/refactor.

## Top-level layout

- `src/app/` – C++ executables (snake_case names; one file per app)
- `src/lib/` – shared library modules (`ThisCase.*` naming)
- `src/io/` – file/path discovery and folder logic (`InputOutput.*`)
- `src/backtracking/` – truth/position handling
- `src/clusters/` – clustering, matching, and cluster/volume helpers
- `src/objects/` – core domain objects (`TriggerPrimitive`, `Cluster`, `Neutrino`, `TrueParticle`)
- `src/ana/` – display/plotting library code used by C++ apps
- `python/` – maintained Python tools for image/volume generation and analysis
- `scripts/` – orchestration wrappers (`compile.sh`, `sequence.sh`, per-step runners)
- `parameters/` – runtime `.dat` parameter files
- `json/` – example settings and test JSON

## Naming convention in `src/`

- **Libraries**: `ThisCase.cpp` / `ThisCase.h`
  - Examples: `src/lib/Global.h`, `src/lib/Utils.cpp`, `src/clusters/MatchClusters.cpp`
- **Applications/executables**: `this_case.cpp`
  - Examples: `src/app/make_clusters.cpp`, `src/app/match_clusters.cpp`

## Data flow (pipeline)

1. `backtrack_tpstream` → signal TPs (`*_tps.root`)
2. `add_backgrounds` → mixed TPs (`tps_bg/*_tps_bg.root`)
3. `make_clusters` → single-plane clusters (`clusters_*/*_clusters.root`)
4. `match_clusters` → matched clusters (`matched_clusters_*/*_matched.root`)
5. Optional Python products:
   - cluster images (`cluster_images_*/*.npy`)
   - volume images (`volume_images_*/*.npz`)
   - volume analysis (`python/ana/analyze_volumes.py`)

`./scripts/sequence.sh` orchestrates these steps.

## Display tools in use

- C++: `display` (`src/app/display.cpp`, wrapper `scripts/display.sh`)
- Python: `python/ana/cluster_display.py` (wrapper `scripts/display_py.sh`)

## Core modules

- `src/lib/ParametersManager.*`: loads and serves `.dat` parameters (`GET_PARAM_*`)
- `src/lib/Global.*`: shared includes/logging globals
- `src/lib/Utils.*`: non-I/O utility helpers and parameter-backed constants
- `src/io/InputOutput.*`: input discovery, output-folder derivation, ROOT branch binding helpers
- `src/clusters/Clustering.*`: clustering primitives and cluster I/O
- `src/clusters/MatchClusters.*`: 3-plane matching logic

## Testing hook

- `test/run_all_tests.sh --clean` runs the smoke pipeline with `json/test_settings.json`.
