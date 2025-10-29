#!/bin/bash
set -e
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPTS_DIR/init.sh

print_help(){
  echo "Usage: $0 -j <json> [-i <input>] [-o <output>] [-s <skip>] [-m <max>] [--no-compile] [--clean-compile] [-v|--verbose]"; 
  echo "Options:";
  echo "  -j|--json <file>          JSON settings file with pattern (e.g. json/make_clusters/example.json)"
  echo "  -i|--input-file <file>    Input file or list (overrides JSON)"
  echo "  -o|--output-folder <dir>  Output folder (overrides the one in JSON file)"
  echo "  -s|--skip <num>           Number of files to skip at start (overrides JSON)"
  echo "  -m|--max <num>            Maximum number of files to process (overrides JSON)"
  echo "  --no-compile              Do not recompile the code"
  echo "  --clean-compile           Clean and recompile the code"
  echo "  -f|--override [true|false] Force reprocessing even if output already exists (useful for debugging)"
  echo "  -v|--verbose              Enable verbose output"
  echo "  -d|--debug                Enable debug mode"
  echo "  -h|--help                 Print this help message."
  exit 0;
}

settingsFile="json/cluster/example.json"
cleanCompile=false
noCompile=false
verbose=false
inputFile=""
output_folder=""
skip_files=""
max_files=""
# output_folder="data" # no standard

while [[ $# -gt 0 ]]; do
  case "$1" in
    -j|--json) settingsFile="$2"; shift 2;;
    -i|--input-file) inputFile="$2"; shift 2;;
    -o|--output-folder) output_folder="$2"; shift 2;;
    -s|--skip) skip_files="$2"; shift 2;;
    -m|--max) max_files="$2"; shift 2;;
    --no-compile) noCompile=true; shift;;
    --clean-compile) cleanCompile=true; shift;;        
    -f|--override)
      if [[ $2 == "true" || $2 == "false" ]]; then
      override=$2
      shift 2
      else
      override=true
      shift
      fi
      ;;
    -v|--verbose) 
      if [[ $2 == "true" || $2 == "false" ]]; then
        verbose=$2
        shift 2
      else
        verbose=true
        shift
      fi
      ;;
    -d|--debug) 
      if [[ $2 == "true" || $2 == "false" ]]; then
        debug=$2
        shift 2
      else
        debug=true
        shift
      fi
      ;;
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
if [ ! -z "$skip_files" ]; then
  cmd+=" -s $skip_files"
fi
if [ ! -z "$max_files" ]; then
  cmd+=" -m $max_files"
fi
if [ "$override" = true ]; then
  cmd+=" -f"
fi
if [ "$verbose" = true ]; then
  cmd+=" -v"
fi
if [ "$debug" = true ]; then
  cmd+=" -d"
fi
echo "Running: $cmd"
exec $cmd
