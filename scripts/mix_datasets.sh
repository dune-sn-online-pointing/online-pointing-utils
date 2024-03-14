#!bin/bash

REPO_HOME=$(git rev-parse --show-toplevel)


# move to the folder, run and come back to scripts
cd ${REPO_HOME}/app/
python dataset_mixer.py --input_json ${REPO_HOME}/json/ds-mix-mt-vs-all.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/ds-mix-mt-vs-all/
cd ${REPO_HOME}/scripts/


