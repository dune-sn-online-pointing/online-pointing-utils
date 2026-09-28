#!/bin/bash
# Apply the radiological-cluster mask directly to already-rendered volume images
# (image-level blob approximation - no cluster ROOT files needed).
#
# Usage: apply_radmask_images.sh --cat-lo N --cat-hi M --base-dir DIR --out-base DIR \
#                                [--radius 40] [--energy-keep 2.0] [--out-suffix _radmask]
set +u
DEFAULT_HOME="/afs/cern.ch/work/e/evilla/private/dune/refactor-online-utils"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
[[ -f "$REPO_ROOT/scripts/init.sh" ]] || REPO_ROOT="${HOME_DIR:-$DEFAULT_HOME}"

ARGS=()
while [[ $# -gt 0 ]]; do
    case $1 in
        --home-dir) REPO_ROOT="$2"; shift 2 ;;
        *) ARGS+=("$1"); shift ;;
    esac
done

export HOME_DIR="$REPO_ROOT"
source "$REPO_ROOT/scripts/init.sh"
python3 "$REPO_ROOT/python/ana_evilla/apply_radmask_to_images.py" "${ARGS[@]}"
