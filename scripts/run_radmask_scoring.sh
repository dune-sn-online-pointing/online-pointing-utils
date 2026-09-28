#!/bin/bash
# Score a CT model on original vs radiological-masked volume images.
# Uses the LCG_106_cuda view (tensorflow) via the ml-for-pointing init script.
set +u
DEFAULT_HOME="/afs/cern.ch/work/e/evilla/private/dune/refactor-online-utils"
ML_INIT="/afs/cern.ch/work/e/evilla/private/dune/refactor-ml-for-pointing/scripts/init.sh"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
[[ -f "$REPO_ROOT/python/ana_evilla/radmask_score_ct.py" ]] || REPO_ROOT="${HOME_DIR:-$DEFAULT_HOME}"

ARGS=()
while [[ $# -gt 0 ]]; do
    case $1 in
        --home-dir) REPO_ROOT="$2"; shift 2 ;;
        *) ARGS+=("$1"); shift ;;
    esac
done

if [[ -f "$ML_INIT" ]]; then
    source "$ML_INIT"
else
    source /cvmfs/sft.cern.ch/lcg/views/LCG_106_cuda/x86_64-el9-gcc11-opt/setup.sh
fi
export TF_CPP_MIN_LOG_LEVEL=3
python3 "$REPO_ROOT/python/ana_evilla/radmask_score_ct.py" "${ARGS[@]}"
