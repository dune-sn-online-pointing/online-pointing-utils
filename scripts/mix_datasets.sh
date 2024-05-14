#!bin/bash

REPO_HOME=$(git rev-parse --show-toplevel)


# move to the folder, run and come back to scripts
cd ${REPO_HOME}/app/
# python dataset_mixer.py --input_json ${REPO_HOME}/json/ds_mix/ds-mix-mt-vs-all.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/ds-mix-mt-vs-all/adc_80000/

python dataset_mixer.py --input_json ${REPO_HOME}/json/ds_mix/ds-mix-es-cc-volume.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/ds-mix-es-cc-volume/

cd ${REPO_HOME}/scripts/


