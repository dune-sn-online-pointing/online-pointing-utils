#!/usr/bin/env bash
set -euo pipefail

# Run extract_calibration over a file or a filelist JSON, overriding output folder and ToT cuts if provided.
# Usage: scripts/extract_calibration.sh path/to/config.json [outdir] [tot_cut]

JSON=${1:-}
OUTDIR=${2:-}
TOT=${3:-}
VERBOSE=false

# Parse flags
for arg in "$@"; do
  case "$arg" in
    -v|--verbose) VERBOSE=true; shift;;
  esac
done

if [[ -z "${JSON}" ]]; then
  echo "Usage: $0 path/to/config.json [outdir] [tot_cut] [-v|--verbose]" >&2
  exit 1
fi

ARGS=( -j "${JSON}" )
if [[ -n "${OUTDIR}" ]]; then
  mkdir -p "${OUTDIR}"
  ARGS+=( --output-folder "${OUTDIR}" )
fi

# If tot provided, create a temp JSON that overrides tot_cut
if [[ -n "${TOT}" ]]; then
  TMP=$(mktemp --suffix=.json)
  cp "${JSON}" "$TMP"
  # naive update tot_cut (works if key exists); otherwise append
  if grep -q '"tot_cut"' "$TMP"; then
    sed -i -E "s/\"tot_cut\"\s*:\s*[0-9]+/\"tot_cut\": ${TOT}/" "$TMP"
  else
    # insert before closing brace
    sed -i -E "s/}\s*$/  , \"tot_cut\": ${TOT}\n}/" "$TMP"
  fi
  JSON="$TMP"
  ARGS=( -j "$JSON" )
fi

# Build if needed
if ! command -v extract_calibration >/dev/null 2>&1; then
  echo "Building extract_calibration..." >&2
  cmake -S . -B build
  cmake --build build --target extract_calibration -j
  PATH="build/app:${PATH}"
fi

# Add verbose flag if set
if [[ "$VERBOSE" = true ]]; then
  ARGS+=( -v )
fi

# Run
if [[ -x build/app/extract_calibration ]]; then
  build/app/extract_calibration "${ARGS[@]}"
else
  extract_calibration "${ARGS[@]}"
fi
