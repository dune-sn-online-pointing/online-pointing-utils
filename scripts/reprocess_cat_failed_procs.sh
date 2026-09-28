#!/bin/bash

set -u

SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export SCRIPTS_DIR
export HOME_DIR=$(dirname "$SCRIPTS_DIR")
source "$HOME_DIR/scripts/init.sh"

cat_id=""
proc_list=""
max_files=20

print_help() {
  echo "Usage: $0 --cat <cat_id> --procs <comma_separated_proc_ids> [--max-files <N>] [--home-dir <path>]"
  exit 1
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --cat)
      cat_id="$2"
      shift
      ;;
    --procs)
      proc_list="$2"
      shift
      ;;
    --max-files)
      max_files="$2"
      shift
      ;;
    --home-dir)
      HOME_DIR="$2"
      export HOME_DIR
      shift
      ;;
    -h|--help)
      print_help
      ;;
    *)
      echo "Unknown argument: $1"
      print_help
      ;;
  esac
  shift
done

if [[ -z "$cat_id" || -z "$proc_list" ]]; then
  echo "Error: --cat and --procs are required."
  print_help
fi

# Normalize CAT id as base-10 to avoid bash printf octal interpretation
# (e.g. 000593 would otherwise become 000005).
cat_id_num=$((10#$cat_id))
cat_id=$(printf "%06d" "$cat_id_num")
json_file="$HOME_DIR/json/cats/cat_${cat_id}.json"

if [[ ! -f "$json_file" ]]; then
  echo "Error: missing JSON file $json_file"
  exit 2
fi

IFS=',' read -r -a procs <<< "$proc_list"

overall_rc=0
for proc_id in "${procs[@]}"; do
  if [[ -z "$proc_id" ]]; then
    continue
  fi

  skip_files=$((proc_id * max_files))
  echo "[cat${cat_id}] Reprocessing proc=${proc_id} skip=${skip_files} max=${max_files}"

  "$HOME_DIR/scripts/sequence.sh" \
    -j "$json_file" \
    --home-dir "$HOME_DIR" \
    --no-compile \
    -gv -f \
    --skip-files "$skip_files" \
    --max-files "$max_files"

  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "[cat${cat_id}] proc=${proc_id} failed with rc=$rc"
    overall_rc=1
  fi
done

exit $overall_rc
