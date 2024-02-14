#!bin/bash
# FILENAME=/eos/user/d/dapullia/root_group_files/all_es_dir_list_2/X/groups_tick_limits_3_channel_limits_1_min_tps_to_group_1.root
# OUTFOLDER=/eos/user/d/dapullia/npy_datasets/all_es_dir_list_2/groups_tick_limits_3_channel_limits_1_min_tps_to_group_1/
MIN_TPS_TO_GROUP=1
DRIFT_DIRECTION=0
SAVE_IMG_DATASET=1
SAVE_LABEL_PROCESS=0
SAVE_LABEL_DIRECTION=0

IMG_WITDH=400
IMG_HEIGHT=1500

# move to the folder, run and come back to scripts
cd ../scripts/
# python groups_to_dataset.py --input_file $FILENAME --output_path $OUTFOLDER --min_tps_to_group $MIN_TPS_TO_GROUP --drift_direction $DRIFT_DIRECTION --save_img_dataset $SAVE_IMG_DATASET --save_process_label $SAVE_LABEL_PROCESS --save_true_dir_label $SAVE_LABEL_DIRECTION
python groups_to_dataset.py --input_file /eos/user/d/dapullia/root_group_files/superimposed_files/cc-lab-bkg/superimposed_groups.root --output_path /eos/user/d/dapullia/npy_datasets/cc-lab-bkg/superimposed_groups/ --min_tps_to_group $MIN_TPS_TO_GROUP --drift_direction $DRIFT_DIRECTION --save_img_dataset $SAVE_IMG_DATASET --save_process_label $SAVE_LABEL_PROCESS --save_true_dir_label $SAVE_LABEL_DIRECTION --img_width $IMG_WITDH --img_height $IMG_HEIGHT
python groups_to_dataset.py --input_file /eos/user/d/dapullia/root_group_files/superimposed_files/es-lab-bkg/superimposed_groups.root --output_path /eos/user/d/dapullia/npy_datasets/es-lab-bkg/superimposed_groups/ --min_tps_to_group $MIN_TPS_TO_GROUP --drift_direction $DRIFT_DIRECTION --save_img_dataset $SAVE_IMG_DATASET --save_process_label $SAVE_LABEL_PROCESS --save_true_dir_label $SAVE_LABEL_DIRECTION --img_width $IMG_WITDH --img_height $IMG_HEIGHT
cd ../bash_scripts/
