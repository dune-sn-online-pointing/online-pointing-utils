#!/bin/bash
set -e
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPTS_DIR/init.sh

print_help(){
  echo "Usage: $0 -j <json> [--no-compile] [--clean-compile] [--output-folder <dir>]"; exit 0;
}

settingsFile="json/cluster/example.json"
cleanCompile=false
noCompile=false
output_folder="data"

while [[ $# -gt 0 ]]; do
  case "$1" in
    -j|--json) settingsFile="$2"; shift 2;;
    --output-folder) output_folder="$2"; shift 2;;
    --no-compile) noCompile=true; shift;;
    --clean-compile) cleanCompile=true; shift;;
    -h|--help) print_help;;
    *) shift;;
  esac
done

settingsFile=$($SCRIPTS_DIR/findSettings.sh -j $settingsFile | tail -n 1)
. $SCRIPTS_DIR/compile.sh -p $HOME_DIR --no-compile $noCompile --clean-compile $cleanCompile

cmd="$BUILD_DIR/src/app/cluster -j $settingsFile --output-folder $output_folder"
echo "Running: $cmd"
exec $cmd
