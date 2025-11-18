#!bin/bash

INPUT_JSON=/afs/cern.ch/work/d/dapullia/evilla/dune/online-pointing-utils/json/plot_var.json
OUTPUT_FOLDER=/eos/user/d/dapullia/dune/cluster_variables/eliminami/
REPO_HOME=$(git rev-parse --show-toplevel)


# move to the folder, run and come back to scripts
cd ${REPO_HOME}/app/
python plot_cluster_variables.py --input_json ${INPUT_JSON} --output_folder ${OUTPUT_FOLDER}
cd ${REPO_HOME}/scripts/
