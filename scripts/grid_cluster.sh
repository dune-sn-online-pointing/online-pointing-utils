#!/usr/bin/env bash
set -euo pipefail
HOME_PATH=$(cd "$(dirname "$0")/.." && pwd)
JSON=${1:-json/cluster/grid_clean60.json}

# Read grid from JSON using python (available) or fall back to fixed values
PYTHON=python3
TICK_LIMITS=$($PYTHON - "$JSON" <<'PY'
import json,sys
j=json.load(open(sys.argv[1]))
print(" ".join(map(str,j.get("tick_limits",[3]))))
PY
)
CHANNEL_LIMITS=$($PYTHON - "$JSON" <<'PY'
import json,sys
j=json.load(open(sys.argv[1]))
print(" ".join(map(str,j.get("channel_limits",[1]))))
PY
)
TOT_CUTS=$($PYTHON - "$JSON" <<'PY'
import json,sys
j=json.load(open(sys.argv[1]))
print(" ".join(map(str,j.get("tot_cuts",[0]))))
PY
)
FILELIST=$($PYTHON - "$JSON" <<'PY'
import json,sys
j=json.load(open(sys.argv[1]))
print(j.get("filelist",""))
PY
)
OUTFOLDER=$($PYTHON - "$JSON" <<'PY'
import json,sys
j=json.load(open(sys.argv[1]))
print(j.get("output_folder","data"))
PY
)

# Iterate grid
for tl in $TICK_LIMITS; do
  for ch in $CHANNEL_LIMITS; do
    for tot in $TOT_CUTS; do
      TMPJSON=$(mktemp)
      cat > "$TMPJSON" <<EOT
{
  "filelist": "$FILELIST",
  "output_folder": "$OUTFOLDER",
  "tick_limit": $tl,
  "channel_limit": $ch,
  "min_tps_to_cluster": 1,
  "adc_integral_cut_induction": 0,
  "adc_integral_cut_collection": 0,
  "tot_cut": $tot
}
EOT
      echo "[grid] Running cluster tl=$tl ch=$ch tot=$tot"
      "$HOME_PATH/build/src/app/cluster" -j "$TMPJSON"
      rm -f "$TMPJSON"
    done
  done
done

# Analyze resulting cluster files with analyze_clusters
LIST_CLUSTERS=$(mktemp)
ls "$OUTFOLDER"/*_clusters_tick*_*_tot*.root | grep -E "clean(ES|CC)60_" > "$LIST_CLUSTERS" || true
AJ=$(mktemp)
cat > "$AJ" <<EOT
{ "filelist": "$LIST_CLUSTERS", "output_folder": "$OUTFOLDER" }
EOT
"$HOME_PATH/build/src/app/analyze_clusters" -j "$AJ"
rm -f "$LIST_CLUSTERS" "$AJ"
