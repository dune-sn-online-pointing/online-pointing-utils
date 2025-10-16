#!/bin/bash

# add_backgrounds.sh - Add random background events to signal TPs

# Find the root directory and initialize environment
export SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/init.sh"

print_help() {
    echo "Usage: $0 -j <json_config> [-v|--verbose] [--no-compile] [--clean-compile]"
    echo ""
    echo "Options:"
    echo "  -j, --json            <JSON configuration file> (required). Local path json/<>/<>.json is accepted."
    echo "  -v, --verbose         Enable verbose output"
    echo "      --no-compile      Skip compilation step"
    echo "      --clean-compile   Force a clean compilation"
    echo "  -h, --help            Show this help message"
    echo ""
    echo "Description:"
    echo "  Merges signal TPs with random background events to create realistic datasets."
    echo "  Outputs *_tps_bkg.root files in the same directory as input files."
}

# Parse all command line arguments
settingsFile=""
verbose=false
noCompile=""
cleanCompile=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -j|--json) settingsFile="$2"; shift 2 ;;
        -v|--verbose) verbose=true; shift ;;
        --no-compile) noCompile="--no-compile"; shift ;;
        --clean-compile) cleanCompile="--clean-compile"; shift ;;
        -h|--help) print_help; exit 0 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

################################################
# Check required arguments
if [ -z "$settingsFile" ]; then
    echo "Error: JSON configuration file is required."
    print_help
    exit 1
fi

find_settings_command="$SCRIPTS_DIR/findSettings.sh -j $settingsFile"
echo " Looking for json settings using command: $find_settings_command"
settingsFile=$($find_settings_command | tail -n 1)
echo -e "Settings file found, full path is: $settingsFile \n"

################################################
# Compile the code if requested
# pass through any compile flags provided on the command line
compile_command="$SCRIPTS_DIR/compile.sh -p $HOME_DIR $noCompile $cleanCompile"
echo "Compiling the code with the following command:"
echo $compile_command
. $compile_command || exit

################################################
# Construct command
command_to_run="$BUILD_DIR/src/app/add_backgrounds -j $settingsFile"

if [ "$verbose" = true ]; then
    command_to_run="$command_to_run -v"
fi

# Run the command
echo "Running: $command_to_run"
$command_to_run
