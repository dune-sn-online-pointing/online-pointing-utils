#!/usr/bin/env bash
set -euo pipefail
HOME_PATH=$(cd "$(dirname "$0")/.." && pwd)
JSON=${1:-json/analyze_tps/example_bktr5_clean_tot_sweep.json}
TOT_LIST=${2:-"0 5 10 15"}

PYTHON=python3
BASE_JSON=$(mktemp)
cp "$JSON" "$BASE_JSON"

for tot in $TOT_LIST; do
  TMP=$(mktemp --suffix=.json)
  $PYTHON - "$BASE_JSON" "$TMP" "$tot" <<'PY'
import json,sys
base = json.load(open(sys.argv[1]))
base['tot_cut'] = int(sys.argv[3])
json.dump(base, open(sys.argv[2],'w'))
PY
  echo "[analyze_tps_sweep] Running tot_cut=$tot"
  "$HOME_PATH/scripts/analyze_tps.sh" -j "$TMP"
  rm -f "$TMP"
done
rm -f "$BASE_JSON"
