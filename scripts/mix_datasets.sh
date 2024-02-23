INPUT_DATASET_IMG=/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/lists/npy_files/img/es-vs-cc-volume-r50.txt
INPUT_DATASET_PROCESS_LABEL=/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/lists/npy_files/process/es-vs-cc-volume-r50.txt
INPUT_DATASET_TRUE_DIR_LABEL=/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/lists/npy_files/true_dir/es-vs-cc-volume-r50.txt
OUTPUT_FOLDER=/eos/user/d/dapullia/interaction-classifier/es-vs-cc-volume-r50/
REMOVE_LABELS="99"
SHUFFLE=1
BALANCE=1


# move to the folder, run and come back to scripts
cd ../scripts/
python dataset_mixer.py --datasets_img $INPUT_DATASET_IMG --datasets_process_label $INPUT_DATASET_PROCESS_LABEL --datasets_true_dir_label $INPUT_DATASET_TRUE_DIR_LABEL --output_folder $OUTPUT_FOLDER --remove_process_labels $REMOVE_LABELS --shuffle $SHUFFLE --balance $BALANCE
cd ../bash_scripts

