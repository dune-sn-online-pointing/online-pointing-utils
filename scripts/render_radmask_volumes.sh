#!/bin/bash
# Render volume images with the truth-free radiological-cluster mask enabled.
#
# Thin condor-friendly driver: renders one cat, one or more (radius, energy-keep)
# variants, X plane only, into <signal_folder>/<prefix>_volume_images_<cond><suffix>.
# It never touches the existing (unmasked) products.
#
# Usage:
#   render_radmask_volumes.sh -j <cat json> [--variants "R:Ekeep:suffix,..."] \
#                             [--planes X] [--skip N] [--max N] [-f]
#
# Default variants: 40 cm / 2 MeV -> "_radmask"   (conservative, recommended)
#                   30 cm / 2 MeV -> "_radmask30" (aggressive)

set +u
# HTCondor copies the executable to the worker node, so the script location cannot be
# used to find the repository: fall back to the absolute AFS path (overridable).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DEFAULT_HOME="/afs/cern.ch/work/e/evilla/private/dune/refactor-online-utils"
if [[ ! -f "$REPO_ROOT/scripts/init.sh" ]]; then
    REPO_ROOT="${HOME_DIR:-$DEFAULT_HOME}"
fi

JSON_FILE=""
VARIANTS="40:2.0:_radmask,30:2.0:_radmask30"
PLANES="X"
EXTRA=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -j|--json)     JSON_FILE="$2"; shift 2 ;;
        --home-dir)    REPO_ROOT="$2"; shift 2 ;;
        --variants)    VARIANTS="$2"; shift 2 ;;
        --planes)      PLANES="$2"; shift 2 ;;
        --skip|--skip-files) EXTRA="$EXTRA --skip $2"; shift 2 ;;
        --max|--max-files)   EXTRA="$EXTRA --max $2"; shift 2 ;;
        -f|--override) EXTRA="$EXTRA -f"; shift ;;
        -v|--verbose)  EXTRA="$EXTRA -v"; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

if [[ -z "$JSON_FILE" || ! -f "$JSON_FILE" ]]; then
    echo "Error: valid -j <json> required (got '$JSON_FILE')"; exit 1
fi

export HOME_DIR="$REPO_ROOT"
echo "Repository home: $REPO_ROOT"
source "$REPO_ROOT/scripts/init.sh"

RC=0
IFS=',' read -ra VS <<< "$VARIANTS"
for v in "${VS[@]}"; do
    IFS=':' read -r R EK SUF <<< "$v"
    echo "=================================================================="
    echo "radmask variant: radius=${R} cm  energy_keep=${EK} MeV  suffix=${SUF}"
    echo "=================================================================="
    python3 "$REPO_ROOT/python/app/create_volumes.py" -j "$JSON_FILE" \
        --planes "$PLANES" --radmask --radmask-radius "$R" \
        --radmask-energy-keep "$EK" --radmask-suffix "$SUF" $EXTRA
    st=$?
    if [[ $st -ne 0 ]]; then echo "variant $v FAILED (exit $st)"; RC=$st; fi
done
exit $RC
