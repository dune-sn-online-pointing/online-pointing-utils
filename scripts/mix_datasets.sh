
REPO_HOME=$(git rev-parse --show-toplevel)


# move to the folder, run and come back to scripts
cd ${REPO_HOME}/app/
python dataset_mixer.py --input_json /afs/cern.ch/work/h/hakins/private/json/ds-mix-met-vs-all.json --output_folder /afs/cern.ch/work/h/hakins/private/npy_dataset/ds-mix-mt-vs-all/
cd ${REPO_HOME}/scripts/


