#!/bin/bash
set -e
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPTS_DIR/init.sh

print_help(){
  echo "Usage: $0 -j <json> [--no-compile] [--clean-compile] [--output-folder <dir>] [-v|--verbose]"; exit 0;
}

settingsFile="json/cluster/example.json"
cleanCompile=false
noCompile=false
verbose=false
# output_folder="data" # no standard

while [[ $# -gt 0 ]]; do
  case "$1" in
    -j|--json) settingsFile="$2"; shift 2;;
    --output-folder) output_folder="$2"; shift 2;;
    --no-compile) noCompile=true; shift;;
    --clean-compile) cleanCompile=true; shift;;
    -v|--verbose) verbose=true; shift;;
    -h|--help) print_help;;
    *) shift;;
  esac
done

settingsFile=$($SCRIPTS_DIR/findSettings.sh -j $settingsFile | tail -n 1)
. $SCRIPTS_DIR/compile.sh -p $HOME_DIR --no-compile $noCompile --clean-compile $cleanCompile

cmd="$BUILD_DIR/src/app/make_clusters -j $settingsFile"
if [[ -n $output_folder ]]; then
  cmd+=" --outFolder $output_folder"
fi
if [ "$verbose" = true ]; then
  cmd+=" -v"
fi
echo "Running: $cmd"
exec $cmd
