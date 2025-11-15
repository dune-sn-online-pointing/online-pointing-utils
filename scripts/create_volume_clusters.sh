#!/bin/bash

REPO_HOME=$(git rev-parse --show-toplevel)
current_dir=$(pwd)

# init
json_settings=""
verbose=false

print_help() {
    echo "*****************************************************************************"
    echo "Usage: ./create_volume_clusters.sh --json-settings <settings.json>"
    echo "  --json-settings  json input file to run this app"
    echo "  -v|--verbose     enable verbose mode"
    echo "  -h | --help   print this help message"
    echo "*****************************************************************************"
    exit 0
}

# parse the input
while [[ $# -gt 0 ]]; do
    case "$1" in
        --json-settings) json_settings="$2"; shift 2 ;;
        -v|--verbose) verbose=true; shift ;;
        -h|--help) print_help ;;
        *) shift ;;
    esac
done

# check that json config has been selected
if [ -z "$json_settings" ]; then
    echo "Please provide a json input file"
    print_help
fi

# check that the json file exists
if [ ! -f ${REPO_HOME}/json/create_volume_clusters/${json_settings} ]; then
    echo "Json file does not exist"
    print_help
fi

echo "REPO_HOME: ${REPO_HOME}"
# compile
echo "Compiling..."
cd ${REPO_HOME}/build/
cmake ..
make -j $(nproc)
# if successful, run the app
if [ $? -ne 0 ]; then
    echo "Compilation failed"
fi

# Run the app
command_to_run="./app/create_volume_clusters -j $json_settings"
if [ "$verbose" = true ]; then
    command_to_run="$command_to_run -v"
fi
echo "Running: ${command_to_run}"
${command_to_run}

cd ${current_dir}
