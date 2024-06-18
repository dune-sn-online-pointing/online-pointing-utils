#!bin/bash

REPO_HOME=$(git rev-parse --show-toplevel)

# move to the folder, run and come back to scripts
cd ${REPO_HOME}/app/

# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_bkg.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-cc-bkg-truth/adc_80000/bkg/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_blips.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-cc_lab/adc_80000/blips/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_main_tracks.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-cc_lab/adc_80000/main_tracks/X/

python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_es-lab_volume.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/superimposed_files/es-lab_volume/
python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_cc-lab_volume.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/superimposed_files/cc-lab_volume/

# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_main_tracks_direction_multiplane_clusters.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/ctds_main_tracks_direction_multiplane_clusters-3d/



cd ${REPO_HOME}/scripts/


