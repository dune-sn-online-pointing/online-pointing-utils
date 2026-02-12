#!/bin/bash
set -e
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPTS_DIR/init.sh

print_help(){
  echo "Usage: $0 [--no-compile] [--clean-compile] [-v|--verbose] [input_root_file]"; exit 0;
}

cleanCompile=false
noCompile=false
verbose=false
INPUT_FILE="data/cleanES60_tps.root"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-compile) noCompile=true; shift;;
    --clean-compile) cleanCompile=true; shift;;
    -v|--verbose) verbose=true; shift;;
    -h|--help) print_help;;
    *) INPUT_FILE="$1"; shift;;
  esac
done

. $SCRIPTS_DIR/compile.sh -p $HOME_DIR --no-compile $noCompile --clean-compile $cleanCompile

cmd="$BUILD_DIR/src/app/plot_avg_times $INPUT_FILE"
if [ "$verbose" = true ]; then
  cmd="$cmd -v"
fi
echo "Running: $cmd"
exec $cmd
