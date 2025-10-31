#!/bin/bash

set -e
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Use absolute path to init.sh from AFS when running on worker nodes
if [[ -f "${SCRIPTS_DIR}/init.sh" ]]; then
    source ${SCRIPTS_DIR}/init.sh
else
    # Fallback: use HOME_DIR if set (passed from parent script), or search up for AFS path
    if [[ -n "${HOME_DIR:-}" ]]; then
        source ${HOME_DIR}/scripts/init.sh
    else
        echo "ERROR: Cannot find init.sh and HOME_DIR not set"
        exit 1
    fi
fi

print_help(){
  echo "Usage: $0 -j <json> [--no-compile] [--clean-compile] [-o <dir>]"; 
  echo "Options:";
  echo "  -j|--json <json>            Json file containing settings."
  echo "  --no-compile                Do not recompile the code"
  echo "  --clean-compile             Clean and recompile the code"
  echo "  -i|--input-file <file>      Input file with list of input ROOT files. Overrides json."
  echo "  -o|--output-folder <dir>    Output folder (default: from json). Overrides json."
  echo "  --max-files <n>             Maximum number of files to process. Overrides json."
  echo "  --skip-files <n>            Number of files to skip at start. Overrides json."
  echo "  -f|--override               Force reprocessing even if output already exists (default: false)"
  echo "  -v|--verbose                Enable verbose mode"
  echo "  -h|--help                   Print this help message."
  exit 0;
}

# settingsFile="json/backtrack/example.json"
cleanCompile=false
noCompile=false
override=false
verbose=false
max_files=""
skip_files=""
# output_folder="" # a default could be data/

while [[ $# -gt 0 ]]; do
  case "$1" in
    -j|--json) settingsFile="$2"; shift 2;;
    -i|--input-file) inputFile="$2"; shift 2;;
    -o|--output-folder) output_folder="$2"; shift 2;;
    --max-files) max_files="$2"; shift 2;;
    --skip-files) skip_files="$2"; shift 2;;
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

if [ -z "$settingsFile" ]
then
    echo " Please specify the settings file, using flag -j <settings_file>. Stopping execution."
    print_help
fi

find_settings_command="$SCRIPTS_DIR/findSettings.sh -j $settingsFile"
echo " Looking for json settings using command: $find_settings_command"
settingsFile=$($find_settings_command | tail -n 1)
echo -e "Settings file found, full path is: $settingsFile \n"

################################################
# Compile the code if requested
compile_command="$SCRIPTS_DIR/compile.sh -p $HOME_DIR --no-compile $noCompile --clean-compile $cleanCompile"
echo "Compiling the code with the following command:"
echo $compile_command
. $compile_command || exit

################################################
# Run this app
backtrack_cmd="$BUILD_DIR/src/app/backtrack_tpstream -j $settingsFile" 
# optional overrides
if [ ! -z ${inputFile+x} ]; then
  backtrack_cmd="$backtrack_cmd --input-file $inputFile"
fi
if [ ! -z ${output_folder+x} ]; then
  backtrack_cmd="$backtrack_cmd --output-folder $output_folder"
fi
if [ ! -z "$max_files" ]; then
  backtrack_cmd="$backtrack_cmd --max-files $max_files"
fi
if [ ! -z "$skip_files" ]; then
  backtrack_cmd="$backtrack_cmd --skip-files $skip_files"
fi
if [ "$override" = true ]; then
  backtrack_cmd="$backtrack_cmd --override"
fi
if [ "$verbose" = true ]; then
  backtrack_cmd="$backtrack_cmd -v"
fi
if [ "$debug" = true ]; then
  backtrack_cmd="$backtrack_cmd -d"
fi
echo "Running: $backtrack_cmd"
exec $backtrack_cmd
