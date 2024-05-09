#!/bin/bash

# Check if argument is provided
if [ $# -eq 0 ]; then
    echo "Error: Please provide a number (0, 1, 2, etc.) as an argument to specify which Python script to run."
    exit 1
fi

#value of the first command-line argument
script_number="$1"

python_scripts=(
    "clusters_to_dataset.py"
    "clusters_to_dataset.py"
    "clusters_to_dataset.py"
)

input_json_files=(
    "/afs/cern.ch/work/h/hakins/private/json/ctds_main_tracks.json"
    "/afs/cern.ch/work/h/hakins/private/json/ctds_blips.json"
    "/afs/cern.ch/work/h/hakins/private/json/ctds_bkg.json"
)

output_folders=(
    "/afs/cern.ch/work/h/hakins/private/npy_dataset/es-cc_lab/main_tracks/"
    "/afs/cern.ch/work/h/hakins/private/npy_dataset/es-cc_lab/blips/"
    "/afs/cern.ch/work/h/hakins/private/npy_dataset/es-cc-bkg-truth/"
)

# check if argument is within range
if [ "$script_number" -ge 0 ] && [ "$script_number" -lt "${#python_scripts[@]}" ]; then
    # set the python script, input json file, and output folder based on the specified script number
    python_script="${python_scripts[$script_number]}"
    input_json_file="${input_json_files[$script_number]}"
    output_folder="${output_folders[$script_number]}"

    # Execute the selected Python script
    REPO_HOME=$(git rev-parse --show-toplevel)
    cd "${REPO_HOME}/app/" || exit
    python "$python_script" --input_json "$input_json_file" --output_folder "$output_folder"
    cd "${REPO_HOME}/scripts/" || exit
else
    echo "Error: Invalid script number. Please provide a number between 0 and $(( ${#python_scripts[@]} - 1 ))."
    exit 1
fi
