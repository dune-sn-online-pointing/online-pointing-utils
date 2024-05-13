#!bin/bash

REPO_HOME=$(git rev-parse --show-toplevel)


# move to the folder, run and come back to scripts
cd ${REPO_HOME}/app/
python dataset_mixer.py --input_json ${REPO_HOME}/json/ds_mix/ds-mix-mt-vs-all.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/ds-mix-mt-vs-all/adc_80000/

# python dataset_mixer.py --input_json ${REPO_HOME}/json/ds-mix-es-cc-10000.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/ds-mix-es-cc-10000/
# python dataset_mixer.py --input_json ${REPO_HOME}/json/ds-mix-es-cc-50000.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/ds-mix-es-cc-50000/
# python dataset_mixer.py --input_json ${REPO_HOME}/json/ds-mix-es-cc-100000.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/ds-mix-es-cc-100000/
# python dataset_mixer.py --input_json ${REPO_HOME}/json/ds-mix-es-cc-200000.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/ds-mix-es-cc-200000/


# python dataset_mixer.py --input_json ${REPO_HOME}/json/ds-mix-es-cc-100000-main_track.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/ds-mix-es-cc-100000/main_track/

cd ${REPO_HOME}/scripts/


