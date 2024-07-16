#!/bin/bash
#_JSON=/afs/cern.ch/work/h/hakins/private/json/cluster_to_root/bkg/bkg_275000
REPO_HOME=$(git rev-parse --show-toplevel)
# parse the input
while [[ $# -gt 0 ]]; do
    case "$1" in
        --cut)
            cut="$2"
            shift 2
            ;;

        --type)
            type="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: ./cluster_to_root.sh --cut <cut> --type <type> [Either "main_track" "blip" "benchmark "or "bkg"]"
            exit 0
            ;;  
        *)
            shift
            ;;
    esac
done

output_file="/afs/cern.ch/work/h/hakins/private/json/cluster_to_root/${type}/${type}_${cut}.json"
cd /afs/cern.ch/work/h/hakins/private/online-pointing-utils/scripts/json_creators
python cluster_to_root_json_creator.py --cut "$cut" --output_file "$output_file" --type "$type" 
echo "REPO_HOME: ${REPO_HOME}"
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
echo "INput Json: ${output_file}"
# Run the app
# INPUT_JSON="/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/json/cluster_to_root/main_tracks.json"
# ./app/cluster_to_root -j $INPUT_JSON
# INPUT_JSON="/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/json/cluster_to_root/blips.json"
# ./app/cluster_to_root -j $INPUT_JSON
# INPUT_JSON="/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/json/cluster_to_root/backgrounds.json"
# ./app/cluster_to_root -j $INPUT_JSON

INPUT_JSON="/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/json/cluster_to_root/es_lab.json"
./app/cluster_to_root -j $INPUT_JSON
INPUT_JSON="/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/json/cluster_to_root/cc_lab.json"
./app/cluster_to_root -j $INPUT_JSON

cd ${REPO_HOME}/scripts/
