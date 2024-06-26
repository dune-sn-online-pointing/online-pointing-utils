#!bin/bash

INPUT_JSON=/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/json/cluster_pos/cluster_pos.json
OUTPUT_FOLDER=/eos/user/d/dapullia/dune/cluster_positions/eliminami/
REPO_HOME=$(git rev-parse --show-toplevel)


# move to the folder, run and come back to scripts
cd ${REPO_HOME}/app/
python plot_cluster_positions.py --input_json ${INPUT_JSON} --output_folder ${OUTPUT_FOLDER}
cd ${REPO_HOME}/scripts/
