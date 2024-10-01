#!/bin/bash

REPO_HOME=$(git rev-parse --show-toplevel)
current_dir=$(pwd)

INPUT_JSON=""

print_help() {
    echo "****************************************************************************************"
    echo "Usage: ./superimpose_signal_and_backgrounds.sh --json-settings <input_json>"
    echo "Options:"
    echo "  --json-settings:    path to the json file with the settings, just relative inside the json folder"
    echo "  -h, --help:         print this message"
    echo "****************************************************************************************"
    exit 0
}

# parse the input
while [[ $# -gt 0 ]]; do
    case "$1" in
        --json-settings) INPUT_JSON="$2"; shift 2 ;;
        -h|--help) print_help ;;
        *) shift ;;
    esac
done

# check if the input json is empty
if [ -z "$INPUT_JSON" ]; then
    echo "No input json provided, needed:"
    print_help
    exit 1
fi  

# check if the json exists in the json folder
if [ ! -f "${REPO_HOME}/json/${INPUT_JSON}" ]; then
    echo "The input json file does not exist in the json folder"
    exit 1
fi

echo "REPO_HOME: ${REPO_HOME}"
# compile
echo "Compiling..."
cd ${REPO_HOME}/build/
cmake ..
make -j $(nproc)
# if unsuccessful, abort
if [ $? -ne 0 ]; then
    echo "Compilation failed"
fi

# Run the app, we're in the build directory
command_to_run="./app/superimpose_signal_and_backgrounds -j $INPUT_JSON"
echo "Running: ${command_to_run}"
${command_to_run}

# go back to the original directory
cd ${current_dir}