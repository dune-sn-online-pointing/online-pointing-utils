#!/bin/bash

# add_backgrounds.sh - Add random background events to signal TPs

# Find the root directory and initialize environment
export SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/init.sh"

print_help() {
    echo "Usage: $0 -j <json_config> [-i <input>] [-o <output>] [-s <skip>] [-m <max>] [-v|--verbose] [-f|--override] [--no-compile] [--clean-compile]"
    echo ""
    echo "Options:"
    echo "  -j, --json            <JSON configuration file> (required). Local path json/<>/<>.json is accepted."
    echo "  -i, --input-file      Input file or list (overrides JSON)"
    echo "  -o, --output-folder   Output folder (overrides JSON)"
    echo "  -s, --skip            Number of files to skip at start (overrides JSON)"
    echo "  -m, --max             Maximum number of files to process (overrides JSON)"
    echo "  -v, --verbose         Enable verbose output"
    echo "  -f, --override        Override existing output files"
    echo "      --no-compile      Skip compilation step"
    echo "      --clean-compile   Force a clean compilation"
    echo "  -h, --help            Show this help message"
    echo ""
    echo "Description:"
    echo "  Merges signal TPs with random background events to create realistic datasets."
    echo "  Outputs *_tps_bkg.root files in the specified output folder."
}

# Parse all command line arguments
settingsFile=""
inputFile=""
output_folder=""
skip_files=""
max_files=""
verbose=false
override=false
noCompile=""
cleanCompile=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    -j|--json) settingsFile="$2"; shift 2;;
    -i|--input-file) inputFile="$2"; shift 2;;
    -o|--output-folder) output_folder="$2"; shift 2;;
  -s|--skip|--skip-files) skip_files="$2"; shift 2;;
  -m|--max|--max-files) max_files="$2"; shift 2;;
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
compile_command="$SCRIPTS_DIR/compile.sh -p $HOME_DIR --no-compile $noCompile --clean-compile $cleanCompile"
echo "Compiling the code with the following command:"
echo $compile_command
. $compile_command || exit

################################################
# Construct command
command_to_run="$BUILD_DIR/src/app/add_backgrounds -j $settingsFile"

if [ ! -z "$skip_files" ]; then
    command_to_run="$command_to_run -s $skip_files"
fi
if [ ! -z "$max_files" ]; then
    command_to_run="$command_to_run -m $max_files"
fi
if [ "$verbose" = true ]; then
    command_to_run="$command_to_run -v"
fi
if [ "$debug" = true ]; then
    command_to_run="$command_to_run -d"
fi
if [ "$override" = true ]; then
    command_to_run="$command_to_run -f"
fi



# Run the command
echo "Running: $command_to_run"
$command_to_run
