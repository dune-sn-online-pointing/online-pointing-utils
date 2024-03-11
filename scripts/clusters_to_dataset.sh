#!bin/bash

REPO_HOME=$(git rev-parse --show-toplevel)

# move to the folder, run and come back to scripts
cd ${REPO_HOME}/app/
python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds_bkg.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-cc-bkg-truth/bkg/X/
python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds_blips.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-cc_lab/blips/X/
python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds_main_tracks.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-cc_lab/main_tracks/X/
cd ${REPO_HOME}/scripts/


