#!bin/bash

REPO_HOME=$(git rev-parse --show-toplevel)

# move to the folder, run and come back to scripts
cd ${REPO_HOME}/app/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds_es_10000.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-lab/superimposed/10000/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds_es_50000.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-lab/superimposed/50000/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds_es_100000.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-lab/superimposed/100000/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds_es_200000.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-lab/superimposed/200000/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds_cc_10000.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/cc-lab/superimposed/10000/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds_cc_50000.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/cc-lab/superimposed/50000/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds_cc_100000.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/cc-lab/superimposed/100000/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds_cc_200000.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/cc-lab/superimposed/200000/X/


# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds_cc_100000-main_track.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/cc-lab/main_track/100000/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds_es_100000-main_track.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-lab/main_track/100000/X/

python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_main_tracks_direction_multiplane_clusters.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/matched_clusters/pointing_high_E/main_tracks_direction_multiplane_clusters/new_img/


# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_bkg_rnd.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/my_random_bkg_cocktail/80000/bkg/X/

# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_bkg-blips.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-cc-bkg-truth_reduced/bkg-blips/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_blips.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-cc_lab/80000/blips/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_main_tracks.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-cc_lab/80000/main_tracks/X/
cd ${REPO_HOME}/scripts/


che sono in 182 