#!/bin/bash

### !!!!! THIS IS FROM HARRY, SPECIAL APPLICATION

REPO_HOME=$(git rev-parse --show-toplevel)
current_dir=$(pwd)

print_help() {
    echo "************************************************************************************************"
    echo "Usage: ./cluster_to_root.sh --cut <cut> --type <type> [Either "main_track" "blip" "benchmark "or "bkg"]"
    echo "Options:"
    echo "  --cut <cut>                           Cut value"
    echo "  --type <type>                         Type of the dataset, either 'main_track', 'blip', 'bkg', 'benchmark'"
    echo "  -h, --help                            Print this help message"
    echo "************************************************************************************************"
    exit 0
}

# Parse
while [[ $# -gt 0 ]]; do
    case "$1" in
        --output-folder) output_folder="$2"; shift 2 ;;
        --cut)          cut="$2"; shift 2 ;;
        --type)         type="$2"; shift 2 ;;
        -h|--help)      print_help ;;
        *)              shift ;;
    esac
done

echo "REPO_HOME: ${REPO_HOME}"
json_settings_file="${output_folder}/cluster_to_root/${type}/${type}_${cut}.json"
echo "Json file to be created: ${json_settings_file}"

echo "Creating json config from the information provided..."
python ${REPO_HOME}/python/cluster_to_root_json_creator.py --cut "$cut" --output_file "$json_settings_file" --type "$type" 
echo "Json settings file created at ${json_settings_file}"

# compile
echo "Compiling..."
cd ${REPO_HOME}/build/
cmake ..
make -j $(nproc)
# if successful, run the app
if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi

# Run the app
# TODO still fix this
# ES
INPUT_JSON="/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/json/cluster_to_root/es_lab.json"
./app/cluster_to_root -j $INPUT_JSON
# CC
# INPUT_JSON="/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/json/cluster_to_root/cc_lab.json"
# ./app/cluster_to_root -j $INPUT_JSON

cd ${current_dir}
