
REPO_HOME=$(git rev-parse --show-toplevel)

# move to the folder, run and come back to scripts
cd ${REPO_HOME}/app/
#python clusters_to_dataset.py --input_json /afs/cern.ch/work/h/hakins/private/json/ctds_main_tracks.json --output_folder /afs/cern.ch/work/h/hakins/private/npy_dataset/es-cc_lab/main_tracks/
#python clusters_to_dataset.py --input_json /afs/cern.ch/work/h/hakins/private/json/ctds_blips.json --output_folder /afs/cern.ch/work/h/hakins/private/npy_dataset/es-cc_lab/blips/
python clusters_to_dataset.py --input_json /afs/cern.ch/work/h/hakins/private/json/ctds_bkg.json --output_folder /afs/cern.ch/work/h/hakins/private/npy_dataset/es-cc-bkg-truth/

cd ${REPO_HOME}/scripts/


