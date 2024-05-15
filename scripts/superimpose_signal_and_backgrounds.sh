#!bin/bash
INPUT_JSON=/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/json/superimpose_signal_and_backgrounds/standard.json
REPO_HOME=$(git rev-parse --show-toplevel)

# parse the input
while [[ $# -gt 0 ]]; do
    case "$1" in
        --input_file)
            INPUT_JSON="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: ./superimpose_signal_and_backgrounds.sh --input_file <input_json>"
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
fi

# Run the app
./app/superimpose_signal_and_backgrounds -j $INPUT_JSON
cd ${REPO_HOME}/scripts/
