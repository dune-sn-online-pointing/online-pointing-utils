#!/usr/bin/env bash
set -euo pipefail

# cluster_display.sh - helper to launch the interactive MARLEY cluster viewer
# Usage:
#   scripts/cluster_display.sh -c data/clusters_cleanES60_tick5_chan2_tot2.root
# Options:
#   -c|--clusters   Path to clusters ROOT file (required)
#   -j|--json       Optional JSON file with parameters
#   --tick-limit    Tick proximity (used only if falling back to TPs)
#   --channel-limit Channel proximity (used only if falling back to TPs)
#   --min-tps       Min TPs per cluster (used only if falling back to TPs)
#   -f|--filename   TP ROOT (only if no clusters file)

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT_DIR/build/src/app/cluster_display"

if [[ ! -x "$BIN" ]]; then
  echo "Building cluster_display..." >&2
  cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build" -DCMAKE_BUILD_TYPE=Release >/dev/null
  cmake --build "$ROOT_DIR/build" --target cluster_display -j >/dev/null
fi

ARGS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    -c|--clusters) ARGS+=("--clusters-file" "$2"); shift 2;;
    -j|--json) ARGS+=("-j" "$2"); shift 2;;
    --tick-limit) ARGS+=("--tick-limit" "$2"); shift 2;;
    --channel-limit) ARGS+=("--channel-limit" "$2"); shift 2;;
    --min-tps) ARGS+=("--min-tps" "$2"); shift 2;;
    -f|--filename) ARGS+=("--filename" "$2"); shift 2;;
    -h|--help) echo "See header of this script for usage."; exit 0;;
    *) echo "Unknown option: $1" >&2; exit 2;;
  esac
done

exec "$BIN" "${ARGS[@]}"
