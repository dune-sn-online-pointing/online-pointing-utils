#!bin/bash
INPUT_JSON=/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/json/cluster_to_root/standard.json
REPO_HOME=$(git rev-parse --show-toplevel)

# parse the input
while [[ $# -gt 0 ]]; do
    case "$1" in
        --input_file)
            INPUT_JSON="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: ./cluster_to_root.sh --input_file <input_json>"
            exit 0
            ;;  
        *)
            shift
            ;;
    esac
done

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
