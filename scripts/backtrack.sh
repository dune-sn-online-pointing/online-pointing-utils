#!/bin/bash

set -e
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPTS_DIR/init.sh

print_help(){
  echo "Usage: $0 -j <json> [--no-compile] [--clean-compile] [--output-folder <dir>]"; 
  echo "Options:";
  echo "  -j|--json <json>            Json file containing settings."
  echo "  --no-compile                Do not recompile the code"
  echo "  --clean-compile             Clean and recompile the code"
  echo "  -i|--input-file <file>      Input file with list of input ROOT files. Overrides json."
  echo "  --output-folder <dir>       Output folder (default: data/). Overrides json."
  echo "  -h|--help                   Print this help message."
  exit 0;
}

# settingsFile="json/backtrack/example.json"
cleanCompile=false
noCompile=false
output_folder="$HOME_DIR/data"

while [[ $# -gt 0 ]]; do
  case "$1" in
    -j|--json) settingsFile="$2"; shift 2;;
    -i|--input-file) inputFile="$2"; shift 2;;
    --output-folder) output_folder="$2"; shift 2;;
    --no-compile) noCompile=true; shift;;
    --clean-compile) cleanCompile=true; shift;;
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
echo "Running: $backtrack_cmd"
exec $backtrack_cmd
