#!/bin/bash

# Initialize env variables similarly to cluster_to_root.sh
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPTS_DIR/init.sh

current_dir=$(pwd)

print_help() {
    echo "************************************************************************************************"
    echo "Usage: $0 -j <json_settings.json> [--clusters-file <file.root> | --filename <tps.root>]"
    echo "Options:"
    echo "  -j|--json-settings <json>  Json file used as input. Relative path inside json/"
    echo "  --clusters-file <file>     Input clusters ROOT file (if omitted, use TP ROOT via --filename)"
    echo "  -f|--filename <file>       Input TPs ROOT file (used when no clusters file is provided)"
    echo "  --mode <clusters|events>   Display mode (default: clusters)"
    echo "  --only-marley              In events mode, show only MARLEY clusters"
    echo "  --tick-limit <n>           Tick proximity for clustering (default 3)"
    echo "  --channel-limit <n>        Channel proximity for clustering (default 1)"
    echo "  --min-tps <n>              Minimum TPs per cluster (default 1)"
    echo "  --tot-cut <n>              ToT cut before clustering (default 0)"
    echo "  -v|--verbose-mode          Turn on verbosity"
    echo "  --no-compile               Do not recompile the code"
    echo "  --clean-compile            Clean and recompile the code"
    echo "  -h|--help                  Print this help message"
    echo "************************************************************************************************"
    exit 0
}

settingsFile="" # optional; can be omitted if passing files explicitly
cleanCompile=false
noCompile=false
verboseMode=false

# CLI parse mirroring cluster_to_root.sh style
clustersFile=""
filenameTPs=""
mode=""
onlyMarley=false
tickLimit=""
chanLimit=""
minTps=""
totCut=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        -j|--json-settings) settingsFile="$2"; shift 2 ;;
        --clusters-file)    clustersFile="$2"; shift 2 ;;
        -f|--filename)      filenameTPs="$2"; shift 2 ;;
        --mode)             mode="$2"; shift 2 ;;
        --only-marley)      onlyMarley=true; shift ;;
        --tick-limit)       tickLimit="$2"; shift 2 ;;
        --channel-limit)    chanLimit="$2"; shift 2 ;;
        --min-tps)          minTps="$2"; shift 2 ;;
        --tot-cut)          totCut="$2"; shift 2 ;;
        -v|--verbose-mode)  verboseMode=true; shift ;;
        --no-compile)       noCompile=true; shift ;;
        --clean-compile)    cleanCompile=true; shift ;;
        -h|--help)          print_help ;;
        *)                  echo "Unknown option: $1"; print_help ;;
    esac
done

# If a settings JSON is provided, resolve it using findSettings.sh for consistency
if [[ -n "$settingsFile" ]]; then
    find_settings_command="$SCRIPTS_DIR/findSettings.sh -j $settingsFile"
    echo " Looking for json settings using command: $find_settings_command"
    settingsFile=$($find_settings_command | tail -n 1)
    echo -e "Settings file found, full path is: $settingsFile \n"
fi

################################################
# Compile the code if requested
compile_command="$SCRIPTS_DIR/compile.sh -p $HOME_DIR --no-compile $noCompile --clean-compile $cleanCompile"
echo "Compiling the code with the following command:"
echo $compile_command
. $compile_command || exit

################################################
# Build command to run cluster_display
app="$BUILD_DIR/src/app/cluster_display"

# Assemble args for the binary
args=()
if [[ -n "$settingsFile" ]]; then
  args+=( -j "$settingsFile" )
fi
if [[ -n "$clustersFile" ]]; then
  args+=( --clusters-file "$clustersFile" )
fi
if [[ -n "$filenameTPs" ]]; then
  args+=( -f "$filenameTPs" )
fi
if [[ -n "$mode" ]]; then
  args+=( --mode "$mode" )
fi
if [[ "$onlyMarley" == true ]]; then
  args+=( --only-marley )
fi
if [[ -n "$tickLimit" ]]; then
  args+=( --tick-limit "$tickLimit" )
fi
if [[ -n "$chanLimit" ]]; then
  args+=( --channel-limit "$chanLimit" )
fi
if [[ -n "$minTps" ]]; then
  args+=( --min-tps "$minTps" )
fi
if [[ -n "$totCut" ]]; then
  args+=( --tot-cut "$totCut" )
fi
if [[ "$verboseMode" == true ]]; then
  args+=( -v )
fi

if [[ ! -x "$app" ]]; then
  echo "Error: app not found at $app" >&2
  exit 1
fi

command_to_run="$app ${args[*]}"
echo "Running command: ${command_to_run}"
# Use eval so quotes in args are respected
eval $command_to_run

cd ${current_dir}
