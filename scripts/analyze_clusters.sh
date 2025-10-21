#!/bin/bash

# Initialize env variables
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPTS_DIR/init.sh

current_dir=$(pwd)

print_help() {
    echo "*******************************************************************************"
    echo "Usage: $0 -j <json_settings.json>"
    echo "Options:"
    echo "  --json-settings <json>      Json file to used as input. Relative path inside json/"
    echo "  --no-compile                Do not recompile the code. Default is to recompile the code"
    echo "  --clean-compile             Clean and recompile the code. Default is to recompile the code without cleaning"
    echo "  -i|--input-file <file>      Input file with list of input ROOT files. Overrides json."
    echo "  --output-folder <dir>       Output folder (default: data/). Overrides json."
    echo "  -v|--verbose                Enable verbose mode"
    echo "  -d|--debug                  Enable debug mode"
    echo "  -h, --help                  Print this help message"
    echo "*******************************************************************************"
    exit 0
}

cleanCompile=false
noCompile=false
verbose=false

# Parse
while [[ $# -gt 0 ]]; do
    case "$1" in
        -j|--json-settings) settingsFile="$2"; shift 2 ;;
        -i|--input-file)    inputFile="$2"; shift 2;;
        --output-folder)    output_folder="$2"; shift 2;;
        --no-compile)       noCompile=true; shift ;;
        --clean-compile)    cleanCompile=true; shift ;;
        -d|--debug)          debug=true; shift ;;
        -v|--verbose)        verbose=true; shift ;;
        -h|--help)          print_help ;;
        *)                  shift ;;
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
command_to_run="$BUILD_DIR/src/app/analyze_clusters -j $settingsFile"
# optional overrides
if [ ! -z ${inputFile+x} ]; then
    command_to_run="$command_to_run --input-file $inputFile"
fi
if [ ! -z ${output_folder+x} ]; then
    command_to_run="$command_to_run --output-folder $output_folder"
fi
if [ "$verbose" = true ]; then
    command_to_run="$command_to_run -v"
fi
if [ "$debug" = true ]; then
    command_to_run="$command_to_run -d"
fi

echo "Running command: ${command_to_run}"
eval $command_to_run
