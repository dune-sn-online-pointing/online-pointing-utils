#!/bin/bash
# HTCondor wrapper: produce U/V volume images for a range of burst-sample cats
set -e

CAT_LO="$1"
CAT_HI="$2"

if [[ -z "$CAT_LO" || -z "$CAT_HI" ]]; then
    echo "Usage: $0 <cat_lo> <cat_hi>"
    exit 1
fi

PROJECT_DIR="/afs/cern.ch/work/e/evilla/private/dune/refactor-online-utils"
cd "$PROJECT_DIR"
source "$PROJECT_DIR/scripts/init.sh"

echo "========================================="
echo "U/V VOLUME PRODUCTION - cats $CAT_LO-$CAT_HI"
echo "Host: $(hostname)  Started: $(date)"
echo "========================================="

python3 python/app/create_volumes_uv_for_cats.py \
    --samples-base /eos/project-e/ep-nu/evilla/sn-online-pointing/sn-burst-samples \
    --conditions tick3_ch2_min2_tot3_e3p0 \
    --cat-range "$CAT_LO" "$CAT_HI"

echo "Finished: $(date)"
