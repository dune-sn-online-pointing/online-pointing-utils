#!bin/bash

REPO_HOME=$(git rev-parse --show-toplevel)


# move to the folder, run and come back to scripts
cd ${REPO_HOME}/app/
python dataset_mixer.py --input_json ${REPO_HOME}/json/stadard-dataset_mixer.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/all_es_dir_list_1000/eliminami/
cd ${REPO_HOME}/scripts/


